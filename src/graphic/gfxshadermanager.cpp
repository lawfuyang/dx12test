#include "graphic/gfxshadermanager.h"

#include "graphic/dx12utils.h"

#include "tmp/shaders/shaderheadersautogen.h"

void GfxShader::Initialize(const unsigned char* byteCode, uint32_t byteCodeSize)
{
    assert(byteCode);
    assert(byteCodeSize > 0);

    DX12_CALL(D3DCreateBlob(byteCodeSize, &m_ShaderBlob));
}

void GfxShaderManager::Initialize()
{
    bbeProfileFunction();

    // TOOD: Parallelize if needed
    for (const auto& shaderNameByteCodePair : gs_AllShadersByteCode)
    {
        const AllShadersNames shaderName = shaderNameByteCodePair.first;
        const uint32_t shaderIdx = (uint32_t)shaderName;

        m_AllShaders[shaderIdx].Initialize(shaderNameByteCodePair.second, sizeof(shaderNameByteCodePair.second));
    }
}
