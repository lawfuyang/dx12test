#include <graphic/gfx/gfxpipelinestateobject.h>
#include <graphic/pch.h>

void GfxPSOManager::Initialize()
{
    bbeProfileFunction();

    assert(!m_PipelineLibrary);

    // Init the memory mapped file.
    StaticWString<MAX_PATH> cacheDir = StringUtils::Utf8ToWide(StringFormat("%sD3D12PipelineLibraryCache.cache", GetTempDirectory()));
    m_MemoryMappedCacheFile.Init(cacheDir.c_str());

    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();

    // Create a Pipeline Library from the serialized blob.
    HRESULT hr = gfxDevice.Dev()->CreatePipelineLibrary(m_MemoryMappedCacheFile.GetData(), m_MemoryMappedCacheFile.GetSize(), IID_PPV_ARGS(&m_PipelineLibrary));
    switch (hr)
    {
    case DXGI_ERROR_UNSUPPORTED:
        g_Log.critical("The driver doesn't support Pipeline libraries. WDDM2.1 drivers must support it");
        break;

    case E_INVALIDARG: // The provided Library is corrupted or unrecognized.
    case D3D12_ERROR_ADAPTER_NOT_FOUND:
        g_Log.critical("The provided Library contains data for different hardware (Don't really need to clear the cache, could have a cache per adapter)");
        break;

    case D3D12_ERROR_DRIVER_VERSION_MISMATCH:
        g_Log.info("The provided Library contains data from an old driver or runtime. We need to re-create it");
        m_MemoryMappedCacheFile.Destroy(true);
        m_MemoryMappedCacheFile.Init(cacheDir.c_str());
        hr = gfxDevice.Dev()->CreatePipelineLibrary(m_MemoryMappedCacheFile.GetData(), m_MemoryMappedCacheFile.GetSize(), IID_PPV_ARGS(&m_PipelineLibrary));
    }

    assert(m_PipelineLibrary);
    SetD3DDebugName(m_PipelineLibrary.Get(), "GfxPSOManager ID3D12PipelineLibrary");

    g_Log.info("GfxPSOManager Initialized");
}

void GfxPSOManager::ShutDown()
{
    bbeProfileFunction();

    assert(m_PipelineLibrary);

    // Important: An ID3D12PipelineLibrary object becomes undefined when the underlying memory, that was used to initalize it, changes.
    assert(m_PipelineLibrary->GetSerializedSize() <= UINT_MAX);    // Code below casts to UINT.
    const UINT librarySize = static_cast<UINT>(m_PipelineLibrary->GetSerializedSize());
    if (m_NewPSOs > 0 && librarySize > 0)
    {
        g_Log.info("Saving '{}' new PSOs into PipelineLibrary", m_NewPSOs);

        // Grow the file if needed.
        const size_t neededSize = sizeof(UINT) + librarySize;
        if (neededSize > m_MemoryMappedCacheFile.GetCurrentFileSize())
        {
            // The file mapping is going to change thus it will invalidate the ID3D12PipelineLibrary object.
            // Serialize the library contents to temporary memory first.
            std::vector<BYTE> pTempData;
            pTempData.resize(librarySize);

            DX12_CALL(m_PipelineLibrary->Serialize(pTempData.data(), librarySize));

            // Now it's safe to grow the mapping.
            m_MemoryMappedCacheFile.GrowMapping(librarySize);

            // Save the size of the library and the library itself.
            memcpy(m_MemoryMappedCacheFile.GetData(), pTempData.data(), librarySize);
            m_MemoryMappedCacheFile.SetSize(librarySize);
        }
        else
        {
            // The mapping didn't change, we can serialize directly to the mapped file.
            // Save the size of the library and the library itself.
            assert(neededSize <= m_MemoryMappedCacheFile.GetCurrentFileSize());
            DX12_CALL(m_PipelineLibrary->Serialize(m_MemoryMappedCacheFile.GetData(), librarySize));
            m_MemoryMappedCacheFile.SetSize(librarySize);
        }
    }
    else
    {
        g_Log.info("No PSOs required to be saved to cache file");
    }

    const bool forceDeleteCacheFile = g_CommandLineOptions.m_PIXCapture;
    m_MemoryMappedCacheFile.Destroy(forceDeleteCacheFile);
    m_PipelineLibrary.Reset();
}

