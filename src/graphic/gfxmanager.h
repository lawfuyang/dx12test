#pragma once

#include "graphic/gfxdevice.h"

class GfxManager
{
    DeclareSingletonFunctions(GfxManager);

public:
    void Initialize();
    void ShutDown();

    void BeginFrame();
    void EndFrame();

    GfxDevice& GetMainGfxDevice() { return m_MainDevice; }

private:
    GfxDevice m_MainDevice;
    GfxSwapChain m_SwapChain;
};
