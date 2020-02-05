#include "graphic/gfxpipelinestateobject.h"

#include "graphic/dx12utils.h"
#include "graphic/gfxmanager.h"
#include "graphic/gfxdevice.h"
#include "graphic/gfxrootsignature.h"
#include "graphic/gfxvertexformat.h"
#include "graphic/gfxshadermanager.h"

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
        g_Log.warn("Either GPU or Display Driver has changed... PSO cache is now invalid");
        m_MemoryMappedCacheFile.Destroy(true);
        m_MemoryMappedCacheFile.Init(cacheDir);
        DX12_CALL(gfxDevice.Dev()->CreatePipelineLibrary(m_MemoryMappedCacheFile.GetData(), m_MemoryMappedCacheFile.GetSize(), IID_PPV_ARGS(&m_PipelineLibrary)));
    }

    assert(m_PipelineLibrary);
    SetD3DDebugName(m_PipelineLibrary.Get(), "GfxPSOManager ID3D12PipelineLibrary");
}

void GfxPSOManager::ShutDown()
{
    bbeProfileFunction();

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
    }

    const bool ForceDeleteCacheFile = true; // Investigate weird gfx init crashes when GfxPSOManager loads from file.
    m_MemoryMappedCacheFile.Destroy(ForceDeleteCacheFile);
    m_PipelineLibrary.Reset();
}

ID3D12PipelineState* GfxPSOManager::GetGraphicsPSO(const GfxPipelineStateObject& pso)
{
    bbeProfileFunction();

    assert(m_PipelineLibrary);

    const std::size_t psoHash = std::hash<GfxPipelineStateObject>{}(pso);
    const std::wstring psoHashStr = std::to_wstring(psoHash);

    GfxDevice& gfxDevice = GfxManager::GetInstance().GetGfxDevice();

    // Describe and create the Graphics PSO
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = pso.m_RootSig->Dev();
    psoDesc.InputLayout = { pso.m_VertexFormat->Dev().pInputElementDescs, pso.m_VertexFormat->Dev().NumElements };
    psoDesc.VS = CD3DX12_SHADER_BYTECODE{ pso.m_VS->GetBlob() };
    psoDesc.PS = CD3DX12_SHADER_BYTECODE{ pso.m_PS->GetBlob() };
    psoDesc.RasterizerState = pso.m_RasterizerStates;
    psoDesc.BlendState = pso.m_BlendStates;
    psoDesc.DepthStencilState = pso.m_DepthStencilStates;
    psoDesc.SampleMask = DefaultSampleMask{};
    psoDesc.PrimitiveTopologyType = GetD3D12PrimitiveTopologyType(pso.m_PrimitiveTopology);
    psoDesc.NumRenderTargets = pso.m_RenderTargets.NumRenderTargets;
    for (uint32_t i = 0; i < pso.m_RenderTargets.NumRenderTargets; ++i)
    {
        psoDesc.RTVFormats[i] = pso.m_RenderTargets.RTFormats[i];
    }
    psoDesc.SampleDesc.Count = pso.m_SampleDescriptors.Count;

    ID3D12PipelineState* psoToReturn = nullptr;
    if (const ::HRESULT hr = m_PipelineLibrary->LoadGraphicsPipeline(psoHashStr.c_str(), &psoDesc, IID_PPV_ARGS(&psoToReturn));
        hr == E_INVALIDARG)
    {
        bbeProfile("ID3D12PipelineLibrary::CreateGraphicsPipelineState");

        g_Log.info("Graphics PSO '{}' not found! Creating new PSO.", utf8_encode(psoHashStr));

        // A PSO with the specified name doesn’t exist, or the input desc doesn’t match the data in the library. Store it in the library for next time.
        DX12_CALL(gfxDevice.Dev()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&psoToReturn)));
        SavePSOToPipelineLibrary(psoToReturn, psoHashStr);
    }

    assert(psoToReturn);
    return psoToReturn;
}

ID3D12PipelineState* GfxPSOManager::GetComputePSO(const GfxPipelineStateObject& pso)
{
    bbeProfileFunction();

    assert(m_PipelineLibrary);

    const std::size_t psoHash = std::hash<GfxPipelineStateObject>{}(pso);
    const std::wstring psoHashStr = std::to_wstring(psoHash);

    GfxDevice& gfxDevice = GfxManager::GetInstance().GetGfxDevice();

    // Describe and create the Compute PSO
    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = pso.m_RootSig->Dev();
    psoDesc.CS = CD3DX12_SHADER_BYTECODE{ pso.m_CS->GetBlob() };

    ID3D12PipelineState* psoToReturn = nullptr;
    if (const ::HRESULT hr = m_PipelineLibrary->LoadComputePipeline(psoHashStr.c_str(), &psoDesc, IID_PPV_ARGS(&psoToReturn));
        hr == E_INVALIDARG)
    {
        bbeProfile("ID3D12PipelineLibrary::CreateComputePipelineState");

        g_Log.info("Compute PSO '{}' not found! Creating new PSO.", utf8_encode(psoHashStr));

        // A PSO with the specified name doesn’t exist, or the input desc doesn’t match the data in the library. Store it in the library for next time.
        DX12_CALL(gfxDevice.Dev()->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&psoToReturn)));
        SavePSOToPipelineLibrary(psoToReturn, psoHashStr);
    }

    assert(psoToReturn);
    return psoToReturn;
}

void GfxPSOManager::SavePSOToPipelineLibrary(ID3D12PipelineState* pso, const std::wstring& psoHashStr)
{
    bbeMultiThreadDetector();

    bbeProfileFunction();

    g_Log.info("Storing new PSO '{}' into PipelineLibrary", utf8_encode(psoHashStr));
    DX12_CALL(m_PipelineLibrary->StorePipeline(psoHashStr.c_str(), pso));
}

void GfxPipelineStateObject::SetRenderTargetFormat(uint32_t idx, DXGI_FORMAT format)
{
    assert(idx < _countof(m_RenderTargets.RTFormats));
    
    m_RenderTargets.NumRenderTargets = std::max(m_RenderTargets.NumRenderTargets, idx + 1);
    m_RenderTargets.RTFormats[idx] = format;
}
