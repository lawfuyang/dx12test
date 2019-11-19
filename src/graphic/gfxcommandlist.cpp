#include "gfxcommandlist.h"

#include "graphic/dx12utils.h"
#include "graphic/gfxdevice.h"
#include "graphic/gfxpipelinestate.h"

void GfxCommandList::Initialize(GfxDevice& gfxDevice, D3D12_COMMAND_LIST_TYPE cmdListType)
{
    bbeProfileFunction();

    bbeAssert(m_CommandList.Get() == nullptr, "");

    // For single-adapter, set to 0. Else, set a bit to identify the node
    const UINT nodeMask = 0;

    // Set a dummy initial pipeline state, so that drivers don't have to deal with undefined state
    // Overhead for this is low, and cost of recording command lists dwarfs cost of a single initial state setting. 
    // So there's little cost in not setting the initial pipeline state parameter, as its really inconvenient.
    ID3D12PipelineState* initialState = nullptr;

    DX12_CALL(gfxDevice.Dev()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_CommandAllocator)));

    // Create the command list.
    DX12_CALL(gfxDevice.Dev()->CreateCommandList(nodeMask, cmdListType, m_CommandAllocator.Get(), initialState, IID_PPV_ARGS(&m_CommandList)));

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
