#include <inttypes.h>
#include <vector>
#include <assert.h>
#include <stdio.h>
#include <array>
#include <wrl.h>

typedef uint64_t WindowHandle;
using Microsoft::WRL::ComPtr;

#include "extern/cpp-taskflow/taskflow/taskflow.hpp"

#include "system/logger.h"
#include "system/utils.h"

#include "graphic/dx12utils.h"

struct ShaderCompileJob;
struct ConstantBufferDesc;

std::string g_AppDir;
std::string g_ShadersTmpDir;
std::string g_SettingsFileDir;
std::string g_ShadersDir;
std::string g_DXCDir;
std::string g_ShadersEnumsAutoGenDir;
std::string g_ShadersHeaderAutoGenDir;
std::string g_ShadersConstantBuffersAutoGenDir;

std::vector<std::string>                     g_AllShaderFiles;
std::vector<ShaderCompileJob>                g_AllShaderCompileJobs;
std::vector<ConstantBufferDesc>              g_AllConstantBuffers;
std::unordered_map<std::size_t, std::string> g_HLSLTypeToCPPTypeMap;

D3D_SHADER_MODEL g_HighestShaderModel = D3D_SHADER_MODEL_5_1;

enum class ShaderType { VS, PS, GS, HS, DS, CS };

const char* g_ShaderTypeStrings[] = { "VS", "PS", "GS", "HS", "DS", "CS" };
const char* g_TargetProfileVersionStrings[] = { "_6_0", "_6_1", "_6_2", "_6_3", "_6_4", "_6_5" };

const std::string g_ShaderDeclarationStr    = "ShaderDeclaration:";
const std::string g_EntryPointStr           = "EntryPoint:";
const std::string g_CBufferStr              = "cbuffer";
const std::string g_EndShaderDeclarationStr = "EndShaderDeclaration";

tf::Executor g_TasksExecutor;

struct DXCProcessWrapper
{
    explicit DXCProcessWrapper(const std::string& inputCommandLine)
    {
        m_StartupInfo.cb = sizeof(::STARTUPINFO);

        std::string commandLine = g_DXCDir.c_str();
        commandLine += " " + inputCommandLine;

        g_Log.info(commandLine);

        if (::CreateProcess(nullptr,    // No module name (use command line).
            (LPSTR)commandLine.c_str(), // Command line.
            nullptr,                    // Process handle not inheritable.
            nullptr,                    // Thread handle not inheritable.
            FALSE,                      // Set handle inheritance to FALSE.
            0,                          // No creation flags.
            nullptr,                    // Use parent's environment block.
            nullptr,                    // Use parent's starting directory.
            &m_StartupInfo,             // Pointer to STARTUPINFO structure.
            &m_ProcessInfo))            // Pointer to PROCESS_INFORMATION structure.
        {
            ::WaitForSingleObject(m_ProcessInfo.hProcess, INFINITE);
            CloseAllHandles();
        }
        else
        {
            CloseAllHandles();
            assert(false);
        }
    }

    void CloseAllHandles()
    {
        auto SafeCloseHandle = [](::HANDLE& hdl) { if (hdl) ::CloseHandle(hdl); hdl = nullptr; };

        SafeCloseHandle(m_StartupInfo.hStdInput);
        SafeCloseHandle(m_StartupInfo.hStdOutput);
        SafeCloseHandle(m_StartupInfo.hStdError);
        SafeCloseHandle(m_ProcessInfo.hProcess);
        SafeCloseHandle(m_ProcessInfo.hThread);
    }

    ::STARTUPINFO         m_StartupInfo = {};
    ::PROCESS_INFORMATION m_ProcessInfo = {};
};

struct ShaderCompileJob
{
    const std::string GetTargetProfileString() const
    {
        std::string result = std::string{ g_ShaderTypeStrings[(int)m_ShaderType] };
        result += g_TargetProfileVersionStrings[g_HighestShaderModel - D3D_SHADER_MODEL_6_0];

        std::transform(result.begin(), result.end(), result.begin(), [](char c) { return std::tolower(c); });

        return result;
    }

