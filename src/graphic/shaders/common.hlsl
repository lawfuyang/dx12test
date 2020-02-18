cbuffer TestConstantBuffer : register(b0)
{
    float4 g_TestConstantBufferValue;
    float4 g_TestConstantBufferValue2;
};
// EndShaderDeclarations

// Root Signature Samplers
SamplerState g_PointClampSampler       : register(s0, space99);
SamplerState g_PointWrapSampler        : register(s1, space99);
SamplerState g_TrilinearClampSampler   : register(s2, space99);
SamplerState g_TrilinearWrapSampler    : register(s3, space99);
SamplerState g_AnisotropicClampSampler : register(s4, space99);
SamplerState g_AnisotropicWrapSampler  : register(s5, space99);
