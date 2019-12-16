#include "graphic/gfxpipelinestateobject.h"

#include "graphic/dx12utils.h"
#include "graphic/gfxmanager.h"
#include "graphic/gfxdevice.h"

void GfxPSOManager::Initialize()
{
    bbeProfileFunction();

    // Init the memory mapped file.
    const std::wstring cacheDir = utf8_decode(GetTempDirectory() + "D3D12PipelineLibraryCache.cache");
    m_MemoryMappedCacheFile.Init(cacheDir);
}

void GfxPSOManager::ShutDown(bool deleteCacheFile)
{
    bbeProfileFunction();

    //// If we're not going to destroy the file, serialize the library to disk.
    //if (!deleteCacheFile)
    //{
    //    assert(m_PipelineLibrary);

    //    // Important: An ID3D12PipelineLibrary object becomes undefined when the underlying memory, that was used to initalize it, changes.
    //    assert(m_PipelineLibrary->GetSerializedSize() <= UINT_MAX);    // Code below casts to UINT.
    //    const UINT librarySize = static_cast<UINT>(m_PipelineLibrary->GetSerializedSize());
    //    if (librarySize > 0)
    //    {
    //        // Grow the file if needed.
    //        const size_t neededSize = sizeof(UINT) + librarySize;
    //        if (neededSize > m_MemoryMappedCacheFile.GetCurrentFileSize())
    //        {
    //            // The file mapping is going to change thus it will invalidate the ID3D12PipelineLibrary object.
    //            // Serialize the library contents to temporary memory first.
    //            std::vector<BYTE> pTempData;
    //            pTempData.resize(librarySize);

    //            DX12_CALL(m_PipelineLibrary->Serialize(pTempData.data(), librarySize));

    //            // Now it's safe to grow the mapping.
    //            m_MemoryMappedCacheFile.GrowMapping(librarySize);

    //            // Save the size of the library and the library itself.
    //            memcpy(m_MemoryMappedCacheFile.GetData(), pTempData.data(), librarySize);
    //            m_MemoryMappedCacheFile.SetSize(librarySize);
    //        }
    //        else
    //        {
    //            // The mapping didn't change, we can serialize directly to the mapped file.
    //            // Save the size of the library and the library itself.
    //            assert(neededSize <= m_MemoryMappedCacheFile.GetCurrentFileSize());
    //            DX12_CALL(m_PipelineLibrary->Serialize(m_MemoryMappedCacheFile.GetData(), librarySize));
    //            m_MemoryMappedCacheFile.SetSize(librarySize);
    //        }

    //        // m_PipelineLibrary is now undefined because we just wrote to the mapped file, don't use it again.
    //        m_PipelineLibrary.Reset();
    //    }
    //}

    m_MemoryMappedCacheFile.Destroy(deleteCacheFile);
}

ID3D12PipelineState* GfxPSOManager::GetPSO()
{
    return nullptr;
}
