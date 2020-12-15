#pragma once

#include <system/memorymappedfile.h>

class GfxContext;

class GfxPSOManager
{
public:
    DeclareSingletonFunctions(GfxPSOManager);

    void Initialize();
    void ShutDown();

    ID3D12PipelineState* GetPSO(GfxContext& context, D3D12_GRAPHICS_PIPELINE_STATE_DESC&& desc, std::size_t psoHash);
    ID3D12PipelineState* GetPSO(GfxContext& context, D3D12_COMPUTE_PIPELINE_STATE_DESC&& desc, std::size_t psoHash);

private:
    template <typename DescType, typename D3D12PSOLoaderFunc, typename D3D12PSOCreationFunc>
    ID3D12PipelineState* GetPSOInternal(GfxContext& context, DescType&& desc, std::size_t psoHash, D3D12PSOLoaderFunc loaderFunc, D3D12PSOCreationFunc creatorFunc);

    std::shared_mutex m_PipelineLibraryRWLock;
    ComPtr<D3D12PipelineLibrary> m_PipelineLibrary;

    MemoryMappedFile m_MemoryMappedCacheFile;

    uint32_t m_NewPSOs = 0;
};
#define g_GfxPSOManager GfxPSOManager::GetInstance()
