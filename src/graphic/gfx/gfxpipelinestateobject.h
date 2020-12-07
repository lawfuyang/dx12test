#pragma once

#include <system/memorymappedfile.h>

class GfxContext;

class GfxPSOManager
{
public:
    DeclareSingletonFunctions(GfxPSOManager);

    void Initialize();
    void ShutDown();

    ID3D12PipelineState* GetPSO(GfxContext&, CD3DX12_PIPELINE_STATE_STREAM2&, std::size_t psoHash);

private:
    std::shared_mutex m_PipelineLibraryRWLock;
    ComPtr<ID3D12PipelineLibrary1> m_PipelineLibrary;

    MemoryMappedFile m_MemoryMappedCacheFile;

    uint32_t m_NewPSOs = 0;
};
#define g_GfxPSOManager GfxPSOManager::GetInstance()
