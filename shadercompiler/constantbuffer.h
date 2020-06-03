#pragma once

struct ConstantBuffer
{
    struct CPPTypeVarNamePair
    {
        std::string m_HLLSVarType;
        std::string m_CPPVarType;
        std::string m_VarName;
    };

    void AddVariable(const std::string& hlslType, const std::string& varName);
    void SanityCheck();

    const char* m_Name = "";
    uint32_t    m_Register = 99;

    std::vector<CPPTypeVarNamePair> m_Variables;
};
