#pragma once

#include "tmp/shaders/shadersenumsautogen.h"

class GfxShader
{
public:
    ID3DBlob* GetBlob() const { return m_ShaderBlob.Get(); }

    void Initialize(const unsigned char* byteCode, uint32_t byteCodeSize);

protected:
    ComPtr<ID3DBlob> m_ShaderBlob;
};

class GfxShaderManager
{
public:
    DeclareSingletonFunctions(GfxShaderManager);

    void Initialize();
    
    GfxShader& GetShader(AllShaders shaderName) { return m_AllShaders[static_cast<uint32_t>(shaderName)]; }

private:
    std::array<GfxShader, g_NumShaders> m_AllShaders;
};
