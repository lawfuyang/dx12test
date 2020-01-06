#pragma once

class GfxVertexInputLayoutManager
{
public:
    DeclareSingletonFunctions(GfxVertexInputLayoutManager);

    void Initialize();
};
static D3D12_INPUT_LAYOUT_DESC gs_Position3f_TexCoord2f;
static D3D12_INPUT_LAYOUT_DESC gs_Position3f_Normal3f_Texcoord2f;
