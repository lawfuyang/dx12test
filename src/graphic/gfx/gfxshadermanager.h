#pragma once

// Must be exactly the same as in the ShaderCompiler
DEFINE_ENUM_WITH_STRING_CONVERSIONS(GfxShaderType, (VS)(PS)(CS));

namespace AutoGenerated
{ 
    struct ShaderData;
}

class GfxShader
{
public:
    ID3DBlob* GetBlob() const { return m_ShaderBlob.Get(); }
    std::size_t GetHash() const { return m_Hash; }
    GfxShaderType GetType() const { return m_Type; }

protected:
    void Initialize(const AutoGenerated::ShaderData&);

    GfxShaderType m_Type;
    ComPtr<ID3DBlob> m_ShaderBlob;
    std::size_t m_Hash = 0;

    friend class GfxShaderManager;
};

class GfxShaderManager
{
public:
    DeclareSingletonFunctions(GfxShaderManager);

    void Initialize(tf::Subflow& sf);

    template <typename ShaderPermutations>
    const GfxShader& GetShader(const ShaderPermutations& permutations) const
    {
        return m_ShaderContainers[ShaderPermutations::ShaderType].at(ShaderPermutations::BaseShaderID).at(permutations.m_ShaderKey);
    }

    template <GfxShaderType>
    static GfxShader& GetNullShader();

private:

    // key == BaseShaderID, val = { ShaderKey, GfxShader }
    using ShaderContainer = std::unordered_map<std::size_t, std::unordered_map<uint32_t, GfxShader>>;
    ShaderContainer m_ShaderContainers[GfxShaderType_Count];
};
#define g_GfxShaderManager GfxShaderManager::GetInstance()
