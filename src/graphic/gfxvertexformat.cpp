#include "graphic/gfxvertexformat.h"

void GfxVertexInputLayoutManager::Initialize()
{
    bbeProfileFunction();

    const D3D12_INPUT_ELEMENT_DESC Position3f_TexCoord2f_Desc[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };
    gs_Position3f_TexCoord2f = { Position3f_TexCoord2f_Desc, _countof(Position3f_TexCoord2f_Desc) };

    const D3D12_INPUT_ELEMENT_DESC Position3f_Normal3f_TexCoord2f_Desc[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };
    gs_Position3f_Normal3f_Texcoord2f = { Position3f_Normal3f_TexCoord2f_Desc , _countof(Position3f_Normal3f_TexCoord2f_Desc) };
}
