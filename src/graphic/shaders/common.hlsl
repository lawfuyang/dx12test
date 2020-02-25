cbuffer FrameParams : register(b0)
{
    uint g_FrameNumber;
    float g_CurrentFrameTime;
    float g_PreviousFrameTime;
};
// EndShaderDeclarations

// Root Signature Samplers
SamplerState g_PointClampSampler       : register(s0, space99);
SamplerState g_PointWrapSampler        : register(s1, space99);
SamplerState g_TrilinearClampSampler   : register(s2, space99);
SamplerState g_TrilinearWrapSampler    : register(s3, space99);
SamplerState g_AnisotropicClampSampler : register(s4, space99);
SamplerState g_AnisotropicWrapSampler  : register(s5, space99);
