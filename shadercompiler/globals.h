#pragma once

#include "shadercompilejob.h"
#include "constantbuffer.h"
#include "shaderpermutationsprintjob.h"

struct Globals
{
    DeclareSingletonFunctions(Globals);

    std::string m_AppDir;
    std::string m_ShadersTmpDir;
    std::string m_ShadersTmpHLSLAutogenDir;
    std::string m_ShadersTmpCPPAutogenDir;
    std::string m_ShadersDir;
    std::string m_DXCDir;
    std::string m_ShadersByteCodesDir;

    std::vector<std::function<void()>> m_JobsPopulators;

    std::mutex m_AllShaderCompileJobsLock;
    std::mutex m_AllConstantBuffersLock;
    std::mutex m_AllShaderPermutationsPrintJobsLock;

    std::vector<ShaderCompileJob>                m_AllShaderCompileJobs;
    std::vector<ConstantBuffer>                  m_AllConstantBuffers;
    std::vector<ShaderPermutationsPrintJob>      m_AllShaderPermutationsPrintJobs;
    std::unordered_map<std::string, std::string> m_HLSLTypeToCPPTypeMap;
    std::unordered_map<std::string, uint32_t>    m_CPPTypeSizeMap;

    D3D_SHADER_MODEL m_HighestShaderModel;

    bool m_CompileFailureDetected;
};
#define g_Globals Globals::GetInstance()
