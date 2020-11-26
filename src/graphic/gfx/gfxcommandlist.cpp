#include <graphic/gfx/gfxcommandlist.h>
#include <graphic/pch.h>

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
}

void GfxCommandListQueue::Initialize(D3D12_COMMAND_LIST_TYPE type)
{
    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();

    m_Type = type;

    m_Fence.Initialize();

    // Describe and create the command queue.
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.NodeMask = 0; // For single-adapter, set to 0. Else, set a bit to identify the node

    bbeProfile("CreateCommandQueue");
    DX12_CALL(gfxDevice.Dev()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_CommandQueue)));

    static const char* QUEUE_NAMES[] =
    {
        "D3D12_COMMAND_LIST_TYPE_DIRECT",
        "D3D12_COMMAND_LIST_TYPE_BUNDLE",
        "D3D12_COMMAND_LIST_TYPE_COMPUTE",
        "D3D12_COMMAND_LIST_TYPE_COPY",
        "D3D12_COMMAND_LIST_TYPE_VIDEO_DECODE",
        "D3D12_COMMAND_LIST_TYPE_VIDEO_PROCESS",
        "D3D12_COMMAND_LIST_TYPE_VIDEO_ENCODE",
    };

    SetD3DDebugName(Dev(), QUEUE_NAMES[type]);
    g_Profiler.RegisterGPUQueue(gfxDevice.Dev(), Dev(), QUEUE_NAMES[type]);

    m_FreeCommandLists.set_capacity(MaxCmdLists);
}

void GfxCommandListQueue::ShutDown()
{
    assert(m_PendingExecuteCommandLists.empty());

    // free all cmd lists
    for (GfxCommandList* cmdList : m_FreeCommandLists)
    {
        m_CommandListsPool.free(cmdList);
    }
}

GfxCommandList* GfxCommandListQueue::AllocateCommandList(const std::string& name)
{
    bbeProfileFunction();

    assert(m_FreeCommandLists.capacity());

    GfxCommandList* newCmdList = nullptr;
    bool isNewCmdList = false;
    {
        bbeAutoLock(m_ListsLock);

        // if no free list, create a new one
        if (m_FreeCommandLists.empty())
        {
            newCmdList = m_CommandListsPool.construct();
            isNewCmdList = true;
        }
        else
        {
            newCmdList = m_FreeCommandLists.front();
            m_FreeCommandLists.pop_front();
        }
    }

    assert(newCmdList);
    if (isNewCmdList)
    {
        newCmdList->Initialize(m_Type);
    }

    assert(newCmdList->Dev());
    SetD3DDebugName(newCmdList->Dev(), name);
    SetD3DDebugName(newCmdList->m_CommandAllocator.Get(), name);

    newCmdList->BeginRecording();

    return newCmdList;
}

void GfxCommandListQueue::ExecutePendingCommandLists() 
{
    uint32_t numCmdListsToExec = 0;
    GfxCommandList* ppPendingFreeCommandLists[MaxCmdLists] = {};
    ID3D12CommandList* ppCommandLists[MaxCmdLists] = {};

    // end recording for all pending cmd lists
    {
        bbeAutoLock(m_ListsLock);

        for (GfxCommandList* cmdListToConsume : m_PendingExecuteCommandLists)
        {
            DX12_CALL(cmdListToConsume->Dev()->Close());

            ppPendingFreeCommandLists[numCmdListsToExec] = cmdListToConsume;
            ppCommandLists[numCmdListsToExec++] = cmdListToConsume->Dev();
        }
        m_PendingExecuteCommandLists.clear();
    }

    g_Profiler.SubmitAllGPULogsToQueue(m_CommandQueue.Get());

    // add executed cmd lists to inflight list queue
    {
        bbeAutoLock(m_ListsLock);

        assert(m_FreeCommandLists.size() + numCmdListsToExec < MaxCmdLists);
        for (uint32_t i = 0; i < numCmdListsToExec; ++i)
        {
            m_FreeCommandLists.push_back(ppPendingFreeCommandLists[i]);
        }
    }

    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();

    // execute cmd lists
    if (numCmdListsToExec > 0)
    {
        // Before submitting any cmd lists, wait for prev submission to complete first
        StallGPUForFence();

        Dev()->ExecuteCommandLists(numCmdListsToExec, ppCommandLists);
    }

    SignalFence();
}

void GfxCommandListsManager::Initialize()
{
    bbeProfileFunction();
    g_Log.info("Initializing GfxCommandListsManager");

    for (GfxCommandListQueue* queue : m_AllQueues)
    {
        assert(queue->m_CommandQueue.Get() == nullptr);
    }
    m_DirectQueue.Initialize(D3D12_COMMAND_LIST_TYPE_DIRECT);
}

void GfxCommandListsManager::ShutDown()
{
    bbeProfileFunction();
    bbeMultiThreadDetector();

    for (GfxCommandListQueue* queue : m_AllQueues)
    {
        queue->ShutDown();
    }
}

void GfxCommandListsManager::QueueCommandListToExecute(GfxCommandList* cmdList)
{
    assert(cmdList);
    GfxCommandListQueue& queue = GetCommandQueue(cmdList->m_Type);

    bbeAutoLock(queue.m_ListsLock);
    assert(queue.m_PendingExecuteCommandLists.size() < GfxCommandListQueue::MaxCmdLists);
    queue.m_PendingExecuteCommandLists.push_back(cmdList);
}

void GfxCommandListsManager::ExecutePendingCommandLists()
{
    bbeProfileFunction();

    for (GfxCommandListQueue* queue : m_AllQueues)
    {
        queue->ExecutePendingCommandLists();
    }
}
