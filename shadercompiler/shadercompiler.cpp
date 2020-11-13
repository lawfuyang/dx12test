
#include <graphic/dx12utils.h>

#include "globals.h"

tf::Executor g_Executor;

void PrintToConsoleAndLogFile(const std::string& str)
{
    printf("%s\n", str.c_str());
    g_Log.info("{}", str.c_str());
}

static void GetD3D12ShaderModels()
{
    ComPtr<IDXGIFactory7> DXGIFactory;
    DX12_CALL(CreateDXGIFactory2(0, IID_PPV_ARGS(&DXGIFactory)));

    ComPtr<IDXGIAdapter1> hardwareAdapter;
    for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != DXGIFactory->EnumAdapters1(adapterIndex, &hardwareAdapter); ++adapterIndex)
    {
        DXGI_ADAPTER_DESC1 desc;
        hardwareAdapter->GetDesc1(&desc);

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            // Don't bother with the Basic Render Driver adapter.
            continue;
        }

        // we only care about D3D_FEATURE_LEVEL_1_0_CORE support
        ComPtr<ID3D12Device6> D3DDevice;
        if (SUCCEEDED(D3D12CreateDevice(hardwareAdapter.Get(), D3D_FEATURE_LEVEL_1_0_CORE, IID_PPV_ARGS(&D3DDevice))))
        {
            g_Log.info("Created ID3D12Device. D3D_FEATURE_LEVEL: {}", GetD3DFeatureLevelName(D3D_FEATURE_LEVEL_1_0_CORE));
        }
        assert(D3DDevice);

        // Query the level of support of Shader Model.
        const D3D12_FEATURE_DATA_SHADER_MODEL shaderModels[] =
        {
            D3D_SHADER_MODEL_6_6,
            D3D_SHADER_MODEL_6_5,
            D3D_SHADER_MODEL_6_4,
            D3D_SHADER_MODEL_6_3,
            D3D_SHADER_MODEL_6_2,
            D3D_SHADER_MODEL_6_1,
            D3D_SHADER_MODEL_6_0,
            D3D_SHADER_MODEL_5_1
        };

        for (D3D12_FEATURE_DATA_SHADER_MODEL shaderModel : shaderModels)
        {
            if (SUCCEEDED(D3DDevice->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel))))
            {
                g_Log.info("D3D12_FEATURE_SHADER_MODEL support: {}", GetD3DShaderModelName(shaderModel.HighestShaderModel));
                g_Globals.m_HighestShaderModel = shaderModel.HighestShaderModel;
                break;
            }
        }
        assert(g_Globals.m_HighestShaderModel > D3D_SHADER_MODEL_5_1);

        break;
    }
}

static std::size_t GetFileContentsHash(const std::string& dir)
{
    const bool IsReadMode = true;
    CFileWrapper file(dir, IsReadMode);

    // no existing generated file exists... return 0
    if (!file)
        return std::size_t{ 0 };

    // read each line and hash the entire file contents
    std::string fileContents;
    StaticString<BBE_KB(1)> buffer;
    while (fgets(buffer.data(), sizeof(buffer), file))
    {
        fileContents += buffer.c_str();
        buffer.clear();
    }
    return std::hash<std::string>{} (fileContents);
}

static void OverrideExistingFileIfNecessary(const std::string& generatedString, const std::string& dirFull)
{
    // hash both existing and newly generated contents
    const std::size_t existingHash = GetFileContentsHash(dirFull);
    const std::size_t newHash = std::hash<std::string>{}(generatedString);

    // if hashes are different, over ride with new contents
    if (existingHash != newHash)
    {
        g_Log.info("hash different for '{}'... over-riding with new contents", dirFull.c_str());
        const bool IsReadMode = false;
        CFileWrapper file{ dirFull, IsReadMode };
        assert(file);
        fprintf(file, "%s", generatedString.c_str());
    }
    else
    {
        g_Log.info("No change detected for '{}'", dirFull);
    }
}

