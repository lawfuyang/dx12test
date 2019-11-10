#pragma once

class GfxDevice;

class GfxManager
{
    DeclareSingletonFunctions(GfxManager);

public:
    void Initialize();
    void ShutDown();

    void BeginFrame();
    void EndFrame();

    GfxDevice& GetMainGfxDevice() { return *m_MainDevice; }

private:
    std::unique_ptr<GfxDevice> m_MainDevice;
};
