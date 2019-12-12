#include <cinttypes>
#include <vector>
#include <cassert>
#include <fstream>
#include <sstream>
#include <array>
#include <wrl.h>

typedef uint64_t WindowHandle;
using Microsoft::WRL::ComPtr;

#include "extern/cpp-taskflow/taskflow/taskflow.hpp"

#include "system/utils.h"
#include "system/profiler.h"
#include "system/logger.h"

#include "graphic/dx12utils.h"

struct ShaderCompileJob;

std::string g_AppDir;
std::string g_SettingsFileDir;
std::string g_ShadersDir;
std::string g_DXCDir;
std::string g_CompiledHeadersOutputDir;
std::string g_ShadersEnumsAutoGenDir;

std::vector<std::string>      g_AllShaderFiles;
std::vector<ShaderCompileJob> g_AllShaderCompileJobs;

D3D_SHADER_MODEL g_HighestShaderModel = D3D_SHADER_MODEL_5_1;

enum class ShaderType { VS, PS, GS, HS, DS, CS };

constexpr const char* g_ShaderTypeStrings[] = { "VS", "PS", "GS", "HS", "DS", "CS" };
constexpr const char* g_TargetProfileVersionStrings[] = { "_6_0", "_6_1", "_6_2", "_6_3", "_6_4", "_6_5" };

constexpr char g_ShaderDeclarationStr[]    = "ShaderDeclaration:";
constexpr char g_EntryPointStr[]           = "EntryPoint:";
constexpr char g_EndShaderDeclarationStr[] = "EndShaderDeclaration";

tf::Executor g_TasksExecutor;

template <bool IsReadMode>
struct CFileWrapper
{
    CFileWrapper(const std::string& fileName)
    {
        m_File = fopen(fileName.c_str(), IsReadMode ? "r" : "w");
    }

    ~CFileWrapper()
    {
        if (m_File)
        {
            fclose(m_File);
            m_File = nullptr;
        }
    }

    operator bool() const
    {
        return m_File;
    }

    operator FILE*() const
    {
        return m_File;
    }

    FILE* m_File = nullptr;
};

struct DXCProcessWrapper
{
    DXCProcessWrapper(const std::string& inputCommandLine)
    {
        m_StartupInfo.cb = sizeof(::STARTUPINFO);

        std::string commandLine = g_DXCDir.c_str();
        commandLine += " " + inputCommandLine;

        bbeInfo(commandLine.c_str());

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
        auto SafeCloseHandle = [](::HANDLE& hdl) {if (hdl)::CloseHandle(hdl); hdl = nullptr; };

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
        std::string commandLine = m_ShaderFilePath;
        commandLine += " -nologo";
        commandLine += " -E " + m_EntryPoint;
        commandLine += " -T " + GetTargetProfileString();
        commandLine += " -Fh " + g_CompiledHeadersOutputDir + m_ShaderName + ".h";
        commandLine += " -Vn " + m_ShaderName + "_ObjCode";

        // debugging stuff
        //commandLine += " -Zi -Qembed_debug -Fd " + g_CompiledHeadersOutputDir + m_ShaderName + ".pdb";

        DXCProcessWrapper compilerProcess{ commandLine };
    }

    ShaderType  m_ShaderType;
    std::string m_ShaderFilePath;
    std::string m_EntryPoint;
    std::string m_ShaderName;
};

static bool ReadShaderModelFromFile()
{
    const bool IsReadMode = true;
    CFileWrapper<IsReadMode> settingsFile{ g_SettingsFileDir.c_str() };
    if (!settingsFile)
        return false;

    char highestShaderModelStr[MAX_PATH] = {};
    for (int i = 0; i < 2; ++i)
    {
        fscanf(settingsFile, "%s", highestShaderModelStr);
    }
    g_HighestShaderModel = (D3D_SHADER_MODEL)std::atoi(highestShaderModelStr);
    assert(g_HighestShaderModel >= D3D_SHADER_MODEL_6_0);

    return true;
}

static void GetD3D12ShaderModels()
{
    bbeProfileFunction();

    if (ReadShaderModelFromFile())
        return;

    ComPtr<IDXGIFactory7> DXGIFactory;
    {
        bbeProfile("CreateDXGIFactory2");
        DX12_CALL(CreateDXGIFactory2(0, IID_PPV_ARGS(&DXGIFactory)));
    }

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
            bbeProfile("D3D12CreateDevice");

            if (SUCCEEDED(D3D12CreateDevice(hardwareAdapter.Get(), level, IID_PPV_ARGS(&D3DDevice))))
            {
                bbeInfo("Created ID3D12Device. D3D_FEATURE_LEVEL: %s", GetD3DFeatureLevelName(level));
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
                bbeInfo("D3D12_FEATURE_SHADER_MODEL support: %s", GetD3DShaderModelName(shaderModel.HighestShaderModel));
                g_HighestShaderModel = shaderModel.HighestShaderModel;
                break;
            }
        }
        assert(g_HighestShaderModel >= D3D_SHADER_MODEL_6_0);

        const bool IsReadMode = false;
        CFileWrapper<IsReadMode> shaderModelFile{ g_SettingsFileDir.c_str() };
        assert(shaderModelFile);
        fprintf(shaderModelFile, "HighestShaderModel: %i\n", (int)g_HighestShaderModel);

        break;
    }
}