//template <typename DescType, typename PipelineLoader, typename PipelineCreator>
//ID3D12PipelineState* GfxPSOManager::GetPSOInternal(GfxContext& context, DescType&& desc, std::size_t psoHash, PipelineLoader&& loaderFunc, PipelineCreator&& creatorFunc)
//{
//    bbeProfileFunction();
//
//    assert(m_PipelineLibrary);
//
//    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();
//
//    const StaticWString<32> psoHashStr = std::to_wstring(psoHash).c_str();
//    ID3D12PipelineState* psoToReturn = nullptr;
//
//    bbeAutoLockRead(m_PipelineLibraryRWLock);
//    ::HRESULT result = loaderFunc(psoHashStr.data(), std::forward<DescType>(desc), psoToReturn);
//    if (result == E_INVALIDARG)
//    {
//        bbeProfile("Create & Save PSO");
//        g_Log.info("Creating & Storing new PSO '{0:X}' into PipelineLibrary", psoHash);
//
//        creatorFunc(std::forward<DescType>(desc), psoToReturn);
//
//        bbeAutoLockScopedRWUpgrade(m_PipelineLibraryRWLock);
//        DX12_CALL(m_PipelineLibrary->StorePipeline(psoHashStr.c_str(), psoToReturn));
//        ++m_NewPSOs;
//    }
//    else if (FAILED(result))
//    {
//        assert(false);
//    }
//
//    assert(psoToReturn);
//    return psoToReturn;
//}

template <typename DescType, typename D3D12PSOLoaderFunc, typename D3D12PSOCreationFunc>
ID3D12PipelineState* GfxPSOManager::GetPSOInternal(GfxContext& context, DescType&& desc, std::size_t psoHash, D3D12PSOLoaderFunc loaderFunc, D3D12PSOCreationFunc creationFunc)
{
    bbeProfileFunction();

    assert(m_PipelineLibrary);

    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();

    const StaticWString<32> psoHashStr = std::to_wstring(psoHash).c_str();
    ID3D12PipelineState* psoToReturn = nullptr;

    bbeAutoLockRead(m_PipelineLibraryRWLock);
    ::HRESULT result = (m_PipelineLibrary.Get()->*loaderFunc)(psoHashStr.data(), &desc, IID_PPV_ARGS(&psoToReturn));
    if (result == E_INVALIDARG)
    {
        bbeProfile("Create & Save PSO");
        g_Log.info("Creating & Storing new PSO '{0:X}' into PipelineLibrary", psoHash);

        DX12_CALL((gfxDevice.Dev()->*creationFunc)(&desc, IID_PPV_ARGS(&psoToReturn)));

        bbeAutoLockScopedRWUpgrade(m_PipelineLibraryRWLock);
        DX12_CALL(m_PipelineLibrary->StorePipeline(psoHashStr.c_str(), psoToReturn));
        ++m_NewPSOs;
    }
    else if (FAILED(result))
    {
        assert(false);
    }

    assert(psoToReturn);
    return psoToReturn;
}

ID3D12PipelineState* GfxPSOManager::GetPSO(GfxContext& gfxContext, D3D12_GRAPHICS_PIPELINE_STATE_DESC&& desc, std::size_t psoHash)
{
    return GetPSOInternal(gfxContext, std::forward<D3D12_GRAPHICS_PIPELINE_STATE_DESC>(desc), psoHash, &D3D12PipelineLibrary::LoadGraphicsPipeline, &D3D12Device::CreateGraphicsPipelineState);
}

ID3D12PipelineState* GfxPSOManager::GetPSO(GfxContext& gfxContext, D3D12_COMPUTE_PIPELINE_STATE_DESC&& desc, std::size_t psoHash)
{
    return GetPSOInternal(gfxContext, std::forward<D3D12_COMPUTE_PIPELINE_STATE_DESC>(desc), psoHash, &D3D12PipelineLibrary::LoadComputePipeline, &D3D12Device::CreateComputePipelineState);
}
