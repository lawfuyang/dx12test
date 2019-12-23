#include "graphic/gfxpipelinestateobject.h"

#include "graphic/dx12utils.h"
#include "graphic/gfxmanager.h"
#include "graphic/gfxdevice.h"
#include "graphic/gfxrootsignature.h"

void GfxPSOManager::Initialize()
{
    bbeProfileFunction();

    assert(!m_PipelineLibrary);

    // Init the memory mapped file.
    const std::wstring cacheDir = utf8_decode(GetTempDirectory() + "D3D12PipelineLibraryCache.cache");
    m_MemoryMappedCacheFile.Init(cacheDir);

    GfxDevice& gfxDevice = GfxManager::GetInstance().GetGfxDevice();

    // Create a Pipeline Library from the serialized blob.
    const HRESULT hr = gfxDevice.Dev()->CreatePipelineLibrary(m_MemoryMappedCacheFile.GetData(), m_MemoryMappedCacheFile.GetSize(), IID_PPV_ARGS(&m_PipelineLibrary));
    switch (hr)
    {
    case DXGI_ERROR_UNSUPPORTED: // The driver doesn't support Pipeline libraries. WDDM2.1 drivers must support it.
        assert(false);

    case E_INVALIDARG: // The provided Library is corrupted or unrecognized.
    case D3D12_ERROR_ADAPTER_NOT_FOUND: // The provided Library contains data for different hardware (Don't really need to clear the cache, could have a cache per adapter).
    case D3D12_ERROR_DRIVER_VERSION_MISMATCH: // The provided Library contains data from an old driver or runtime. We need to re-create it.
        m_MemoryMappedCacheFile.Destroy(true);
        m_MemoryMappedCacheFile.Init(cacheDir);
        DX12_CALL(gfxDevice.Dev()->CreatePipelineLibrary(m_MemoryMappedCacheFile.GetData(), m_MemoryMappedCacheFile.GetSize(), IID_PPV_ARGS(&m_PipelineLibrary)));
    }

    assert(m_PipelineLibrary);
    SetD3DDebugName(m_PipelineLibrary.Get(), "GfxPSOManager ID3D12PipelineLibrary");
}

void GfxPSOManager::ShutDown(bool deleteCacheFile)
{
    bbeProfileFunction();

    // If we're not going to destroy the file, serialize the library to disk.
    if (!deleteCacheFile)
    {
        assert(m_PipelineLibrary);

        // Important: An ID3D12PipelineLibrary object becomes undefined when the underlying memory, that was used to initalize it, changes.
        assert(m_PipelineLibrary->GetSerializedSize() <= UINT_MAX);    // Code below casts to UINT.
        const UINT librarySize = static_cast<UINT>(m_PipelineLibrary->GetSerializedSize());
        if (librarySize > 0)
        {
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

            // m_PipelineLibrary is now undefined because we just wrote to the mapped file, don't use it again.
        }
    }

    m_MemoryMappedCacheFile.Destroy(deleteCacheFile);
    m_PipelineLibrary.Reset();
}

ID3D12PipelineState* GfxPSOManager::GetPSO()
{
    return nullptr;
}

void GfxPipelineStateObject::SetRootSignature(const GfxRootSignature* rootSig)
{
    assert(rootSig);
    m_RootSig = rootSig->Dev();
}
