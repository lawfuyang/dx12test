#include "shadercompilejob.h"
#include "constantbuffer.h"
#include "shaderkeyhelper.h"
#include "shaderpermutationsprintjob.h"

void PrintToConsoleAndLogFile(const std::string& str);

extern std::string g_ShadersDir;
extern std::mutex g_AllConstantBuffersLock;
extern std::mutex g_AllShaderPermutationsPrintJobsLock;
extern std::vector<ConstantBuffer> g_AllConstantBuffers;
extern std::vector<ShaderPermutationsPrintJob> g_AllShaderPermutationsPrintJobs;
extern std::unordered_map<std::string, std::vector<std::string>> g_AllShaderPermutationsStrings;

struct ForwardLighting
{
    DEFINE_ENUM_WITH_STRING_CONVERSIONS(VSPermutations,
        (VERTEX_FORMAT_Position2f_TexCoord2f_Color4ub)
        (VERTEX_FORMAT_Position3f_Normal3f_Texcoord2f)
        (VERTEX_FORMAT_Position3f_Normal3f_Texcoord2f_Tangent3f)
        (VSPermutations_Count)
    );

    DEFINE_ENUM_WITH_STRING_CONVERSIONS(PSPermutations,
        (USE_PBR_CONSTS)
        (PSPermutations_Count)
    );

    static bool FilterVSKeys(uint32_t key)
    {
        bool isValid = true;
        isValid &= OnlyOneBitSet(key, 
            1 << VERTEX_FORMAT_Position2f_TexCoord2f_Color4ub,
            1 << VERTEX_FORMAT_Position3f_Normal3f_Texcoord2f,
            1 << VERTEX_FORMAT_Position3f_Normal3f_Texcoord2f_Tangent3f);

        //g_Log.info("key: {}, {}", std::bitset<4>{key}.to_string().c_str(), isValid);

        return isValid;
    }

    static void InitConstantBuffers()
    {
        {
            //cbuffer PerFrameConsts : register(b1)
            //{
            //    float4x4 g_ViewProjMatrix;
            //    float4 g_SceneLightDir;
            //    float4 g_SceneLightIntensity;
            //};
            ConstantBuffer cb;
            cb.m_Name = "PerFrameConsts";
            cb.m_Register = 1;
            cb.AddVariable("float4x4", "ViewProjMatrix");
            cb.AddVariable("float4", "CameraPosition");
            cb.AddVariable("float4", "SceneLightDir");
            cb.AddVariable("float4", "SceneLightIntensity");
            cb.AddVariable("float", "ConstPBRRoughness");
            cb.AddVariable("float", "ConstPBRMetallic");

            bbeAutoLock(g_AllConstantBuffersLock);
            g_AllConstantBuffers.push_back(cb);
        }

        {
            //cbuffer PerInstanceConsts : register(b0)
            //{
            //    float4x4 g_WorldMatrix;
            //};
            ConstantBuffer cb;
            cb.m_Name = "PerInstanceConsts";
            cb.m_Register = 0;
            cb.AddVariable("float4x4", "WorldMatrix");

            bbeAutoLock(g_AllConstantBuffersLock);
            g_AllConstantBuffers.push_back(cb);
        }
    }

    static void PopulateJobs()
    {
        InitConstantBuffers();

        bool validVSKeys[1 << VSPermutations_Count];
        GetValidShaderKeys(validVSKeys, FilterVSKeys);

        bool validPSKeys[1 << PSPermutations_Count];
        GetValidShaderKeys(validPSKeys, NoFilterForKeys);

        std::vector<std::string> allVSPerms(VSPermutations_Count);
        for (uint32_t i = 0; i < VSPermutations_Count; ++i)
        {
            allVSPerms[i] = EnumToString((VSPermutations)i);
        }

        std::vector<std::string> allPSPerms(PSPermutations_Count);
        for (uint32_t i = 0; i < PSPermutations_Count; ++i)
        {
            allPSPerms[i] = EnumToString((PSPermutations)i);
        }

        PopulateJobParams params;
        params.m_ShaderFilePath = g_ShadersDir + "forwardlighting.hlsl";
        params.m_ShaderName = ms_ShaderName;
        params.m_ShaderID = GetCompileTimeCRC32(ms_ShaderName);

        params.m_EntryPoint = "VSMain";
        params.m_ShaderType = GfxShaderType::VS;
        params.m_DefineStrings = allVSPerms;
        params.m_KeysArray = validVSKeys;
        params.m_KeysArraySz = _countof(validVSKeys);
        PopulateJobsArray(params);

        params.m_EntryPoint = "PSMain";
        params.m_ShaderType = GfxShaderType::PS;
        params.m_DefineStrings = allPSPerms;
        params.m_KeysArray = validPSKeys;
        params.m_KeysArraySz = _countof(validPSKeys);
        PopulateJobsArray(params);


        bbeAutoLock(g_AllShaderPermutationsPrintJobsLock);
        {
            ShaderPermutationsPrintJob& printJob = g_AllShaderPermutationsPrintJobs.emplace_back();
            printJob.m_ShaderType = GfxShaderType::VS;
            printJob.m_BaseShaderID = GetCompileTimeCRC32(ms_ShaderName);
            printJob.m_BaseShaderName = StringFormat("VS_%s", ms_ShaderName);
            printJob.m_Defines = allVSPerms;
        }

        {
            ShaderPermutationsPrintJob& printJob = g_AllShaderPermutationsPrintJobs.emplace_back();
            printJob.m_ShaderType = GfxShaderType::PS;
            printJob.m_BaseShaderID = GetCompileTimeCRC32(ms_ShaderName);
            printJob.m_BaseShaderName = StringFormat("PS_%s", ms_ShaderName);
            printJob.m_Defines = allPSPerms;
        }
    }

    static inline const char* ms_ShaderName = "ForwardLighting";
};

RegisterJobPopulator(ForwardLighting, ForwardLighting::PopulateJobs);
