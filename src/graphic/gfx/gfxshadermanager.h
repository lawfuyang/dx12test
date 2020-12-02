#pragma once

// Must be exactly the same as in the ShaderCompiler
DEFINE_ENUM_WITH_STRING_CONVERSIONS(GfxShaderType, (VS)(PS)(CS));

namespace AutoGenerated
{ 
    struct ShaderData;
}

struct GfxShader
{
    GfxShaderType m_Type;
    ComPtr<ID3DBlob> m_ShaderBlob;
    std::size_t m_Hash = 0;

private:
    void Initialize(const AutoGenerated::ShaderData&);

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
    using ShaderContainer = std::pmr::unordered_map<std::size_t, std::pmr::unordered_map<uint32_t, GfxShader>>;
    ShaderContainer m_ShaderContainers[GfxShaderType_Count];
};
#define g_GfxShaderManager GfxShaderManager::GetInstance()
