
struct VS_IN
{
#if defined(VERTEX_FORMAT_Position2f_TexCoord2f_Color4ub)
    float2 m_Position : POSITION;
    float2 m_TexCoord : TEXCOORD;
    float4 m_Color    : COLOR;
#endif

#if defined(VERTEX_FORMAT_Position3f_Normal3f_Texcoord2f_Tangent3f)
    float3 m_Position : POSITION;
    float3 m_Normal   : NORMAL;
    float2 m_TexCoord : TEXCOORD;
    float3 m_Tangent  : TANGENT;
#endif
};


struct VS_OUT
{
#if defined(VERTEX_FORMAT_Position2f_TexCoord2f_Color4ub)
    float4 m_Position : SV_POSITION;
    float2 m_TexCoord : TEXCOORD;
    float4 m_Color    : COLOR;
#endif

#if defined(VERTEX_FORMAT_Position3f_Normal3f_Texcoord2f_Tangent3f)
    float4 m_Position : SV_POSITION;
    float3 m_Normal   : NORMAL;
    float2 m_TexCoord : TEXCOORD;
    float3 m_Tangent  : TANGENT;
#endif
};
