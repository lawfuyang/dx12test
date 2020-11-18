
// increase up to 64 when needed. This is just to prevent file names from being too long...
static const uint32_t MAX_SHADER_KEY_BITS = 4;

void PrintToConsoleAndLogFile(const std::string& str);

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

    std::string commandLine = g_Globals.m_DXCDir.c_str();
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

static const char* GetTargetProfileString(GfxShaderType type)
{
    const char* ShaderTypeStrings[] = { "VS", "PS", "GS", "HS", "DS", "CS" };
    static const std::unordered_map<D3D_SHADER_MODEL, const char*> s_TargetProfileToStr =
    {
        { D3D_SHADER_MODEL_5_1, "_5_1" },
        { D3D_SHADER_MODEL_6_0, "_6_0" },
        { D3D_SHADER_MODEL_6_1, "_6_1" },
        { D3D_SHADER_MODEL_6_2, "_6_2" },
        { D3D_SHADER_MODEL_6_3, "_6_3" },
        { D3D_SHADER_MODEL_6_4, "_6_4" },
        { D3D_SHADER_MODEL_6_5, "_6_5" },
        { D3D_SHADER_MODEL_6_6, "_6_6" },
    };

    StaticString<BBE_KB(1)> result = StringFormat("%s%s", ShaderTypeStrings[type], s_TargetProfileToStr.at(g_HighestD3D12ShaderModel)).c_str();
    StringUtils::ToLower(result);

    return result.c_str();
}

void ShaderCompileJob::StartJob()
{
    m_ShaderObjCodeFileDir = StringFormat("%s%s.h", g_Globals.m_ShadersTmpDir.c_str(), m_ShaderName.c_str());
    m_ShaderObjCodeVarName = StringFormat("%s_ObjCode", m_ShaderName.c_str());

    std::string commandLine = m_ShaderFilePath;
    commandLine += StringFormat(" -E %s", m_EntryPoint.c_str());
    commandLine += StringFormat(" -T %s", GetTargetProfileString(m_ShaderType));
    commandLine += StringFormat(" -Fh %s", m_ShaderObjCodeFileDir.c_str());
    //commandLine += " -Fsh " + g_ShadersTmpDir + m_ShaderName + "_hash.h"; // TODO
    commandLine += StringFormat(" -Vn %s", m_ShaderObjCodeVarName.c_str());
    commandLine += " -nologo ";
    commandLine += " -WX ";
    commandLine += StringFormat(" -I%s", g_Globals.m_ShadersTmpDir.c_str());
    commandLine += StringFormat(" -D%s ", (m_ShaderType == GfxShaderType::VS ? "VERTEX_SHADER" : "PIXEL_SHADER"));
    commandLine += " -Qunused-arguments ";

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
    const std::string shaderTypePrefix = StringFormat("%s_", EnumToString(params.m_ShaderType));

    uint32_t totalPermutations = 0;
    for (uint32_t key = 0; key < params.m_KeysArraySz; ++key)
    {
        if (!params.m_KeysArray[key])
            continue;

        static_assert(MAX_SHADER_KEY_BITS <= sizeof(unsigned long long) * CHAR_BIT);
        const std::bitset<MAX_SHADER_KEY_BITS> keyBitSet{ key };

        ShaderCompileJob newJob;
        newJob.m_BaseShaderID = params.m_ShaderID;
        newJob.m_ShaderKey = key;
        newJob.m_EntryPoint = params.m_EntryPoint;
        newJob.m_ShaderFilePath = params.m_ShaderFilePath;
        newJob.m_BaseShaderName = params.m_ShaderName;
        newJob.m_ShaderName = StringFormat("%s_%s_%s", shaderTypePrefix, newJob.m_BaseShaderName, keyBitSet.to_string());
        newJob.m_ShaderType = params.m_ShaderType;

        RunOnAllBits(keyBitSet.to_ulong(), [&](uint32_t index)
            {
                newJob.m_Defines.push_back(params.m_DefineStrings[index]);
            });

        ++totalPermutations;

        bbeAutoLock(g_Globals.m_AllShaderCompileJobsLock);
        g_Globals.m_AllShaderCompileJobs.push_back(newJob);
    }

    PrintToConsoleAndLogFile(StringFormat("%s_%s: %u permutations", EnumToString(params.m_ShaderType), params.m_ShaderName, totalPermutations));
}
