#pragma once

#define RegisterJobPopulator(Name, Func)                    \
extern std::vector<std::function<void()>> g_JobsPopulators; \
struct __Name__Init                                         \
{                                                           \
    __Name__Init::__Name__Init()                            \
    {                                                       \
        g_JobsPopulators.push_back(Func);                   \
    }                                                       \
};                                                          \
static const __Name__Init gs__Name__Init;


enum class GfxShaderType : uint32_t { VS, PS, CS };

struct ShaderCompileJob
{
    void StartJob();

    GfxShaderType m_ShaderType;
    uint32_t m_BaseShaderID;
    uint32_t m_ShaderKey;
    std::string m_ShaderFilePath;
    std::string m_EntryPoint;
    std::string m_ShaderName;
    std::string m_BaseShaderName;
    std::vector<std::string> m_Defines;

    // These 2 will be set internally
    std::string m_ShaderObjCodeVarName;
    std::string m_ShaderObjCodeFileDir;
};

struct DXCProcessWrapper
{
    DXCProcessWrapper(const std::string& inputCommandLine, const ShaderCompileJob& job);
};

struct PopulateJobParams
{
    std::string m_ShaderFilePath;
    std::string m_ShaderName;
    std::string m_EntryPoint;
    GfxShaderType m_ShaderType;
    std::vector<std::string> m_DefineStrings;
    bool* m_KeysArray;
    uint32_t m_KeysArraySz;
    uint32_t m_ShaderID;
};

void PopulateJobsArray(const PopulateJobParams& params);