static void PrintGeneratedByteCodeHeadersFile()
{
    std::string generatedString;
    generatedString += "#pragma once\n\n";
    generatedString += "namespace AutoGenerated\n";
    generatedString += "{\n\n";

    for (const ShaderCompileJob& shaderJob : g_Globals.m_AllShaderCompileJobs)
    {
        generatedString += StringFormat("#include \"%s\"\n", shaderJob.m_ShaderObjCodeFileDir.c_str());
    }

    generatedString += "\n";
    generatedString += "struct ShaderData\n";
    generatedString += "{\n";
    generatedString += "    const unsigned char* m_ByteCodeArray;\n";
    generatedString += "    const uint32_t m_ByteCodeSize;\n";
    generatedString += "    const std::size_t m_Hash;\n";
    generatedString += "    const uint32_t m_ShaderKey;\n";
    generatedString += "    const uint32_t m_BaseShaderID;\n";
    generatedString += "    const GfxShaderType m_ShaderType;\n";
    generatedString += "};\n";

    generatedString += "\n";
    generatedString += "static const ShaderData gs_AllShadersData[] = \n";
    generatedString += "{\n";
    for (const ShaderCompileJob& shaderJob : g_Globals.m_AllShaderCompileJobs)
    {
        generatedString += "    {\n";
        generatedString += StringFormat("        %s,\n", shaderJob.m_ShaderObjCodeVarName.c_str());
        generatedString += StringFormat("        _countof(%s),\n", shaderJob.m_ShaderObjCodeVarName.c_str());
        generatedString += StringFormat("        %" PRIu64 ",\n", GetFileContentsHash(shaderJob.m_ShaderObjCodeFileDir));
        generatedString += StringFormat("        %u,\n", shaderJob.m_ShaderKey);
        generatedString += StringFormat("        %u,\n", shaderJob.m_BaseShaderID);
        generatedString += StringFormat("        %s,\n", gs_GfxShaderTypeStrings[(uint32_t)shaderJob.m_ShaderType]);
        generatedString += "    },\n";
        generatedString += "\n";
    }
    generatedString += "};\n\n";

    generatedString += "\n";
    generatedString += "}\n";

    OverrideExistingFileIfNecessary(generatedString, g_Globals.m_ShadersByteCodesDir);
}

static void PrintShaderPermutationStructs()
{
    for (const ShaderPermutationsPrintJob& job : g_Globals.m_AllShaderPermutationsPrintJobs)
    {
        std::string generatedString;
        generatedString += "#pragma once\n\n";
        generatedString += "namespace Shaders\n";
        generatedString += "{\n\n";

        generatedString += StringFormat("struct %sPermutations\n", job.m_BaseShaderName.c_str());
        generatedString += "{\n";

        generatedString += "    union\n";
        generatedString += "    {\n";
        generatedString += "        struct\n";
        generatedString += "        {\n";
        for (const std::string& permutationString : job.m_Defines)
        {
            generatedString += StringFormat("           bool %s : 1;\n", permutationString.c_str());
        }
        generatedString += "        };\n";
        generatedString += "        uint32_t m_ShaderKey = 0;\n";
        generatedString += "    };\n\n";
        generatedString += "private:\n";
        generatedString += StringFormat("    static const uint32_t BaseShaderID = %u;\n", job.m_BaseShaderID);
        generatedString += StringFormat("    static const GfxShaderType ShaderType = %s;\n", gs_GfxShaderTypeStrings[(uint32_t)job.m_ShaderType]);
        generatedString += "\n";
        generatedString += "    friend class GfxShaderManager;\n";

        generatedString += "};\n";

        generatedString += "\n";
        generatedString += "}\n";


        const std::string outputDir = g_Globals.m_ShadersTmpCPPAutogenDir + job.m_BaseShaderName + ".h";
        OverrideExistingFileIfNecessary(generatedString, outputDir);
    }
}

static void PrintAutogenFilesForShaders()
{
    PrintGeneratedByteCodeHeadersFile();
    PrintShaderPermutationStructs();
}