    void StartJob()
    {
        m_ShaderObjCodeFileDir = g_ShadersTmpDir + m_ShaderName + ".h";
        m_ShaderObjCodeVarName = m_ShaderName + "_ObjCode";

        std::string commandLine = m_ShaderFilePath;
        commandLine += " -nologo";
        commandLine += " -E " + m_EntryPoint;
        commandLine += " -T " + GetTargetProfileString();
        commandLine += " -Fh " + m_ShaderObjCodeFileDir;
        commandLine += " -Vn " + m_ShaderObjCodeVarName;

        // debugging stuff
        //commandLine += " -Zi -Qembed_debug -Fd " + g_ShadersTmpDir + m_ShaderName + ".pdb";

        DXCProcessWrapper compilerProcess{ commandLine };
    }

    ShaderType  m_ShaderType;
    std::string m_ShaderFilePath;
    std::string m_EntryPoint;
    std::string m_ShaderName;
    std::string m_ShaderObjCodeVarName;
    std::string m_ShaderObjCodeFileDir;
};

struct ConstantBufferDesc
{
    struct CPPTypeVarNamePair
    {
        std::string m_CPPVarType;
        std::string m_VarName;
    };

    void AddVariable(const std::string& hlslType, const std::string& varName)
    {
        assert(varName[0] == 'm' && varName[1] == '_');

        CPPTypeVarNamePair newVar;

        newVar.m_CPPVarType = g_HLSLTypeToCPPTypeMap.at(std::hash<std::string>{}(hlslType));
        newVar.m_VarName = varName;

        m_Variables.push_back(newVar);
    }

    std::string m_BufferName;
    uint32_t    m_Register = 99;

    std::vector<CPPTypeVarNamePair> m_Variables;
};

static bool ReadShaderModelFromFile()
{
    const bool IsReadMode = true;
    CFileWrapper settingsFile{ g_SettingsFileDir.c_str(), IsReadMode };
    if (!settingsFile)
        return false;

    char highestShaderModelStr[MAX_PATH] = {};
    for (int i = 0; i < 2; ++i)
    {
        fscanf_s(settingsFile, "%s", highestShaderModelStr, (unsigned int)sizeof(highestShaderModelStr));
    }
    g_HighestShaderModel = (D3D_SHADER_MODEL)std::atoi(highestShaderModelStr);
    assert(g_HighestShaderModel >= D3D_SHADER_MODEL_6_0);

    return true;
}

static void GetD3D12ShaderModels()
{
    if (ReadShaderModelFromFile())
        return;

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

        constexpr D3D_FEATURE_LEVEL featureLevels[] =
        {
            D3D_FEATURE_LEVEL_12_1,
            D3D_FEATURE_LEVEL_12_0,
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0
        };

        ComPtr<ID3D12Device6> D3DDevice;
        for (D3D_FEATURE_LEVEL level : featureLevels)
        {
            if (SUCCEEDED(D3D12CreateDevice(hardwareAdapter.Get(), level, IID_PPV_ARGS(&D3DDevice))))
            {
                g_Log.info("Created ID3D12Device. D3D_FEATURE_LEVEL: %s", GetD3DFeatureLevelName(level));
                break;
            }
        }
        assert(D3DDevice.Get() != nullptr);

        // Query the level of support of Shader Model.
        constexpr D3D12_FEATURE_DATA_SHADER_MODEL shaderModels[] =
        {
            D3D_SHADER_MODEL_6_5,
            D3D_SHADER_MODEL_6_4,
            D3D_SHADER_MODEL_6_3,
            D3D_SHADER_MODEL_6_2,
            D3D_SHADER_MODEL_6_1,
            D3D_SHADER_MODEL_6_0
        };

        for (D3D12_FEATURE_DATA_SHADER_MODEL shaderModel : shaderModels)
        {
            if (SUCCEEDED(D3DDevice->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel))))
            {
                g_Log.info("D3D12_FEATURE_SHADER_MODEL support: {}", GetD3DShaderModelName(shaderModel.HighestShaderModel));
                g_HighestShaderModel = shaderModel.HighestShaderModel;
                break;
            }
        }
        assert(g_HighestShaderModel >= D3D_SHADER_MODEL_6_0);

        const bool IsReadMode = false;
        CFileWrapper shaderModelFile{ g_SettingsFileDir.c_str(), IsReadMode };
        assert(shaderModelFile);
        fprintf(shaderModelFile, "HighestShaderModel: %i\n", (int)g_HighestShaderModel);

        break;
    }
}

