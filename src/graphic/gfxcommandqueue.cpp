#include "graphic/gfxcommandqueue.h"

#include "graphic/dx12utils.h"
#include "graphic/gfxdevice.h"

void GfxCommandQueue::Initialize(GfxDevice* gfxDevice, D3D12_COMMAND_LIST_TYPE queueType, D3D12_COMMAND_QUEUE_FLAGS flags)
{
    bbeProfileFunction();

    bbeAssert(gfxDevice, "");
    bbeAssert(m_CommandQueue.Get() == nullptr, "");

    m_OwnerDevice = gfxDevice;

    //D3D12_COMMAND_LIST_TYPE   Type;
    //INT                       Priority;
    //D3D12_COMMAND_QUEUE_FLAGS Flags;
    //UINT                      NodeMask;

    // Describe and create the command queue.
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = queueType;
    queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    queueDesc.Flags = flags;
    queueDesc.NodeMask = 0; // For single-adapter, set to 0. Else, set a bit to identify the node

    DX12_CALL(m_OwnerDevice->Dev()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_CommandQueue)));
}
