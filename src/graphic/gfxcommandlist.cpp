#include "gfxcommandlist.h"

#include "graphic/gfxmanager.h"
#include "graphic/dx12utils.h"
#include "graphic/gfxdevice.h"

void GfxCommandList::Initialize(D3D12_COMMAND_LIST_TYPE cmdListType)
{
    bbeProfileFunction();

    GfxDevice& gfxDevice = GfxManager::GetInstance().GetGfxDevice();

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

    GfxDevice& gfxDevice = GfxManager::GetInstance().GetGfxDevice();

    CommandListPool& pool = m_Pools[D3D12_COMMAND_LIST_TYPE_DIRECT];

    assert(pool.m_CommandQueue.Get() == nullptr);

    // Describe and create the command queue.
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.NodeMask = 0; // For single-adapter, set to 0. Else, set a bit to identify the node

    DX12_CALL(gfxDevice.Dev()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&pool.m_CommandQueue)));

    // Init some free cmd lists
    for (uint32_t i = 0; i < 32; ++i)
    {
        GfxCommandList* newCmdList = pool.m_CommandListsPool.construct();
        newCmdList->Initialize(D3D12_COMMAND_LIST_TYPE_DIRECT);
        pool.m_FreeCommandLists.push(newCmdList);
    }
}

GfxCommandList* GfxCommandListsManager::Allocate(D3D12_COMMAND_LIST_TYPE cmdListType)
{
    bbeProfileFunction();

    CommandListPool& pool = m_Pools[cmdListType];
    GfxCommandList* newCmdList = nullptr;
    bbeOnExitScope
    {
        newCmdList->BeginRecording(); 
        pool.m_ActiveCommandLists.push(newCmdList);
    };

    pool.m_FreeCommandLists.consume_one([&](GfxCommandList* poppedFreeList)
        {
            newCmdList = poppedFreeList;
        });
    if (newCmdList)
    {
        return newCmdList;
    }

    newCmdList = pool.m_CommandListsPool.construct();
    newCmdList->Initialize(cmdListType);

    return newCmdList;
}

void GfxCommandListsManager::ExecuteAllActiveCommandLists()
{
    bbeProfileFunction();

    for (auto& cmdListTypePoolPair : m_Pools)
    {
        CommandListPool& pool = cmdListTypePoolPair.second;

        pool.m_ActiveCommandLists.consume_all([&](GfxCommandList* cmdListToConsume)
            {
                cmdListToConsume->EndRecording();

                ID3D12CommandList* ppCommandLists[] = { cmdListToConsume->Dev() };
                pool.m_CommandQueue->ExecuteCommandLists(1, ppCommandLists);

                pool.m_FreeCommandLists.push(cmdListToConsume);
            });
    }
}
