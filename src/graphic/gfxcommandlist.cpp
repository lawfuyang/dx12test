#include "graphic/gfxcommandlist.h"

#include "graphic/gfxmanager.h"
#include "graphic/dx12utils.h"
#include "graphic/gfxdevice.h"

// copy-paste from MicroProfile-3.1
struct GPULogMetadata
{
    using GPULogEntryType = uint64_t;
    static const uint32_t GPU_LOG_BUFFER_SIZE = (MICROPROFILE_PER_THREAD_GPU_BUFFER_SIZE) / sizeof(GPULogEntryType);
    static const uint32_t GPU_LOG_SCOPE_STACK_MAX = 32;

    GPULogEntryType Log[GPU_LOG_BUFFER_SIZE];
    uint32_t nPut;
    uint32_t nStart;
    uint32_t nId;
    void* pContext;
    uint32_t nAllocated;
    uint32_t nStackScope;
    MicroProfileScopeStateC ScopeState[GPU_LOG_SCOPE_STACK_MAX];
};

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

    assert(m_CommandList == nullptr);

    // Create the command list.
    DX12_CALL(gfxDevice.Dev()->CreateCommandList(nodeMask, m_Type, m_CommandAllocator.Get(), initialState, IID_PPV_ARGS(&m_CommandList)));

    // Command lists are created in the recording state, but there is nothing
    // to record yet. The main loop expects it to be closed, so close it now.
    DX12_CALL(m_CommandList->Close());
}

void GfxCommandList::BeginRecording()
{
    bbeProfileFunction();

    // Command list allocators can only be reset when the associated command lists have finished execution on the GPU
    // Apps should use fences to determine GPU execution progress
    DX12_CALL(m_CommandAllocator->Reset());

    // When ExecuteCommandList() is called on a particular command list, that command list can then be reset at any time and must be before re-recording
    DX12_CALL(m_CommandList->Reset(m_CommandAllocator.Get(), nullptr));

    m_Log = MicroProfileThreadLogGpuAlloc();
    MICROPROFILE_GPU_BEGIN(m_CommandList.Get(), m_Log);
}

void GfxCommandList::EndRecording()
{
    DX12_CALL(m_CommandList->Close());
}

void GfxCommandListsManager::Initialize()
{
    bbeProfileFunction();
    g_Log.info("Initializing GfxCommandListsManager");

    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();

    assert(m_DirectPool.m_CommandQueue.Get() == nullptr);

    // Describe and create the command queue.
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.NodeMask = 0; // For single-adapter, set to 0. Else, set a bit to identify the node

    bbeProfile("CreateCommandQueue");
    DX12_CALL(gfxDevice.Dev()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_DirectPool.m_CommandQueue)));
    SetD3DDebugName(m_DirectPool.m_CommandQueue.Get(), "Main Direct Queue");

    g_Profiler.InitializeGPUProfiler(gfxDevice.Dev(), m_DirectPool.m_CommandQueue.Get());
    g_Profiler.RegisterGPUQueue(m_DirectPool.m_CommandQueue.Get(), "Direct Queue");
}

void GfxCommandListsManager::ShutDown()
{
    bbeProfileFunction();
    bbeMultiThreadDetector();

    assert(m_DirectPool.m_ActiveCommandLists.empty() == true);

    // free all cmd lists
    while (!m_DirectPool.m_FreeCommandLists.empty())
    {
        GfxCommandList* cmdList = m_DirectPool.m_FreeCommandLists.front();
        m_DirectPool.m_FreeCommandLists.pop();
        m_DirectPool.m_CommandListsPool.free(cmdList);
    }
}

GfxCommandList* GfxCommandListsManager::Allocate(D3D12_COMMAND_LIST_TYPE cmdListType, const std::string& name)
{
    bbeProfileFunction();

    GfxCommandList* newCmdList = nullptr;
    bool isNewCmdList = false;
    {
        bbeAutoLock(m_DirectPool.m_ListsLock);

        // if no free list, create a new one
        if (m_DirectPool.m_FreeCommandLists.empty())
        {
            newCmdList = m_DirectPool.m_CommandListsPool.construct();
            isNewCmdList = true;
        }
        else
        {
            newCmdList = m_DirectPool.m_FreeCommandLists.front();
            m_DirectPool.m_FreeCommandLists.pop();
        }
    }

    assert(newCmdList);
    if (isNewCmdList)
    {
        newCmdList->Initialize(cmdListType);
    }

    assert(newCmdList->Dev());
    newCmdList->SetName(name);
    SetD3DDebugName(newCmdList->Dev(), name);

    newCmdList->BeginRecording();

    {
        bbeAutoLock(m_DirectPool.m_ListsLock);
        m_DirectPool.m_ActiveCommandLists.push(newCmdList);
    }

    return newCmdList;
}

void GfxCommandListsManager::ExecuteAllActiveCommandLists()
{
    bbeProfileFunction();

    const uint32_t MAX_CMD_LISTS = 128;

    uint32_t numCmdListsToExec = 0;
    GfxCommandList* ppPendingFreeCommandLists[MAX_CMD_LISTS] = {};
    ID3D12CommandList* ppCommandLists[MAX_CMD_LISTS] = {};

    // end recording for all active cmd lists and remove them from active queue
    {
        bbeAutoLock(m_DirectPool.m_ListsLock);
        assert(m_DirectPool.m_ActiveCommandLists.size() < MAX_CMD_LISTS);

        while (!m_DirectPool.m_ActiveCommandLists.empty())
        {
            GfxCommandList* cmdListToConsume = m_DirectPool.m_ActiveCommandLists.front();
            cmdListToConsume->EndRecording();

            ppPendingFreeCommandLists[numCmdListsToExec] = cmdListToConsume;
            ppCommandLists[numCmdListsToExec++] = cmdListToConsume->Dev();
            m_DirectPool.m_ActiveCommandLists.pop();
        }
    }

    // Submit GPU profiler logs
    for (uint32_t i = 0; i < numCmdListsToExec; ++i)
    {
        const uint64_t token = MICROPROFILE_GPU_END(ppPendingFreeCommandLists[i]->m_Log);
        MICROPROFILE_GPU_SUBMIT(g_Profiler.GetGPUQueueHandle(m_DirectPool.m_CommandQueue.Get()), token);
        MicroProfileThreadLogGpuFree(ppPendingFreeCommandLists[i]->m_Log);
    }
    
    // execute cmd lists
    if (numCmdListsToExec > 0)
    {
        m_DirectPool.m_CommandQueue->ExecuteCommandLists(numCmdListsToExec, ppCommandLists);
    }

    // add executed cmd lists to free list queue
    {
        bbeAutoLock(m_DirectPool.m_ListsLock);

        for (uint32_t i = 0; i < numCmdListsToExec; ++i)
        {
            m_DirectPool.m_FreeCommandLists.push(ppPendingFreeCommandLists[i]);
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
