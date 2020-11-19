
#ifndef __COMMONVERTEXFORMATS_HLSL__
#define __COMMONVERTEXFORMATS_HLSL__

struct VS_IN
{
#if defined(VERTEX_FORMAT_Position3f_Normal3f_Texcoord2f)
    float3 m_Position : POSITION;
    float3 m_Normal   : NORMAL;
    float2 m_TexCoord : TEXCOORD;
#elif defined(VERTEX_FORMAT_Position3f_Normal3f_Texcoord2f_Tangent3f)
    float3 m_Position : POSITION;
    float3 m_Normal   : NORMAL;
    float2 m_TexCoord : TEXCOORD;
    float3 m_Tangent  : TANGENT;
#else

    // default 2D vertex format (IMGUI)
    float2 m_Position : POSITION;
    float2 m_TexCoord : TEXCOORD;
    float4 m_Color    : COLOR;
#endif
};

#endif // #define __COMMONVERTEXFORMATS_HLSL__
