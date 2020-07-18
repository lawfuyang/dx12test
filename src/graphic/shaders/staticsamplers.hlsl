
#ifndef __STATICSAMPLERS_HLSL__
#define __STATICSAMPLERS_HLSL__

// Root Signature Samplers
SamplerState g_PointClampSampler       : register(s0, space99);
SamplerState g_PointWrapSampler        : register(s1, space99);
SamplerState g_TrilinearClampSampler   : register(s2, space99);
SamplerState g_TrilinearWrapSampler    : register(s3, space99);
SamplerState g_AnisotropicClampSampler : register(s4, space99);
SamplerState g_AnisotropicWrapSampler  : register(s5, space99);

#endif // #define __STATICSAMPLERS_HLSL__
