#pragma once

enum class GfxShaderType : uint32_t;

struct ShaderPermutationsPrintJob
{
    GfxShaderType m_ShaderType;
    uint32_t m_BaseShaderID;
    std::string m_BaseShaderName;
    std::vector<std::string> m_Defines;
};
