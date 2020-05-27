#include "graphic/gfx/gfxpipelinestateobject.h"

#include "graphic/dx12utils.h"
#include "graphic/gfx/gfxmanager.h"
#include "graphic/gfx/gfxdevice.h"
#include "graphic/gfx/gfxrootsignature.h"
#include "graphic/gfx/gfxvertexformat.h"
#include "graphic/gfx/gfxshadermanager.h"

void GfxPSOManager::Initialize()
{
    bbeProfileFunction();

    assert(!m_PipelineLibrary);

    // Init the memory mapped file.
    StaticWString<MAX_PATH> cacheDir = MakeWStrFromStr(GetTempDirectory() + "D3D12PipelineLibraryCache.cache").c_str();
    m_MemoryMappedCacheFile.Init(cacheDir.c_str());

    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();

    // Create a Pipeline Library from the serialized blob.
    const HRESULT hr = gfxDevice.Dev()->CreatePipelineLibrary(m_MemoryMappedCacheFile.GetData(), m_MemoryMappedCacheFile.GetSize(), IID_PPV_ARGS(&m_PipelineLibrary));
    switch (hr)
    {
    case DXGI_ERROR_UNSUPPORTED:
        g_Log.critical("The driver doesn't support Pipeline libraries. WDDM2.1 drivers must support it");
        assert(false);

    case E_INVALIDARG: // The provided Library is corrupted or unrecognized.
    case D3D12_ERROR_ADAPTER_NOT_FOUND:
        g_Log.info("The provided Library contains data for different hardware (Don't really need to clear the cache, could have a cache per adapter)");
    case D3D12_ERROR_DRIVER_VERSION_MISMATCH:
        g_Log.info("The provided Library contains data from an old driver or runtime. We need to re-create it");
        m_MemoryMappedCacheFile.Destroy(true);
        m_MemoryMappedCacheFile.Init(cacheDir.c_str());
        DX12_CALL(gfxDevice.Dev()->CreatePipelineLibrary(m_MemoryMappedCacheFile.GetData(), m_MemoryMappedCacheFile.GetSize(), IID_PPV_ARGS(&m_PipelineLibrary)));
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
    assert(pso.m_RootSig);
    assert(pso.m_VertexFormat);
    assert(pso.m_VS);
    assert(pso.m_VS->GetBlob());
    assert(pso.m_PS ? pso.m_PS->GetBlob() != nullptr : true);
    assert(pso.m_PrimitiveTopology != D3D_PRIMITIVE_TOPOLOGY_UNDEFINED);
    assert(pso.m_RenderTargets.NumRenderTargets > 0);

    const std::size_t psoHash = std::hash<GfxPipelineStateObject>{}(pso);
    const std::wstring psoHashStr = std::to_wstring(psoHash);

    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();

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

    ID3D12PipelineState* psoToReturn = nullptr;
    if (const ::HRESULT hr = m_PipelineLibrary->LoadGraphicsPipeline(psoHashStr.c_str(), &psoDesc, IID_PPV_ARGS(&psoToReturn));
        hr == E_INVALIDARG)
    {
        bbeProfile("ID3D12PipelineLibrary::CreateGraphicsPipelineState");

        g_Log.info("Graphics PSO '{0:X}' not found! Creating new PSO.", psoHash);

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

    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();

    // Describe and create the Compute PSO
    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = pso.m_RootSig->Dev();
    psoDesc.CS = CD3DX12_SHADER_BYTECODE{ pso.m_CS->GetBlob() };

    ID3D12PipelineState* psoToReturn = nullptr;
    if (const ::HRESULT hr = m_PipelineLibrary->LoadComputePipeline(psoHashStr.c_str(), &psoDesc, IID_PPV_ARGS(&psoToReturn));
        hr == E_INVALIDARG)
    {
        bbeProfile("ID3D12PipelineLibrary::CreateComputePipelineState");

        g_Log.info("Compute PSO '{0:X}' not found! Creating new PSO.", psoHash);

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

    g_Log.info("Storing new PSO '{0:X}' into PipelineLibrary", std::stoull(psoHashStr));
    DX12_CALL(m_PipelineLibrary->StorePipeline(psoHashStr.c_str(), pso));

    ++m_NewPSOs;
}

void GfxPipelineStateObject::SetRenderTargetFormat(uint32_t idx, DXGI_FORMAT format)
{
    assert(idx < _countof(m_RenderTargets.RTFormats));
    
    m_RenderTargets.NumRenderTargets = std::max(m_RenderTargets.NumRenderTargets, idx + 1);
    m_RenderTargets.RTFormats[idx] = format;
}
