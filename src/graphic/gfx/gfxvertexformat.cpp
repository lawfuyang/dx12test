#include "graphic/gfx/gfxvertexformat.h"

#include <graphic/dx12utils.h>

void GfxVertexFormat::Initialize(const D3D12_INPUT_ELEMENT_DESC* desc, uint32_t numElements)
{
    assert(m_Hash == 0);

    m_Desc.NumElements = numElements;
    m_Desc.pInputElementDescs = desc;

    for (uint32_t i = 0; i < numElements; ++i)
    {
        boost::hash_combine(m_Hash, std::string{ desc->SemanticName });
        boost::hash_combine(m_Hash, desc->SemanticIndex);
        boost::hash_combine(m_Hash, desc->InputSlot);
        boost::hash_combine(m_Hash, desc->AlignedByteOffset);
        boost::hash_combine(m_Hash, desc->InputSlotClass);
        boost::hash_combine(m_Hash, desc->InstanceDataStepRate);
    }
}

void GfxDefaultVertexFormats::Initialize()
{
    bbeProfileFunction();

    static const D3D12_INPUT_ELEMENT_DESC s_Position3f_Color4ub_Desc[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    static const D3D12_INPUT_ELEMENT_DESC s_Position2f_TexCoord2f_Color4ub_Desc[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    static const D3D12_INPUT_ELEMENT_DESC s_Position3f_TexCoord2f_Desc[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    static const D3D12_INPUT_ELEMENT_DESC s_Position3f_TexCoord2f_Color4ub_Desc[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    static const D3D12_INPUT_ELEMENT_DESC s_Position3f_Normal3f_TexCoord2f_Desc[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    static const D3D12_INPUT_ELEMENT_DESC s_Position3f_Normal3f_Texcoord2f_Tangent3f_Desc[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    GfxDefaultVertexFormats::Position3f_Color4ub.Initialize(s_Position3f_Color4ub_Desc, _countof(s_Position3f_Color4ub_Desc));
    GfxDefaultVertexFormats::Position2f_TexCoord2f_Color4ub.Initialize(s_Position2f_TexCoord2f_Color4ub_Desc, _countof(s_Position2f_TexCoord2f_Color4ub_Desc));
    GfxDefaultVertexFormats::Position3f_TexCoord2f.Initialize(s_Position3f_TexCoord2f_Desc, _countof(s_Position3f_TexCoord2f_Desc));
    GfxDefaultVertexFormats::Position3f_TexCoord2f_Color4ub.Initialize(s_Position3f_TexCoord2f_Color4ub_Desc, _countof(s_Position3f_TexCoord2f_Color4ub_Desc));
    GfxDefaultVertexFormats::Position3f_Normal3f_Texcoord2f.Initialize(s_Position3f_Normal3f_TexCoord2f_Desc, _countof(s_Position3f_Normal3f_TexCoord2f_Desc));
    GfxDefaultVertexFormats::Position3f_Normal3f_Texcoord2f_Tangent3f.Initialize(s_Position3f_Normal3f_Texcoord2f_Tangent3f_Desc, _countof(s_Position3f_Normal3f_Texcoord2f_Tangent3f_Desc));
}