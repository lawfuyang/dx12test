#pragma once

class GfxShader
{
    ID3DBlob* GetBlob() const { return m_ShaderBlob.Get(); }

protected:
    ComPtr<ID3DBlob> m_ShaderBlob;
};

class GfxVertexShader : public GfxShader
{

};

class GfxPixelShader : public GfxShader
{

};
