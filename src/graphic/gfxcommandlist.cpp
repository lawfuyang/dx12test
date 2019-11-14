#include "gfxcommandlist.h"

#include "graphic/dx12utils.h"
#include "graphic/gfxdevice.h"

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

    // Create the command list.
    DX12_CALL(gfxDevice.Dev()->CreateCommandList(nodeMask, cmdListType, gfxDevice.GetCommandAllocator(), initialState, IID_PPV_ARGS(&m_CommandList)));

    // Command lists are created in the recording state, but there is nothing
    // to record yet. The main loop expects it to be closed, so close it now.
    DX12_CALL(m_CommandList->Close());
}