static void PopulateJobs()
{
    bbeProfileFunction();

    tf::Taskflow tf;
    tf.parallel_for(g_AllShaderFiles.begin(), g_AllShaderFiles.end(), [&](const std::string& fullPath)
    {
        bbeProfile("Parsing file"); 
        bbeInfo("Found shader file: %s", GetFileNameFromPath(fullPath).c_str());

        std::ifstream inputStream{ fullPath };
        assert(inputStream);

        ShaderCompileJob newJob;

        std::string line;
        while (std::getline(inputStream, line))
        {
            if (line.find(g_EndShaderDeclarationStr) != std::string::npos)
                break;

            std::stringstream inputStringStream{ line };

            bool newJobFound = false;
            std::string token;
            while (inputStringStream >> token)
            {
                if (token.find(g_ShaderDeclarationStr) != std::string::npos)
                {
                    assert(newJobFound == false);
                    newJobFound = true;
                    newJob.m_ShaderName = std::string{ token.begin() + sizeof(g_ShaderDeclarationStr) - 1, token.end() }; // minus 1 because of colon
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

                if (token.find(g_EntryPointStr) != std::string::npos)
                {
                    assert(newJobFound);
                    newJob.m_EntryPoint = std::string{ token.begin() + sizeof(g_EntryPointStr) - 1, token.end() }; // minus 1 because of colon
                }
            }

            if (newJobFound)
            {
                bbeInfo("Found shader entry: %s", newJob.m_ShaderName.c_str());
                g_AllShaderCompileJobs.push_back(std::move(newJob));
            }
        }
    });
    
    g_TasksExecutor.run(tf).wait();

    std::sort(g_AllShaderCompileJobs.begin(), g_AllShaderCompileJobs.end(), [](const ShaderCompileJob& lhs, const ShaderCompileJob& rhs)
        {
            return lhs.m_ShaderName < rhs.m_ShaderName;
        });
}

static void RunAllJobs()
{
    bbeProfileFunction();

    tf::Taskflow tf;
    tf.parallel_for(g_AllShaderCompileJobs.begin(), g_AllShaderCompileJobs.end(), [&](ShaderCompileJob& shaderJob)
        {
            bbeProfile("Run ShaderCompileJob");

            shaderJob.StartJob();
        });

    g_TasksExecutor.run(tf).wait();
}

static void PrintGeneratedEnumFile()
{
    bbeProfileFunction();

    const bool IsReadMode = false;
    CFileWrapper<IsReadMode> generatedEnumFile{ g_ShadersEnumsAutoGenDir };
    assert(generatedEnumFile);

    fprintf(generatedEnumFile, "#pragma once\n\n");
    fprintf(generatedEnumFile, "enum class AllShaders\n");
    fprintf(generatedEnumFile, "{\n");
    for (int i = 0; i < g_AllShaderCompileJobs.size(); ++i)
    {
        fprintf(generatedEnumFile, "    %s,\n", g_AllShaderCompileJobs[i].m_ShaderName.c_str());
    }
    fprintf(generatedEnumFile, "};\n");
    fprintf(generatedEnumFile, "const uint32_t g_NumShaders = %" PRIu64 ";\n", g_AllShaderCompileJobs.size());
}

static void RunShaderCompiler()
{
    // uncomment to profile entire ShaderCompiler
    //const ProfilerInstance profilerInstance{ true }; bbeProfileFunction();
    
    GetFilesInDirectory(g_AllShaderFiles, g_ShadersDir);

    GetD3D12ShaderModels();

    PopulateJobs();

    RunAllJobs();

    PrintGeneratedEnumFile();
}

int main()
{
    Logger::GetInstance().Initialize("../bin/ShaderCompilerOutput.txt");

    g_AppDir                   = GetApplicationDirectory();
    g_SettingsFileDir          = g_AppDir + "..\\tools\\dxc\\settings.ini";
    g_CompiledHeadersOutputDir = g_AppDir + "tmp\\ShaderCompilerOutput\\";
    g_ShadersDir               = g_AppDir + "..\\src\\graphic\\shaders";
    g_ShadersEnumsAutoGenDir   = g_CompiledHeadersOutputDir + "shadersenumsautogen.h";
    g_DXCDir                   = g_AppDir + "..\\tools\\dxc\\dxc.exe";
    
    RunShaderCompiler();

    system("pause");
    return 0;
}
