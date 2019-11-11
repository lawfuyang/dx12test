#include "graphic/gfxcommandqueue.h"

#include "graphic/dx12utils.h"
#include "graphic/gfxdevice.h"

void GfxCommandQueue::Initialize(GfxDevice* gfxDevice, D3D12_COMMAND_LIST_TYPE queueType)
{
    bbeProfileFunction();

    bbeAssert(m_OwnerDevice == nullptr, "");
    m_OwnerDevice = gfxDevice;

    // Describe and create the command queue.
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = queueType;

    DX12_CALL(m_OwnerDevice->Dev()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_CommandQueue)));
}
