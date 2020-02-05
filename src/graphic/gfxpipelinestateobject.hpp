
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
                boost::hash_combine(finalHash, GenericTypeHash(s.m_BlendStates));

                // Depth Stencil State
                boost::hash_combine(finalHash, GenericTypeHash(s.m_DepthStencilStates));

                // Depth Stencil Format
                boost::hash_combine(finalHash, s.m_DepthStencilFormat);

                // Rasterizer States
                boost::hash_combine(finalHash, GenericTypeHash(s.m_RasterizerStates));

                // Render Targets
                boost::hash_combine(finalHash, GenericTypeHash(s.m_RenderTargets));

                // Sample Desccriptors
                boost::hash_combine(finalHash, GenericTypeHash(s.m_SampleDescriptors));
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
