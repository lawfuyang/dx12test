#include "globals.h"
#include "shadercompilejob.h"
#include "constantbuffer.h"
#include "shaderkeyhelper.h"
#include "shaderpermutationsprintjob.h"

void PrintToConsoleAndLogFile(const std::string& str);

struct IMGUI
{
    static void PopulateJobs()
    {
        //cbuffer IMGUICBuffer : register(b0)
        //{
        //    float4x4 g_ProjMatrix;
        //};
        ConstantBuffer cb;
        cb.m_Name = "IMGUIConsts";
        cb.m_Register = 0;
        cb.AddVariable("float4x4", "ProjMatrix");

        {
            bbeAutoLock(g_Globals.m_AllConstantBuffersLock);
            g_Globals.m_AllConstantBuffers.push_back(cb);
        }

        bool validKeys[1] = { true };

        PopulateJobParams params;
        params.m_ShaderFilePath = g_Globals.m_ShadersDir + "imgui.hlsl";
        params.m_ShaderName = "IMGUI";
        params.m_EntryPoint = "VSMain";
        params.m_ShaderType = GfxShaderType::VS;
        params.m_KeysArray = validKeys;
        params.m_KeysArraySz = _countof(validKeys);
        params.m_ShaderID = GetCompileTimeCRC32(ms_ShaderName);
        PopulateJobsArray(params);

        params.m_EntryPoint = "PSMain";
        params.m_ShaderType = GfxShaderType::PS;
        PopulateJobsArray(params);

        bbeAutoLock(g_Globals.m_AllShaderPermutationsPrintJobsLock);
        {
            ShaderPermutationsPrintJob& printJob = g_Globals.m_AllShaderPermutationsPrintJobs.emplace_back();
            printJob.m_ShaderType = GfxShaderType::VS;
            printJob.m_BaseShaderID = GetCompileTimeCRC32(ms_ShaderName);
            printJob.m_BaseShaderName = StringFormat("VS_%s", ms_ShaderName);
        }

        {
            ShaderPermutationsPrintJob& printJob = g_Globals.m_AllShaderPermutationsPrintJobs.emplace_back();
            printJob.m_ShaderType = GfxShaderType::PS;
            printJob.m_BaseShaderID = GetCompileTimeCRC32(ms_ShaderName);
            printJob.m_BaseShaderName = StringFormat("PS_%s", ms_ShaderName);
        }
    }

    static inline const char* ms_ShaderName = "IMGUI";
};

RegisterJobPopulator(IMGUI, IMGUI::PopulateJobs);
