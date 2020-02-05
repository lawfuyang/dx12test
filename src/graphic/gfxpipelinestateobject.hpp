
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

            if (s.m_VS)
            {
                // Vertex Input Layout
                boost::hash_combine(finalHash, s.m_VertexFormat->GetHash());

                // Primitive Topology
                boost::hash_combine(finalHash, s.m_PrimitiveTopology);

                // VS/PS Shaders
                boost::hash_combine(finalHash, s.m_VS->GetHash());
                if (s.m_PS)
                    boost::hash_combine(finalHash, s.m_PS->GetHash());

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
            }
            else if (s.m_CS)
            {
                // We only need to hash CS shader
                boost::hash_combine(finalHash, s.m_CS->GetHash());
            }

            return finalHash;
        }
    };
}
