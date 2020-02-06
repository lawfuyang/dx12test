
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
                // VS/PS Shaders
                boost::hash_combine(finalHash, s.m_VS->GetHash());
                if (s.m_PS)
                    boost::hash_combine(finalHash, s.m_PS->GetHash());

                // Blend States
                boost::hash_combine(finalHash, s.m_BlendStates.AlphaToCoverageEnable);
                boost::hash_combine(finalHash, s.m_BlendStates.IndependentBlendEnable);
                for (uint32_t i = 0; i < s.m_RenderTargets.NumRenderTargets; ++i)
                {
                    boost::hash_combine(finalHash, s.m_BlendStates.RenderTarget[i].BlendEnable);
                    boost::hash_combine(finalHash, s.m_BlendStates.RenderTarget[i].LogicOpEnable);
                    boost::hash_combine(finalHash, s.m_BlendStates.RenderTarget[i].SrcBlend);
                    boost::hash_combine(finalHash, s.m_BlendStates.RenderTarget[i].DestBlend);
                    boost::hash_combine(finalHash, s.m_BlendStates.RenderTarget[i].BlendOp);
                    boost::hash_combine(finalHash, s.m_BlendStates.RenderTarget[i].SrcBlendAlpha);
                    boost::hash_combine(finalHash, s.m_BlendStates.RenderTarget[i].DestBlendAlpha);
                    boost::hash_combine(finalHash, s.m_BlendStates.RenderTarget[i].BlendOpAlpha);
                    boost::hash_combine(finalHash, s.m_BlendStates.RenderTarget[i].LogicOp);
                    boost::hash_combine(finalHash, s.m_BlendStates.RenderTarget[i].RenderTargetWriteMask);
                }

                // Rasterizer States
                boost::hash_combine(finalHash, GenericTypeHash(s.m_RasterizerStates));

                // Depth Stencil State
                boost::hash_combine(finalHash, GenericTypeHash(s.m_DepthStencilStates));

                // Vertex Input Layout
                boost::hash_combine(finalHash, s.m_VertexFormat->GetHash());

                // Primitive Topology
                boost::hash_combine(finalHash, GetD3D12PrimitiveTopologyType(s.m_PrimitiveTopology));

                // Num Render Targets
                boost::hash_combine(finalHash, s.m_RenderTargets.NumRenderTargets);

                // RTV Formats
                for (uint32_t i = 0; i < s.m_RenderTargets.NumRenderTargets; ++i)
                {
                    boost::hash_combine(finalHash, s.m_RenderTargets.RTFormats[i]);
                }

                // DSV Format
                boost::hash_combine(finalHash, s.m_DepthStencilFormat);

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
