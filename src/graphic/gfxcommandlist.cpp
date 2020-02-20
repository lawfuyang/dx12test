#include "gfxcommandlist.h"

#include "graphic/gfxmanager.h"
#include "graphic/dx12utils.h"
#include "graphic/gfxdevice.h"

void GfxCommandList::Initialize(D3D12_COMMAND_LIST_TYPE cmdListType)
{
    bbeProfileFunction();

    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();

    m_Type = cmdListType;

    // For single-adapter, set to 0. Else, set a bit to identify the node
    const UINT nodeMask = 0;

    // Set a dummy initial pipeline state, so that drivers don't have to deal with undefined state
    // Overhead for this is low, and cost of recording command lists dwarfs cost of a single initial state setting. 
    // So there's little cost in not setting the initial pipeline state parameter, as its really inconvenient.
    ID3D12PipelineState* initialState = nullptr;

    DX12_CALL(gfxDevice.Dev()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_CommandAllocator)));

    for (uint32_t i = 0; i < 2; ++i)
    {
        assert(m_CommandList[i] == nullptr);

        // Create the command list.
        DX12_CALL(gfxDevice.Dev()->CreateCommandList(nodeMask, m_Type, m_CommandAllocator.Get(), initialState, IID_PPV_ARGS(&m_CommandList[i])));

        // Command lists are created in the recording state, but there is nothing
        // to record yet. The main loop expects it to be closed, so close it now.
        DX12_CALL(m_CommandList[i]->Close());
    }
}

void GfxCommandList::BeginRecording()
{
    bbeProfileFunction();

    m_RecordingCmdListIdx = 1 - m_RecordingCmdListIdx;

    // Command list allocators can only be reset when the associated command lists have finished execution on the GPU
    // Apps should use fences to determine GPU execution progress
    DX12_CALL(m_CommandAllocator->Reset());

    // When ExecuteCommandList() is called on a particular command list, that command list can then be reset at any time and must be before re-recording
    DX12_CALL(m_CommandList[m_RecordingCmdListIdx]->Reset(m_CommandAllocator.Get(), nullptr));
}

void GfxCommandList::EndRecording()
{
    DX12_CALL(m_CommandList[m_RecordingCmdListIdx]->Close());
}

// TODO: Make queues for different types of cmd lists
void GfxCommandListsManager::Initialize()
{
    bbeProfileFunction();
    g_Log.info("Initializing GfxCommandListsManager");

    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();

    CommandListPool& directPool = m_DirectPool;

    assert(directPool.m_CommandQueue.Get() == nullptr);

    // Describe and create the command queue.
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.NodeMask = 0; // For single-adapter, set to 0. Else, set a bit to identify the node

    // init ID3D12CommandQueue & some free ID3D12GraphicsCommandLists in parallel
    tf::Taskflow tf;

    tf.emplace([&]()
        {
            bbeProfile("CreateCommandQueue");
            DX12_CALL(gfxDevice.Dev()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&directPool.m_CommandQueue)));
            g_Profiler.InitializeGPUProfiler(gfxDevice.Dev(), directPool.m_CommandQueue.Get());
            g_Profiler.RegisterGPUQueue(directPool.m_CommandQueue.Get(), "Direct Queue");
        }).name("CreateCommandQueue");

    bool dummyArr[32];
    tf.parallel_for(dummyArr, dummyArr + _countof(dummyArr), [&](bool)
        {
            GfxCommandList* newCmdList = [&]()
            {
                static AdaptiveLock s_Lock;
                bbeAutoLock(s_Lock);
                return directPool.m_CommandListsPool.construct();
            }();
            assert(newCmdList);
            newCmdList->Initialize(D3D12_COMMAND_LIST_TYPE_DIRECT);

            static AdaptiveLock s_Lock;
            bbeAutoLock(s_Lock);
            directPool.m_FreeCommandLists.push(newCmdList);
        });

    g_TasksExecutor.run(tf).wait();
}

void GfxCommandListsManager::ShutDown()
{
    bbeProfileFunction();

    assert(m_DirectPool.m_ActiveCommandLists.empty() == true);

    while (m_DirectPool.m_FreeCommandLists.size())
    {
        GfxCommandList* cmdList = m_DirectPool.m_FreeCommandLists.front();
        m_DirectPool.m_FreeCommandLists.pop();
        m_DirectPool.m_CommandListsPool.free(cmdList);
    }
}

GfxCommandList* GfxCommandListsManager::Allocate(D3D12_COMMAND_LIST_TYPE cmdListType)
{
    bbeMultiThreadDetector();
    bbeProfileFunction();

    // TODO: Need to investigate why at high frame rates (>60 fps), it's not allocating proper free cmd lists
    //       It's always calling "BeginRecording" on recording cmd lists

    CommandListPool& pool = GetPoolFromType(cmdListType);
    GfxCommandList* newCmdList = nullptr;

    // try getting a free cmd list first
    if (pool.m_FreeCommandLists.size())
    {
        newCmdList = pool.m_FreeCommandLists.front();
        assert(newCmdList);
        pool.m_FreeCommandLists.pop();
    }
    else
    {
        // if no free list, create a new one
        newCmdList = pool.m_CommandListsPool.construct();
        assert(newCmdList);
        newCmdList->Initialize(cmdListType);
    }
    
    // We should have a legit cmd list at this point
    assert(newCmdList);

    newCmdList->BeginRecording();
    pool.m_ActiveCommandLists.push(newCmdList);

    return newCmdList;
}

void GfxCommandListsManager::ExecuteAllActiveCommandLists()
{
    bbeMultiThreadDetector();
    bbeProfileFunction();

    CommandListPool* const allPools[] =
    {
        &m_DirectPool,
    };

    for (CommandListPool* pool : allPools)
    {
        while (pool->m_ActiveCommandLists.size())
        {
            GfxCommandList* cmdListToConsume = pool->m_ActiveCommandLists.front();
            pool->m_ActiveCommandLists.pop();

            cmdListToConsume->EndRecording();

            ID3D12CommandList* ppCommandLists[] = { cmdListToConsume->Dev() };
            pool->m_CommandQueue->ExecuteCommandLists(1, ppCommandLists);

            pool->m_FreeCommandLists.push(cmdListToConsume);
        }
    }
}

GfxCommandListsManager::CommandListPool& GfxCommandListsManager::GetPoolFromType(D3D12_COMMAND_LIST_TYPE type)
{
    switch (type)
    {
    case D3D12_COMMAND_LIST_TYPE_DIRECT: return m_DirectPool;
    default: assert(false); return m_DirectPool;
    }
}