static void HandleShaderDeclaration(ShaderCompileJob& newJob, const std::string& fullPath, char* token)
{
    if (std::strncmp(token, g_ShaderDeclarationStr.c_str(), g_ShaderDeclarationStr.size()) == 0)
    {
        assert(newJob.m_ShaderName.size() == 0);
        newJob.m_ShaderName = token + g_ShaderDeclarationStr.size();
        newJob.m_ShaderFilePath = fullPath;

        for (int i = 0; i < _countof(g_ShaderTypeStrings); ++i)
        {
            if (newJob.m_ShaderName[0] == g_ShaderTypeStrings[i][0])
            {
                newJob.m_ShaderType = (ShaderType)i;
                break;
            }
        }
    }
}

static void HandleShaderEntryPoint(ShaderCompileJob& newJob, char* token)
{
    if (std::strncmp(token, g_EntryPointStr.c_str(), g_EntryPointStr.size()) == 0)
    {
        assert(newJob.m_ShaderName.size() > 0);
        newJob.m_EntryPoint = token + g_EntryPointStr.size();
    }
}

static void HandleConstantBuffer(CFileWrapper& shaderFile, char (&line)[MAX_PATH], char* token)
{
    if (std::strncmp(token, g_CBufferStr.c_str(), g_CBufferStr.size()) == 0)
    {
        // assume CBuffer declaration line format == "cbuffer BufferName : register(b0)"
        // assume no comments in the whole declaration
        // assume all variables prefix with 'g_', as we will rename it to 'm_' in the engine

        ConstantBufferDesc newCBuffer;

        // get cbuffer name and register
        char* bufferName = std::strtok(NULL, " ");
        char* colon = std::strtok(NULL, " ");
        char* registerStr = std::strtok(NULL, " ");
        newCBuffer.m_BufferName = bufferName;
        sscanf(registerStr, "%*10c%u", &newCBuffer.m_Register);

        g_Log.info("Found constant buffer: '{}', bound at register: {}", bufferName, newCBuffer.m_Register);

        while (fgets(line, sizeof(line), shaderFile))
        {
            // beginning of CBuffer declaration
            if (std::strncmp(line, "{", 1) == 0)
                continue;

            // end of CBuffer declaration
            if (std::strncmp(line, "};", 2) == 0)
                break;

            // add variable type and name. Conveniently scan semi-colon as well
            char typeStr[10] = {};
            char varName[MAX_PATH] = {};
            sscanf(line, "%s %s", typeStr, varName);

            varName[0] = 'm';
            newCBuffer.AddVariable(typeStr, varName);
        }

        // do some sanity checks
        for (char c : newCBuffer.m_BufferName)
        {
            assert(std::isalpha(c));
        }
        assert(newCBuffer.m_Register != 99);
        g_AllConstantBuffers.push_back(newCBuffer);
    }
}

static void GetD3D12ShaderModelsAndPopulateJobs()
{
    tf::Taskflow tf;

    tf.emplace([&]() { GetD3D12ShaderModels(); });

    tf.parallel_for(g_AllShaderFiles.begin(), g_AllShaderFiles.end(), [&](const std::string& fullPath)
    {
        g_Log.info("Found shader file: {}", GetFileNameFromPath(fullPath).c_str());

        const bool IsReadMode = true;
        CFileWrapper shaderFile{ fullPath, IsReadMode };
        assert(shaderFile);

        char line[MAX_PATH] = {};
        bool endOfShaderDeclarations = false;
        while (fgets(line, sizeof(line), shaderFile) && endOfShaderDeclarations == false)
        {
            ShaderCompileJob newJob;
            char* token = std::strtok(line, " \n");
            while (token)
            {
                // reached end of all shader declarations. Stop
                if (strcmp(token, g_EndShaderDeclarationStr.c_str()) == 0)
                {
                    endOfShaderDeclarations = true;
                    break;
                }

                HandleShaderDeclaration(newJob, fullPath, token);
                HandleShaderEntryPoint(newJob, token);
                HandleConstantBuffer(shaderFile, line, token);

                // TODO: handle structs
                // TODO: handle buffers
                // TODO: handle textures
                // TODO: handle samplers

                token = strtok(NULL, " \n");
            }

            // insert new shader entry to be compiled
            if (newJob.m_ShaderName.size() > 0)
            {
                g_Log.info("Found shader: {}", newJob.m_ShaderName.c_str());
                g_AllShaderCompileJobs.push_back(std::move(newJob));
            }
        }
    });
    
    g_TasksExecutor.run(tf).wait();

    // sort shader names to maintain consistent hash
    std::sort(g_AllShaderCompileJobs.begin(), g_AllShaderCompileJobs.end(), [](const ShaderCompileJob& lhs, const ShaderCompileJob& rhs)
        {
            return lhs.m_ShaderName < rhs.m_ShaderName;
        });
}

