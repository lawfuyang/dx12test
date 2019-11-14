#include "graphic/gfxfence.h"

#include "graphic/dx12utils.h"
#include "graphic/gfxdevice.h"

void GfxFence::Initialize(GfxDevice& gfxDevice, D3D12_FENCE_FLAGS fenceFlags)
{
    bbeProfileFunction();

    const uint64_t initialValue = 0;

    // Create synchronization objects.
    DX12_CALL(gfxDevice.Dev()->CreateFence(initialValue, fenceFlags, IID_PPV_ARGS(&m_Fence)));

    // Create an event handle to use for frame synchronization.
    m_FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (m_FenceEvent == nullptr)
    {
        DX12_CALL(HRESULT_FROM_WIN32(GetLastError()));
    }
}
