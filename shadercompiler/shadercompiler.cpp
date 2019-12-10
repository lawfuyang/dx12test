#include <vector>
#include <cassert>
#include <fstream>
#include <sstream>
#include <array>
#include <wrl.h>

typedef uint64_t WindowHandle;
using Microsoft::WRL::ComPtr;

#include "system/utils.h"
#include "system/profiler.h"
#include "system/logger.h"

#include "graphic/dx12utils.h"

struct ShaderCompileJob;

std::string g_AppDir;
std::string g_ShadersDir;
std::string g_DXCDir;
std::string g_CompiledHeadersOutputDir;
std::string g_AutogenFileOutputDir;

std::vector<std::string>      g_AllShaderFiles;
std::vector<ShaderCompileJob> g_AllShaderCompilerJobs;

D3D_SHADER_MODEL g_HighestShaderModel = D3D_SHADER_MODEL_5_1;

enum class ShaderType { VS, PS, GS, HS, DS, CS };
constexpr const char* g_ShaderTypeStrings[] = { "VS", "PS", "GS", "HS", "DS", "CS" };
constexpr const char* g_TargetProfileVersionStrings[] = { "_6_0", "_6_1", "_6_2", "_6_3", "_6_4", "_6_5" };

constexpr char g_ShaderDeclarationStr[]    = "ShaderDeclaration:";
constexpr char g_EntryPointStr[]           = "EntryPoint:";
constexpr char g_EndShaderDeclarationStr[] = "EndShaderDeclaration";

struct DXCProcessWrapper
{
    DXCProcessWrapper(const std::string& inputCommandLine)
    {
        m_StartupInfo.cb = sizeof(::STARTUPINFO);

        std::string commandLine = g_DXCDir.c_str();
        commandLine += " " + inputCommandLine;

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
        commandLine += " -E " + m_EntryPoint;
        commandLine += " -T " + GetTargetProfileString();
        commandLine += " -Fo " + g_CompiledHeadersOutputDir + m_ShaderName + ".bin";

        // debugging stuff
        commandLine += " -Zi -Qembed_debug ";

        DXCProcessWrapper compilerProcess{ commandLine };
    }

    ShaderType  m_ShaderType;
    std::string m_ShaderFilePath;
    std::string m_EntryPoint;
    std::string m_ShaderName;
};

static void GetD3D12ShaderModels()
{
    bbeProfileFunction();

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
        assert(g_HighestShaderModel != D3D_SHADER_MODEL_5_1);

        break;
    }
}

int main()
{
    Logger::GetInstance().Initialize("../bin/ShaderCompilerOutput.txt");

    // uncomment to profile entire ShaderCompiler
    // const ProfilerInstance profilerInstance{ true }; bbeProfile("main");

    g_AppDir                   = GetApplicationDirectory();
    g_CompiledHeadersOutputDir = g_AppDir + "\\tmp\\ShaderCompilerOutput\\";
    g_ShadersDir               = g_AppDir + "..\\src\\graphic\\shaders";
    g_AutogenFileOutputDir     = g_ShadersDir + "\\shadersautogen.h";
    g_DXCDir                   = g_AppDir + "..\\tools\\dxc\\dxc.exe";
    
    {
        bbeProfile("Get All shader files");
        GetFilesInDirectory(g_AllShaderFiles, g_ShadersDir);
    }

    GetD3D12ShaderModels();
    
    // TODO: parallelize this
    for (const std::string& fullPath : g_AllShaderFiles)
    {
        bbeProfile("Parsing file");

        printf("Found shader file: %s\n", fullPath.c_str());

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
                printf("Found shader entry: %s\n", newJob.m_ShaderName.c_str());
                g_AllShaderCompilerJobs.push_back(std::move(newJob));
            }
        }
    }
    printf("\n");

    // TODO: parallelize this
    for (ShaderCompileJob& shaderJob : g_AllShaderCompilerJobs)
    {
        shaderJob.StartJob();
    }

    system("pause");
    return 0;
}