static void RunAllJobs()
{
    tf::Taskflow tf;
    tf.parallel_for(g_AllShaderCompileJobs.begin(), g_AllShaderCompileJobs.end(), [&](ShaderCompileJob& shaderJob)
        {
            shaderJob.StartJob();
        });

    g_TasksExecutor.run(tf).wait();
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
    char buffer[BBE_KB(1)] = {};
    while (fgets(buffer, sizeof(buffer), file))
    {
        fileContents += buffer;
        memset(buffer, 0, sizeof(buffer));
    }
    return std::hash<std::string>{} (fileContents);
}

static void OverrideExistingFileIfNecessary(const std::string& generatedString, const std::string& dir)
{
    // hash both existing and newly generated contents
    const std::size_t existingHash = GetFileContentsHash(dir);
    const std::size_t newHash = std::hash<std::string>{}(generatedString);

    // if hashes are different, over ride with new contents
    if (existingHash != newHash)
    {
        g_Log.info("hash different for '{}'... over-riding with new contents", dir);
        const bool IsReadMode = false;
        CFileWrapper file{ dir, IsReadMode };
        assert(file);
        fprintf(file, "%s", generatedString.c_str());
    }
}

static void PrintGeneratedEnumFile()
{
    // populate the string with the generated shaders
    std::string generatedString;
    generatedString += "#pragma once\n\n";
    generatedString += "enum class ShaderPermutation\n";
    generatedString += "{\n";
    for (const ShaderCompileJob& shaderJob : g_AllShaderCompileJobs)
    {
        generatedString += StringFormat("    %s,\n", shaderJob.m_ShaderName.c_str());
    }
    generatedString += "};\n";
    generatedString += StringFormat("const uint32_t g_NumShaders = %" PRIu64 ";\n", g_AllShaderCompileJobs.size());

    OverrideExistingFileIfNecessary(generatedString, g_ShadersEnumsAutoGenDir);
}

static void PrintGeneratedByteCodeHeadersFile()
{
    std::string generatedString;
    generatedString += "#pragma once\n\n";
    generatedString += "namespace AutoGenerated\n";
    generatedString += "{\n\n";

    for (const ShaderCompileJob& shaderJob : g_AllShaderCompileJobs)
    {
        generatedString += StringFormat("#include \"%s\"\n", shaderJob.m_ShaderObjCodeFileDir.c_str());
    }

    generatedString += "\n";
    generatedString += "struct AutoGeneratedShaderData\n";
    generatedString += "{\n";
    generatedString += "    ShaderPermutation m_ShaderPermutation = (ShaderPermutation)0xDEADBEEF;\n";
    generatedString += "    const unsigned char* m_ByteCodeArray = nullptr;\n";
    generatedString += "    uint32_t m_ByteCodeSize = 0;\n";
    generatedString += "    std::size_t m_Hash = 0;\n";
    generatedString += "};\n";

    generatedString += "\n";
    generatedString += "static constexpr AutoGeneratedShaderData gs_AllShadersData[] = \n";
    generatedString += "{\n";
    for (const ShaderCompileJob& shaderJob : g_AllShaderCompileJobs)
    {
        generatedString += "    {\n";
        generatedString += StringFormat("        ShaderPermutation::%s,\n", shaderJob.m_ShaderName.c_str());
        generatedString += StringFormat("        %s,\n", shaderJob.m_ShaderObjCodeVarName.c_str());
        generatedString += StringFormat("        _countof(%s),\n", shaderJob.m_ShaderObjCodeVarName.c_str());
        generatedString += StringFormat("        %" PRIu64 "\n", GetFileContentsHash(shaderJob.m_ShaderObjCodeFileDir));
        generatedString += "    },\n";
        generatedString += "\n";
    }
    generatedString += "};\n";

    generatedString += "\n";
    generatedString += "}\n";

    OverrideExistingFileIfNecessary(generatedString, g_ShadersHeaderAutoGenDir);
}

