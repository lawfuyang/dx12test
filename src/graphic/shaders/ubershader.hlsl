
#include "staticsamplers.hlsl"
#include "commonvertexformats.hlsl"

#include "autogen/hlsl/PerFrameConsts.h"

Texture2D<float4> g_Texture : register(t0);

#if defined(VERTEX_SHADER)
VS_OUT VSMain(VS_IN input, uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
    VS_OUT result = (VS_OUT)0;

#if defined(VERTEX_FORMAT_Position2f_TexCoord2f_Color4ub)
    result.m_Position = mul(float4(input.m_Position, 0, 1), g_ViewProjMatrix);
#elif defined(VERTEX_FORMAT_Position3f_Normal3f_Texcoord2f_Tangent3f)
    result.m_Position = mul(float4(input.m_Position, 1), g_ViewProjMatrix);
#endif

    result.m_TexCoord = input.m_TexCoord;

#if defined(VERTEX_FORMAT_Position3f_Normal3f_Texcoord2f_Tangent3f)
    result.m_Normal = input.m_Normal;
    result.m_Tangent = input.m_Tangent;
#endif

    return result;
}
#endif

#if defined(PIXEL_SHADER)
float4 PSMain(VS_OUT input) : SV_TARGET
{
    float4 diffuse = g_Texture.Sample(g_AnisotropicWrapSampler, input.m_TexCoord);
    return diffuse;
}
#endif
