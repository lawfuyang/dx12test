#pragma once

class GfxVertexInputLayoutManager
{
public:
    DeclareSingletonFunctions(GfxVertexInputLayoutManager);

    void Initialize();
};

class GfxVertexFormat
{
public:
    D3D12_INPUT_LAYOUT_DESC Dev() const { return m_Desc; }

    void Initialize(const D3D12_INPUT_ELEMENT_DESC*, uint32_t numElements);
    std::size_t GetHash() const { return m_Hash; }

private:
    D3D12_INPUT_LAYOUT_DESC m_Desc = {};
    std::size_t m_Hash = 0;
};

struct GfxDefaultVertexFormats
{
    inline static GfxVertexFormat Position3f_TexCoord2f;
    inline static GfxVertexFormat Position3f_Normal3f_Texcoord2f;
};
