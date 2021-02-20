
static bool RunDXCCompiler(const std::string& inputCommandLine, std::string& errorMsg)
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

    const std::string commandLine = StringFormat("%s..\\extern\\dxc\\dxc.exe %s", GetApplicationDirectory(), inputCommandLine.c_str());

    // uncomment to see full cmdline sent to DXC
    // PrintToConsoleAndLogFile(commandLine);

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
        errorMsg = GetLastErrorAsString();
        return false;
    }

    ::CloseHandle(processInfo.hProcess);
    ::CloseHandle(processInfo.hThread);
    ::CloseHandle(hChildStd_OUT_Wr);

    DWORD totalRead = 0;
    errorMsg.resize(BBE_MB(1)); // Up to 1 MB of output
    for (;;)
    {
        ::Sleep(1); // Sleep every loop iteration so the thread does not read 4/5 chars per "::ReadFile"

        DWORD dwRead = 0;
        success = ::ReadFile(hChildStd_OUT_Rd, errorMsg.data(), 1024, &dwRead, NULL);
        totalRead += dwRead;
        if (!success || dwRead == 0) break;
    }

    return totalRead == 0;
}

void CompilePermutation(const Shader& parentShader, Shader::Permutation& permutation, GfxShaderType shaderType)
{
    if (gs_CompileFailureDetected)
        return;

    const char* shaderTypeStr = EnumToString(shaderType);

    const std::string shaderNameWithPrefix = StringFormat("%s_%s", shaderTypeStr, permutation.m_Name.c_str());

    std::string shaderModelStr = StringFormat("%s_%s", shaderTypeStr, gs_ShaderModelToUse.c_str());
    StringUtils::ToLower(shaderModelStr);
    shaderModelStr = StringFormat(" -T %s", shaderModelStr.c_str());

    std::string commandLine = StringFormat("%s\\%s", g_GlobalDirs.m_ShadersSrcDir.c_str(), parentShader.m_FileName.c_str());
    commandLine += StringFormat(" -E %s", parentShader.m_EntryPoints[shaderType].c_str());
    commandLine += shaderModelStr;
    commandLine += StringFormat(" -Fh %s", permutation.m_ShaderObjCodeFileDir.c_str());
    commandLine += StringFormat(" -Vn %s_ObjCode", shaderNameWithPrefix.c_str());
    commandLine += " -nologo ";
    commandLine += " -WX ";
    commandLine += " -all_resources_bound "; // TODO: Dynamic Iddexing
    commandLine += StringFormat(" -I%s", g_GlobalDirs.m_ShadersTmpDir.c_str());

    for (const std::string& define : permutation.m_Defines)
    {
        commandLine += StringFormat(" -D%s ", define.c_str());
    }

    // debugging stuff
    commandLine += StringFormat(" -Zi -Qembed_debug -Fd %s%s.pdb", g_GlobalDirs.m_ShadersTmpPDBAutogenDir.c_str(), shaderNameWithPrefix.c_str());
    
    std::string errorMsg;
    if (!RunDXCCompiler(commandLine, errorMsg))
    {
        PrintToConsoleAndLogFile(StringFormatBig("%s: %s", parentShader.m_FileName.c_str(), errorMsg.c_str()));
        gs_CompileFailureDetected = true;
        return;
    }

    PrintToConsoleAndLogFile(StringFormat("Compiled %s", shaderNameWithPrefix.c_str()));
}