static void PrintAutogenFilesForCBs(ConstantBuffer& cb)
{
    cb.SanityCheck();

    PrintToConsoleAndLogFile(StringFormat("ConstantBuffer: %s, register: %u", cb.m_Name, cb.m_Register));

    // CPP
    {
        std::string generatedString;
        generatedString += "#pragma once\n\n";
        generatedString += "namespace AutoGenerated\n";
        generatedString += "{\n\n";

        generatedString += StringFormat("struct alignas(16) %s\n", cb.m_Name);
        generatedString += "{\n";
        generatedString += StringFormat("    inline static const char* ms_Name = \"%s GfxConstantBuffer\";\n\n", cb.m_Name);

        for (const ConstantBuffer::CPPTypeVarNamePair& var : cb.m_Variables)
        {
            generatedString += StringFormat("    %s m_%s;\n", var.m_CPPVarType.c_str(), var.m_VarName.c_str());
        }

        generatedString += "};\n\n";

        generatedString += "\n";
        generatedString += "}\n";

        const std::string outputDir = g_Globals.m_ShadersTmpCPPAutogenDir + cb.m_Name + ".h";
        OverrideExistingFileIfNecessary(generatedString, outputDir);
    }

    // HLSL
    {
        std::string generatedString;

        generatedString += StringFormat("#ifndef __%s_H__\n", cb.m_Name);
        generatedString += StringFormat("#define __%s_H__\n\n", cb.m_Name);

        // cbuffer definition
        generatedString += StringFormat("cbuffer %s_CB : register(b%u)\n", cb.m_Name, cb.m_Register);
        generatedString += "{\n";
        for (const ConstantBuffer::CPPTypeVarNamePair& var : cb.m_Variables)
        {
            generatedString += StringFormat("    %s g_%s_%s;\n", var.m_HLLSVarType.c_str(), cb.m_Name, var.m_VarName.c_str());
        }
        generatedString += "};\n\n";

        // struct for cbuffer definition
        generatedString += StringFormat("struct %s\n", cb.m_Name, cb.m_Register);
        generatedString += "{\n";
        for (const ConstantBuffer::CPPTypeVarNamePair& var : cb.m_Variables)
        {
            generatedString += StringFormat("    %s m_%s;\n", var.m_HLLSVarType.c_str(), var.m_VarName.c_str());
        }
        generatedString += "};\n\n";

        // Helper creater function for struct
        generatedString += StringFormat("%s Create%s()\n", cb.m_Name, cb.m_Name);
        generatedString += "{\n";
        generatedString += StringFormat("    %s consts;\n", cb.m_Name);
        for (const ConstantBuffer::CPPTypeVarNamePair& var : cb.m_Variables)
        {
            generatedString += StringFormat("    consts.m_%s = g_%s_%s;\n", var.m_VarName.c_str(), cb.m_Name, var.m_VarName.c_str());
        }
        generatedString += "    return consts;\n";
        generatedString += "}\n\n";

        generatedString += StringFormat("#endif // #define __%s_H__\n", cb.m_Name);

        const std::string outputDir = g_Globals.m_ShadersTmpHLSLAutogenDir + cb.m_Name + ".h";
        OverrideExistingFileIfNecessary(generatedString, outputDir);
    }
}

