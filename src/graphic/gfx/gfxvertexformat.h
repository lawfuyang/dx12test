#pragma once

class GfxVertexFormat
{
public:
    D3D12_INPUT_LAYOUT_DESC Dev() const { return m_Desc; }
    
    template <uint32_t NumElements>
    void Initialize(const D3D12_INPUT_ELEMENT_DESC(&desc)[NumElements]);

    std::size_t GetHash() const { return m_Hash; }
    bool IsNullLayout() const { return m_Hash == 0; }

    bool operator==(const GfxVertexFormat& rhs) const { return m_Hash == rhs.m_Hash; }

private:
    D3D12_INPUT_LAYOUT_DESC m_Desc = {};
    std::size_t m_Hash = 0;
};

struct GfxDefaultVertexFormats
{
    DeclareSingletonFunctions(GfxDefaultVertexFormats);

    void Initialize();

    inline static GfxVertexFormat Position2f_TexCoord2f_Color4ub;
    inline static GfxVertexFormat Position3f_Color4ub;
    inline static GfxVertexFormat Position3f_TexCoord2f;
    inline static GfxVertexFormat Position3f_TexCoord2f_Color4ub;
    inline static GfxVertexFormat Position3f_Normal3f_Texcoord2f;
    inline static GfxVertexFormat Position3f_Normal3f_Texcoord2f_Tangent3f;
};
#define g_GfxDefaultVertexFormats GfxDefaultVertexFormats::GetInstance()
