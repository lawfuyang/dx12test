#pragma once

class GfxVertexInputLayoutManager
{
public:
    DeclareSingletonFunctions(GfxVertexInputLayoutManager);

    void Initialize();
};

struct GfxDefaultVertexFormats
{
    inline static D3D12_INPUT_LAYOUT_DESC Position3f_TexCoord2f = {};
    inline static D3D12_INPUT_LAYOUT_DESC Position3f_Normal3f_Texcoord2f = {};
};
