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

    bbeProfile("CreateCommandQueue");
    DX12_CALL(gfxDevice.Dev()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&directPool.m_CommandQueue)));
    g_Profiler.InitializeGPUProfiler(gfxDevice.Dev(), directPool.m_CommandQueue.Get());
    g_Profiler.RegisterGPUQueue(directPool.m_CommandQueue.Get(), "Direct Queue");
}

void GfxCommandListsManager::ShutDown()
{
    bbeProfileFunction();

    assert(m_DirectPool.m_ActiveCommandLists.empty() == true);

    // free all cmd lists
    while (!m_DirectPool.m_FreeCommandLists.empty())
    {
        GfxCommandList* cmdList = nullptr;
        if (m_DirectPool.m_FreeCommandLists.try_pop(cmdList))
        {
            m_DirectPool.m_CommandListsPool.free(cmdList);
        }
    }
}

GfxCommandList* GfxCommandListsManager::Allocate(D3D12_COMMAND_LIST_TYPE cmdListType)
{
    bbeProfileFunction();

    GfxCommandList* newCmdList = nullptr;

    // if no free list, create a new one
    if (m_DirectPool.m_FreeCommandLists.empty())
    {
        bbeAutoLock(m_DirectPool.m_PoolLock);
        newCmdList = m_DirectPool.m_CommandListsPool.construct();
        assert(newCmdList);

        newCmdList->Initialize(cmdListType);
    }
    else
    {
        while (!m_DirectPool.m_FreeCommandLists.try_pop(newCmdList));
        assert(newCmdList);
    }

    newCmdList->BeginRecording();
    m_DirectPool.m_ActiveCommandLists.push(newCmdList);

    return newCmdList;
}

void GfxCommandListsManager::ExecuteAllActiveCommandLists()
{
    bbeMultiThreadDetector();
    bbeProfileFunction();

    while (!m_DirectPool.m_ActiveCommandLists.empty())
    {
        GfxCommandList* cmdListToConsume = nullptr;
        if (m_DirectPool.m_ActiveCommandLists.try_pop(cmdListToConsume))
        {
            cmdListToConsume->EndRecording();

            ID3D12CommandList* ppCommandLists[] = { cmdListToConsume->Dev() };
            m_DirectPool.m_CommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

            m_DirectPool.m_FreeCommandLists.push(cmdListToConsume);
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
