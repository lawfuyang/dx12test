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

    DX12_CALL(gfxDevice.Dev()->CreateCommandAllocator(cmdListType, IID_PPV_ARGS(&m_CommandAllocator)));

    assert(m_CommandList == nullptr);

    // Create the command list.
    DX12_CALL(gfxDevice.Dev()->CreateCommandList(nodeMask, m_Type, m_CommandAllocator.Get(), initialState, IID_PPV_ARGS(&m_CommandList)));

    // Command lists are created in the recording state, but there is nothing
    // to record yet. The main loop expects it to be closed, so close it now.
    DX12_CALL(m_CommandList->Close());
}

void GfxCommandList::BeginRecording()
{
    // Command list allocators can only be reset when the associated command lists have finished execution on the GPU
    // Apps should use fences to determine GPU execution progress
    DX12_CALL(m_CommandAllocator->Reset());

    // When ExecuteCommandList() is called on a particular command list, that command list can then be reset at any time and must be before re-recording
    DX12_CALL(m_CommandList->Reset(m_CommandAllocator.Get(), nullptr));
}

void GfxCommandListQueue::Initialize(D3D12_COMMAND_LIST_TYPE type)
{
    bbeProfileFunction();

    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();

    m_Type = type;

    m_Fence.Initialize();

    // Describe and create the command queue.
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = type;
    queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.NodeMask = 0; // For single-adapter, set to 0. Else, set a bit to identify the node

    {
        bbeProfile("CreateCommandQueue");
        DX12_CALL(gfxDevice.Dev()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_CommandQueue)));
    }

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

    // init a bunch of free cmd lists first
    m_CommandLists.set_capacity(MaxCmdLists);
    for (uint32_t i = 0; i < MaxCmdLists; ++i)
    {
        GfxCommandList newCmdList;
        newCmdList.Initialize(type);
        m_CommandLists.push_back(newCmdList);
    }
}

void GfxCommandListQueue::ShutDown()
{
    assert(m_PendingExecuteCommandLists.empty());

    // free all cmd lists
    for (GfxCommandList& cmdList : m_CommandLists)
    {
        cmdList.Dev()->Release();
    }
}

GfxCommandList* GfxCommandListQueue::AllocateCommandList(std::string_view name)
{
    bbeProfileFunction();

    // before allocating, make sure the queue itself is already initialized
    assert(!m_CommandLists.empty());

    bbeAutoLock(m_ListsLock);

    // get new cmd list in front of circular buffer and rotate it
    GfxCommandList* newCmdList = &*m_CommandLists.begin();
    assert(m_CommandLists.full());
    m_CommandLists.rotate(m_CommandLists.begin() + 1);

    // begin recording
    assert(newCmdList->Dev());
    newCmdList->BeginRecording();

    // set debug names
    SetD3DDebugName(newCmdList->Dev(), name);
    SetD3DDebugName(newCmdList->m_CommandAllocator.Get(), name);

    return newCmdList;
}

void GfxCommandListQueue::ExecutePendingCommandLists() 
{
    uint32_t numCmdListsToExec = 0;
    ID3D12CommandList* ppCommandLists[MaxCmdLists] = {};

    // end recording for all pending cmd lists
    {
        bbeAutoLock(m_ListsLock);

        for (GfxCommandList* cmdListToConsume : m_PendingExecuteCommandLists)
        {
            DX12_CALL(cmdListToConsume->Dev()->Close());

            ppCommandLists[numCmdListsToExec++] = cmdListToConsume->Dev();
        }
        m_PendingExecuteCommandLists.clear();
    }

    // execute cmd lists
    if (numCmdListsToExec > 0)
    {
        Dev()->ExecuteCommandLists(numCmdListsToExec, ppCommandLists);    
    }

    SignalFence();
}

void GfxCommandListsManager::Initialize(tf::Subflow& sf)
{
    for (GfxCommandListQueue* queue : m_AllQueues)
    {
        assert(queue->m_CommandQueue.Get() == nullptr);
    }
    sf.emplace([this] { m_DirectQueue.Initialize(D3D12_COMMAND_LIST_TYPE_DIRECT); });
    sf.emplace([this] { m_AsyncComputeQueue.Initialize(D3D12_COMMAND_LIST_TYPE_COMPUTE); });
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
