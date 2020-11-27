#include <graphic/gfx/gfxpipelinestateobject.h>
#include <graphic/pch.h>

void GfxPipelineStateObject::SetRenderTargetFormat(uint32_t idx, DXGI_FORMAT format)
{
    assert(idx < _countof(m_RenderTargets.RTFormats));

    m_RenderTargets.NumRenderTargets = std::max(m_RenderTargets.NumRenderTargets, idx + 1);
    m_RenderTargets.RTFormats[idx] = format;
}

void GfxPSOManager::Initialize()
{
    bbeProfileFunction();

    assert(!m_PipelineLibrary);

    // Init the memory mapped file.
    StaticWString<MAX_PATH> cacheDir = StringUtils::Utf8ToWide(StringFormat("%sD3D12PipelineLibraryCache.cache", GetTempDirectory())).c_str();
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

ID3D12PipelineState* GfxPSOManager::GetGraphicsPSO(const GfxPipelineStateObject& pso)
{
    bbeProfileFunction();

    assert(m_PipelineLibrary);
    assert(pso.m_RootSig && pso.m_RootSig->Dev());
    assert(pso.m_VS);
    assert(pso.m_VS->GetBlob());
    assert(pso.m_VertexFormat);
    assert(pso.m_PrimitiveTopology != D3D_PRIMITIVE_TOPOLOGY_UNDEFINED);

    if (pso.m_PS)
    {
        assert(pso.m_PS->GetBlob());
        assert(pso.m_RenderTargets.NumRenderTargets > 0);
    }

    // Describe and create the Graphics PSO
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = pso.m_RootSig->Dev();
    psoDesc.InputLayout = { pso.m_VertexFormat->Dev().pInputElementDescs, pso.m_VertexFormat->Dev().NumElements };
    psoDesc.VS = CD3DX12_SHADER_BYTECODE{ pso.m_VS->GetBlob() };
    psoDesc.PS = pso.m_PS ? CD3DX12_SHADER_BYTECODE{ pso.m_PS->GetBlob() } : CD3DX12_SHADER_BYTECODE{ nullptr, 0 };
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
    psoDesc.DSVFormat = pso.m_DepthStencilFormat;
    psoDesc.SampleDesc.Count = pso.m_SampleDescriptors.Count;

    return LoadOrSavePSOFromLibrary(pso, psoDesc);
}

ID3D12PipelineState* GfxPSOManager::GetComputePSO(const GfxPipelineStateObject& pso)
{
    bbeProfileFunction();

    assert(m_PipelineLibrary);

    // Describe and create the Compute PSO
    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = pso.m_RootSig->Dev();
    psoDesc.CS = CD3DX12_SHADER_BYTECODE{ pso.m_CS->GetBlob() };

    return LoadOrSavePSOFromLibrary(pso, psoDesc);
}

static void CreatePSOInternal(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc, ID3D12PipelineState*& psoToReturn)
{
    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();
    DX12_CALL(gfxDevice.Dev()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&psoToReturn)));
}

static void CreatePSOInternal(const D3D12_COMPUTE_PIPELINE_STATE_DESC& psoDesc, ID3D12PipelineState*& psoToReturn)
{
    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();
    DX12_CALL(gfxDevice.Dev()->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&psoToReturn)));
}

template <typename PSODesc>
ID3D12PipelineState* GfxPSOManager::LoadOrSavePSOFromLibrary(const GfxPipelineStateObject& pso, const PSODesc& psoDesc)
{
    bbeProfileFunction();

    const StaticWString<32> psoHashStr = std::to_wstring(pso.GetHash()).c_str();

    // TODO: RWLock? Study perf...
    bbeAutoLock(m_PipelineLibraryLock);

    ID3D12PipelineState* psoToReturn = nullptr;
    if (!LoadPSOInternal(psoHashStr.c_str(), psoDesc, psoToReturn))
    {
        bbeProfile("Create & Save PSO");
        g_Log.info("Creating & Storing new PSO '{}' into PipelineLibrary", pso.GetHash());

        // A PSO with the specified name doesn’t exist, or the input desc doesn’t match the data in the library. Store it in the library for next time.
        CreatePSOInternal(psoDesc, psoToReturn);
        

        DX12_CALL(m_PipelineLibrary->StorePipeline(psoHashStr.c_str(), psoToReturn));

        ++m_NewPSOs;
    }

    assert(psoToReturn);
    return psoToReturn;
}
template ID3D12PipelineState* GfxPSOManager::LoadOrSavePSOFromLibrary(const GfxPipelineStateObject&, const D3D12_GRAPHICS_PIPELINE_STATE_DESC&);
template ID3D12PipelineState* GfxPSOManager::LoadOrSavePSOFromLibrary(const GfxPipelineStateObject&, const D3D12_COMPUTE_PIPELINE_STATE_DESC&);
