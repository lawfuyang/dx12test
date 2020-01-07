#pragma once

class GfxVertexInputLayoutManager
{
public:
    DeclareSingletonFunctions(GfxVertexInputLayoutManager);

    void Initialize();
};

namespace GfxDefaultVertexFormats
{
    static D3D12_INPUT_LAYOUT_DESC Position3f_TexCoord2f;
    static D3D12_INPUT_LAYOUT_DESC Position3f_Normal3f_Texcoord2f;
}
