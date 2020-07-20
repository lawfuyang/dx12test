
#include "shadercompilejob.h"

// increase up to 64 when needed. This is just to prevent file names from being too long...
static const uint32_t MAX_SHADER_KEY_BITS = 4;

void PrintToConsoleAndLogFile(const std::string& str);

extern std::mutex g_AllShaderCompileJobsLock;
extern std::vector<ShaderCompileJob> g_AllShaderCompileJobs;
extern D3D_SHADER_MODEL g_HighestShaderModel;
extern std::string g_DXCDir;
extern std::string g_ShadersTmpDir;
extern bool g_CompileFailureDetected;

DXCProcessWrapper::DXCProcessWrapper(const std::string& inputCommandLine, const ShaderCompileJob& job)
{
    ::SECURITY_ATTRIBUTES saAttr;

    saAttr.nLength = sizeof(::SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    ::HANDLE hChildStd_OUT_Rd = NULL;
    ::HANDLE hChildStd_OUT_Wr = NULL;

    // Create a pipe for the child process's STDOUT. 
    bool success = ::CreatePipe(&hChildStd_OUT_Rd, &hChildStd_OUT_Wr, &saAttr, 0);
    assert(success);

    // Ensure the read handle to the pipe for STDOUT is not inherited.
    success = ::SetHandleInformation(hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0);
    assert(success);

    ::STARTUPINFO startupInfo = {};
    startupInfo.cb = sizeof(::STARTUPINFO);
    startupInfo.hStdError = hChildStd_OUT_Wr;
    startupInfo.hStdOutput = hChildStd_OUT_Wr;
    startupInfo.dwFlags |= STARTF_USESTDHANDLES;

    ::PROCESS_INFORMATION processInfo = {};

    std::string commandLine = g_DXCDir.c_str();
    commandLine += " " + inputCommandLine;

    if (!::CreateProcess(nullptr,   // No module name (use command line).
        (LPSTR)commandLine.c_str(), // Command line.
        nullptr,                    // Process handle not inheritable.
        nullptr,                    // Thread handle not inheritable.
        TRUE,                       // handles are inherited
        0,                          // No creation flags.
        nullptr,                    // Use parent's environment block.
        nullptr,                    // Use parent's starting directory.
        &startupInfo,               // Pointer to STARTUPINFO structure.
        &processInfo))              // Pointer to PROCESS_INFORMATION structure.
    {
        PrintToConsoleAndLogFile(GetLastErrorAsString());
        assert(false);
    }

    ::CloseHandle(processInfo.hProcess);
    ::CloseHandle(processInfo.hThread);
    ::CloseHandle(hChildStd_OUT_Wr);

    uint32_t numCharsRead = 0;
    std::string buffer;
    buffer.resize(BBE_MB(1)); // Up to 1 MB of output
    for (;;)
    {
        ::Sleep(1); // Sleep every loop iteration so the thread does not read 4/5 chars per "::ReadFile"

        DWORD dwRead = 0;
        const bool success = ::ReadFile(hChildStd_OUT_Rd, buffer.data(), 1024, &dwRead, NULL);
        numCharsRead += dwRead;
        if (!success || dwRead == 0) break;
    }

    if (numCharsRead)
    {
        g_Log.info("{}: {}", job.m_ShaderName.c_str(), buffer.c_str());
        g_CompileFailureDetected = true;
    }
}

static const std::string GetTargetProfileString(GfxShaderType type)
{
    const char* ShaderTypeStrings[] = { "VS", "PS", "GS", "HS", "DS", "CS" };
    const char* TargetProfileVersionStrings[] = { "_6_0", "_6_1", "_6_2", "_6_3", "_6_4", "_6_5" };

    std::string result = std::string{ ShaderTypeStrings[(int)type] };
    result += TargetProfileVersionStrings[g_HighestShaderModel - D3D_SHADER_MODEL_6_0];

    std::transform(result.begin(), result.end(), result.begin(), [](char c) { return std::tolower(c); });

    return result;
}

void ShaderCompileJob::StartJob()
{
    m_ShaderObjCodeFileDir = g_ShadersTmpDir + m_ShaderName + ".h";
    m_ShaderObjCodeVarName = m_ShaderName + "_ObjCode";

    std::string commandLine = m_ShaderFilePath;
    commandLine += " -E " + m_EntryPoint;
    commandLine += " -T " + GetTargetProfileString(m_ShaderType);
    commandLine += " -Fh " + m_ShaderObjCodeFileDir;
    commandLine += " -Vn " + m_ShaderObjCodeVarName;
    commandLine += " -nologo";
    commandLine += " -I" + g_ShadersTmpDir;
    commandLine += StringFormat(" -D%s ", (m_ShaderType == GfxShaderType::VS ? "VERTEX_SHADER" : "PIXEL_SHADER"));
    commandLine += " -Qunused-arguments ";
    commandLine += " -Qstrip_debug ";
    commandLine += " -Qstrip_priv ";
    commandLine += " -Qstrip_reflect ";
    commandLine += " -Qstrip_rootsignature ";

    for (const std::string& define : m_Defines)
    {
        commandLine += StringFormat(" -D%s ", define.c_str());
    }

    // debugging stuff
    //commandLine += " -Zi -Qembed_debug -Fd " + g_ShadersTmpDir + m_ShaderName + ".pdb";

    g_Log.info("ShaderCompileJob: " + commandLine);

    DXCProcessWrapper compilerProcess{ commandLine, *this };

    if (!g_CompileFailureDetected)
    {
        PrintToConsoleAndLogFile(StringFormat("Compiled %s", m_ShaderName.c_str()));
    }
}

void PopulateJobsArray(const PopulateJobParams& params)
{
    uint32_t totalPermutations = 0;
    for (uint32_t key = 0; key < params.m_KeysArraySz; ++key)
    {
        if (!params.m_KeysArray[key])
            continue;

        const std::bitset<MAX_SHADER_KEY_BITS> keyBitSet{ key };

        ShaderCompileJob newJob;
        newJob.m_BaseShaderID = params.m_ShaderID;
        newJob.m_ShaderKey = key;
        newJob.m_EntryPoint = params.m_EntryPoint;
        newJob.m_ShaderFilePath = params.m_ShaderFilePath;
        newJob.m_BaseShaderName = params.m_ShaderName;
        newJob.m_ShaderName = (params.m_ShaderType == GfxShaderType::VS ? "VS_" : "PS_") + newJob.m_BaseShaderName + "_" + keyBitSet.to_string();
        newJob.m_ShaderType = params.m_ShaderType;

        RunOnAllBits(keyBitSet.to_ulong(), [&](uint32_t index)
            {
                newJob.m_Defines.push_back(params.m_DefineStrings[index]);
            });

        ++totalPermutations;

        bbeAutoLock(g_AllShaderCompileJobsLock);
        g_AllShaderCompileJobs.push_back(newJob);
    }

    PrintToConsoleAndLogFile(StringFormat("%s%s: %u permutations", (params.m_ShaderType == GfxShaderType::VS ? "VS_" : "PS_"), params.m_ShaderName, totalPermutations));
}
