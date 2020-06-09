#include "constantbuffer.h"

extern std::unordered_map<std::string, std::string> g_HLSLTypeToCPPTypeMap;
extern std::unordered_map<std::string, uint32_t>    g_CPPTypeSizeMap;

void ConstantBuffer::AddVariable(const std::string& hlslType, const std::string& varName)
{
    CPPTypeVarNamePair newVar;

    newVar.m_HLLSVarType = hlslType;
    newVar.m_CPPVarType = g_HLSLTypeToCPPTypeMap.at(hlslType);
    newVar.m_VarName = varName;

    m_Variables.push_back(newVar);
}

void ConstantBuffer::SanityCheck()
{
    // check size alignment
    uint32_t bufferSize = 0;
    for (const CPPTypeVarNamePair& var : m_Variables)
    {
        bufferSize += g_CPPTypeSizeMap.at(var.m_CPPVarType);
    }

    // add dummy padding vars to align to 16 byte
    uint32_t paddingVarCount = 0;
    while (!bbeIsAligned(bufferSize, 16))
    {
        AddVariable("uint", StringFormat("%s_DummyPaddingVar%d", m_Name, paddingVarCount++));
        bufferSize += 4;
    }
}
