
namespace std
{
    template<>
    struct hash<GfxPipelineStateObject>
    {
        std::size_t operator()(const GfxPipelineStateObject& s) const noexcept
        {
            std::size_t finalHash = 0;

            // RootSig
            boost::hash_combine(finalHash, s.m_RootSig->GetHash());

            // Vertex Input Layout
            for (uint32_t i = 0; i < s.m_InputLayout.NumElements; ++i)
            {
                const D3D12_INPUT_ELEMENT_DESC& desc = s.m_InputLayout.pInputElementDescs[i];

                //boost::hash_combine(finalHash, std::string{ desc.SemanticName }); // We did a shallow copy... ptr is invalid
                boost::hash_combine(finalHash, desc.SemanticIndex);
                boost::hash_combine(finalHash, desc.InputSlot);
                boost::hash_combine(finalHash, desc.AlignedByteOffset);
                boost::hash_combine(finalHash, desc.InputSlotClass);
                boost::hash_combine(finalHash, desc.InstanceDataStepRate);
            }

            // Primitive Topology
            boost::hash_combine(finalHash, s.m_PrimitiveTopologyType);

            // Shaders
            if (s.m_VS)
                boost::hash_combine(finalHash, s.m_VS->GetHash());
            if (s.m_PS)
                boost::hash_combine(finalHash, s.m_PS->GetHash());
            if (s.m_CS)
                boost::hash_combine(finalHash, s.m_CS->GetHash());

            // Blend States
            const std::byte* blendStatesRawMem = reinterpret_cast<const std::byte*>(&s.m_BlendStates);
            boost::hash_range(finalHash, blendStatesRawMem, blendStatesRawMem + sizeof(D3D12_BLEND_DESC));

            // Depth Stencil State
            const std::byte* depthStencilStatesRawMem = reinterpret_cast<const std::byte*>(&s.m_DepthStencilStates);
            boost::hash_range(finalHash, depthStencilStatesRawMem, depthStencilStatesRawMem + sizeof(D3D12_DEPTH_STENCIL_DESC1));

            // Depth Stencil Format
            boost::hash_combine(finalHash, s.m_DepthStencilFormat);

            // Rasterizer States
            const std::byte* resterizerStatesRawMem = reinterpret_cast<const std::byte*>(&s.m_RasterizerStates);
            boost::hash_range(finalHash, resterizerStatesRawMem, resterizerStatesRawMem + sizeof(D3D12_RASTERIZER_DESC));

            // Render Targets
            const std::byte* renderTargetsRawMem = reinterpret_cast<const std::byte*>(&s.m_RenderTargets);
            boost::hash_range(finalHash, renderTargetsRawMem, renderTargetsRawMem + sizeof(D3D12_RT_FORMAT_ARRAY));

            // Sample Desccriptors
            const std::byte* sampleDescRawMem = reinterpret_cast<const std::byte*>(&s.m_SampleDescriptors);
            boost::hash_range(finalHash, sampleDescRawMem, sampleDescRawMem + sizeof(DXGI_SAMPLE_DESC));

            return finalHash;
        }
    };
}
