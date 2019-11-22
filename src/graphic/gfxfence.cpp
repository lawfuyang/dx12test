#include "graphic/gfxfence.h"

#include "graphic/gfxmanager.h"
#include "graphic/dx12utils.h"
#include "graphic/gfxdevice.h"

void GfxFence::Initialize(D3D12_FENCE_FLAGS fenceFlags)
{
    bbeProfileFunction();

    GfxDevice& gfxDevice = GfxManager::GetInstance().GetGfxDevice();

    const uint64_t initialValue = 0;

    // Create synchronization objects.
    DX12_CALL(gfxDevice.Dev()->CreateFence(initialValue, fenceFlags, IID_PPV_ARGS(&m_Fence)));

    // Create an event handle to use for frame synchronization.
    m_FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    bbeAssert(m_FenceEvent, "%s", GetLastErrorAsString().c_str());
}