struct GlobalsInitializer
{
    GlobalsInitializer()
    {
        // init dirs
        g_Globals.m_AppDir                   = GetApplicationDirectory();
        g_Globals.m_ShadersTmpDir            = g_Globals.m_AppDir + "..\\tmp\\shaders\\";
        g_Globals.m_ShadersTmpHLSLAutogenDir = g_Globals.m_ShadersTmpDir + "autogen\\hlsl\\";
        g_Globals.m_ShadersTmpCPPAutogenDir  = g_Globals.m_ShadersTmpDir + "autogen\\cpp\\";
        g_Globals.m_ShadersDir               = g_Globals.m_AppDir + "..\\src\\graphic\\shaders\\";
        g_Globals.m_ShadersByteCodesDir      = g_Globals.m_ShadersTmpDir + "shaderbytecodes.h";
        g_Globals.m_DXCDir                   = g_Globals.m_AppDir + "..\\extern\\dxc\\dxc.exe";

        CreateDirectory(StringFormat("%s..\\tmp", g_Globals.m_AppDir.c_str()).c_str(), nullptr);
        CreateDirectory(g_Globals.m_ShadersTmpDir.c_str(), nullptr);
        CreateDirectory((g_Globals.m_ShadersTmpDir + "autogen\\").c_str(), nullptr);
        CreateDirectory(g_Globals.m_ShadersTmpHLSLAutogenDir.c_str(), nullptr);
        CreateDirectory(g_Globals.m_ShadersTmpCPPAutogenDir.c_str(), nullptr);

        g_Globals.m_AllShaderCompileJobs.reserve(1024);

        // init misc traits
    #define ADD_TYPE(hlslType, CPPType, TypeSize)   \
        g_Globals.m_HLSLTypeToCPPTypeMap[hlslType] = CPPType; \
        g_Globals.m_CPPTypeSizeMap[CPPType] = TypeSize;

        ADD_TYPE("int", "int32_t", 4);
        ADD_TYPE("int2", "bbeVector2I", 8);
        ADD_TYPE("int3", "bbeVector3I", 12);
        ADD_TYPE("int4", "bbeVector4I", 16);
        ADD_TYPE("uint", "uint32_t", 4);
        ADD_TYPE("uint2", "bbeVector2U", 8);
        ADD_TYPE("uint3", "bbeVector3U", 12);
        ADD_TYPE("uint4", "bbeVector4U", 16);
        ADD_TYPE("float", "float", 4);
        ADD_TYPE("float2", "bbeVector2", 8);
        ADD_TYPE("float3", "bbeVector3", 12);
        ADD_TYPE("float4", "bbeVector4", 16);
        ADD_TYPE("float4x4", "bbeMatrix", 64);
    #undef ADD_TYPE
    }
};
static const GlobalsInitializer g_GlobalsInitializer;

int main()
{
    // first, Init the logger
    Logger::GetInstance().Initialize("../bin/shadercompiler_output.txt");

    tf::Taskflow getShaderModelGetTF;
    getShaderModelGetTF.emplace([]() { GetD3D12ShaderModels(); });
    std::future<void> shaderModelFuture = g_Executor.run(getShaderModelGetTF);

    // get each shader to populate g_AllShaderCompileJobs
    {
        tf::Taskflow tf;

        for (const auto& func : g_Globals.m_JobsPopulators)
        {
            tf.emplace([&]() { func(); });
        }
        g_Executor.run(tf).wait();
    }

    // print auto-gen constant buffer headers
    {
        tf::Taskflow tf;

        tf.for_each(g_Globals.m_AllConstantBuffers.begin(), g_Globals.m_AllConstantBuffers.end(), [&](ConstantBuffer& cb)
            {
                PrintAutogenFilesForCBs(cb);
            });

        // sort the container, so that it always produces consistent auto-gen files
        tf.emplace([&]()
            {
                std::sort(g_Globals.m_AllShaderCompileJobs.begin(), g_Globals.m_AllShaderCompileJobs.end(), [&](const ShaderCompileJob& lhs, ShaderCompileJob& rhs)
                    {
                        return lhs.m_BaseShaderName < rhs.m_BaseShaderName;
                    });
            });

        g_Executor.run(tf).wait();
    }

    // Must wait for d3d12 shader model to be retrived first before we schedule the jobs
    shaderModelFuture.wait();

    // run all jobs in g_AllShaderCompileJobs
    {
        tf::Taskflow tf;

        tf.for_each(g_Globals.m_AllShaderCompileJobs.begin(), g_Globals.m_AllShaderCompileJobs.end(), [&](ShaderCompileJob& shaderJob)
            {
                shaderJob.StartJob();
            });

        g_Executor.run(tf).wait();
    }

    if (g_Globals.m_CompileFailureDetected)
    {
        printf("Compile failure(s) detected! Check log output for more information.\n\n");
    }
    else
    {
        PrintAutogenFilesForShaders();
    }

    if (g_Globals.m_AllShaderCompileJobs.size() > 1024)
    {
        PrintToConsoleAndLogFile("g_AllShaderCompileJobs has more than 1024 jobs! Increase reserve count");
    }

    system("pause");
    return 0;
}
