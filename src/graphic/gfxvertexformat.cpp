#include "graphic/gfxvertexformat.h"

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

void GfxVertexInputLayoutManager::Initialize()
{
    bbeProfileFunction();

    static const D3D12_INPUT_ELEMENT_DESC s_Position3f_TexCoord2f_Desc[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    static const D3D12_INPUT_ELEMENT_DESC s_Position3f_Normal3f_TexCoord2f_Desc[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    GfxDefaultVertexFormats::Position3f_TexCoord2f.Initialize(s_Position3f_TexCoord2f_Desc, _countof(s_Position3f_TexCoord2f_Desc));
    GfxDefaultVertexFormats::Position3f_Normal3f_Texcoord2f.Initialize(s_Position3f_Normal3f_TexCoord2f_Desc, _countof(s_Position3f_Normal3f_TexCoord2f_Desc));
}