static void PrintGeneratedConstantBuffersFile()
{
    std::string generatedString;
    generatedString += "#pragma once\n\n";
    generatedString += "namespace AutoGenerated\n";
    generatedString += "{\n\n";

    for (const ConstantBufferDesc& cBuffer : g_AllConstantBuffers)
    {
        generatedString += StringFormat("struct %s\n", cBuffer.m_BufferName.c_str());
        generatedString += "{\n";
        generatedString += StringFormat("    static const uint32_t ms_Register = %u;\n\n", cBuffer.m_Register);

        for (const ConstantBufferDesc::CPPTypeVarNamePair& var : cBuffer.m_Variables)
        {
            generatedString += StringFormat("    %s %s\n", var.m_CPPVarType.c_str(), var.m_VarName.c_str());
        }

        generatedString += "};\n\n";
    }

    generatedString += "\n";
    generatedString += "}\n";

    OverrideExistingFileIfNecessary(generatedString, g_ShadersConstantBuffersAutoGenDir);
}

static void PrintGeneratedFiles()
{
    tf::Taskflow tf;

    tf.emplace([&]() { PrintGeneratedEnumFile(); });
    tf.emplace([&]() { PrintGeneratedByteCodeHeadersFile(); });
    tf.emplace([&]() { PrintGeneratedConstantBuffersFile(); });

    g_TasksExecutor.run(tf).wait();
}

static void RunShaderCompiler()
{
    GetFilesInDirectory(g_AllShaderFiles, g_ShadersDir);

    GetD3D12ShaderModelsAndPopulateJobs();

    RunAllJobs();

    PrintGeneratedFiles();
}

int main()
{
    // first, Init the logger
    Logger::GetInstance().Initialize("../bin/shadercompiler_output.txt");

    g_AppDir                           = GetApplicationDirectory();
    g_ShadersTmpDir                    = g_AppDir + "..\\tmp\\shaders\\";
    g_SettingsFileDir                  = g_ShadersTmpDir + "shadercompilersettings.ini";
    g_ShadersDir                       = g_AppDir + "..\\src\\graphic\\shaders";
    g_ShadersEnumsAutoGenDir           = g_ShadersTmpDir + "shadersenumsautogen.h";
    g_ShadersHeaderAutoGenDir          = g_ShadersTmpDir + "shaderheadersautogen.h";
    g_ShadersConstantBuffersAutoGenDir = g_ShadersTmpDir + "shaderconstantbuffersautogen.h";
    g_DXCDir                           = g_AppDir + "..\\tools\\dxc\\dxc.exe";

#define ADD_TYPE(hlslType, CPPType) g_HLSLTypeToCPPTypeMap[std::hash<std::string>{}(hlslType)] = CPPType;

    ADD_TYPE("bool", "bool");
    ADD_TYPE("int", "int32_t");
    ADD_TYPE("int2", "XMINT2");
    ADD_TYPE("int3", "XMINT3");
    ADD_TYPE("int4", "XMINT4");
    ADD_TYPE("uint", "uint32_t");
    ADD_TYPE("uint2", "XMUINT2");
    ADD_TYPE("uint3", "XMUINT3");
    ADD_TYPE("uint4", "XMUINT4");
    ADD_TYPE("float", "float");
    ADD_TYPE("float2", "XMFLOAT2");
    ADD_TYPE("float3", "XMFLOAT3");
    ADD_TYPE("float4", "XMFLOAT4");
    ADD_TYPE("float4x4", "XMMATRIX");

#undef ADD_TYPE
    
    RunShaderCompiler();

    system("pause");
    return 0;
}
