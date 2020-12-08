
static void InitializeGlobals()
{
    const std::string tmpDir = StringFormat("%s..\\tmp\\", GetApplicationDirectory());

    // init dirs
    g_GlobalDirs.m_ShadersSrcDir                             = StringFormat("%s..\\src\\graphic\\shaders", GetApplicationDirectory());
    g_GlobalDirs.m_ShadersJSONSrcDir                         = StringFormat("%s..\\shadercompiler\\shaders", GetApplicationDirectory());
    g_GlobalDirs.m_ShadersTmpDir                             = StringFormat("%sshaders\\", tmpDir.c_str());
    g_GlobalDirs.m_ShadersTmpAutoGenDir                      = g_GlobalDirs.m_ShadersTmpDir + "autogen\\";
    g_GlobalDirs.m_ShadersTmpCPPAutogenDir                   = g_GlobalDirs.m_ShadersTmpAutoGenDir + "cpp\\";
    g_GlobalDirs.m_ShadersTmpHLSLAutogenDir                  = g_GlobalDirs.m_ShadersTmpAutoGenDir + "hlsl\\";
    g_GlobalDirs.m_ShadersTmpPDBAutogenDir                   = g_GlobalDirs.m_ShadersTmpAutoGenDir + "pdb\\";
    g_GlobalDirs.m_ShadersTmpCPPShaderInputsAutogenDir       = g_GlobalDirs.m_ShadersTmpAutoGenDir + "cpp\\ShaderInputs\\";
    g_GlobalDirs.m_ShadersTmpCPPShaderPermutationsAutogenDir = g_GlobalDirs.m_ShadersTmpAutoGenDir + "cpp\\ShaderPermutations\\";

    CreateDirectory(StringFormat("%s..\\tmp", GetApplicationDirectory()), nullptr);
    CreateDirectory(g_GlobalDirs.m_ShadersTmpDir.c_str(), nullptr);
    CreateDirectory(g_GlobalDirs.m_ShadersTmpAutoGenDir.c_str(), nullptr);
    CreateDirectory(g_GlobalDirs.m_ShadersTmpCPPAutogenDir.c_str(), nullptr);
    CreateDirectory(g_GlobalDirs.m_ShadersTmpHLSLAutogenDir.c_str(), nullptr);
    CreateDirectory(g_GlobalDirs.m_ShadersTmpPDBAutogenDir.c_str(), nullptr);
    CreateDirectory(g_GlobalDirs.m_ShadersTmpCPPShaderInputsAutogenDir.c_str(), nullptr);
    CreateDirectory(g_GlobalDirs.m_ShadersTmpCPPShaderPermutationsAutogenDir.c_str(), nullptr);

    const std::string jsonDir = StringFormat("%sshadercompiler_options.json", tmpDir.c_str());
    CFileWrapper jsonFile{ jsonDir.c_str() };

    // use default shader if no json file found
    if (!jsonFile)
    {
        json defaultJSON;
        defaultJSON["ShaderModel"] = gs_ShaderModelToUse;
        
        std::ofstream outStream{ jsonDir };
        outStream << defaultJSON;
        return;
    }

    try
    {
        json optionsJSON = json::parse(jsonFile);
        gs_ShaderModelToUse = optionsJSON.at("ShaderModel");
    }
    catch (const std::exception& e)
    {
        PrintToConsoleAndLogFile(e.what());
        system("pause");
        std::exit(-1);
    }
}

static void InitializeShaderFilesDependencies()
{
    std::vector<std::string> allShaderFiles;
    GetFilesInDirectory(allShaderFiles, g_GlobalDirs.m_ShadersSrcDir);

    std::unordered_map<std::string, std::unordered_set<std::string>> allFilesDependencies;
    for (std::string_view shaderFileDir : allShaderFiles)
    {
        std::unordered_set<std::string>& dependencies = allFilesDependencies[GetFileNameFromPath(shaderFileDir.data())];

        CFileWrapper file{ shaderFileDir.data() };

        std::string buffer;
        buffer.resize(BBE_KB(1));
        while (fgets(buffer.data(), buffer.size(), file))
        {
            static const std::regex IncludeRegex{ "^\\s*\\#include\\s+[\" < ]([^ \">]+)*[\" > ]" };

            std::smatch matchResult;
            std::regex_search(buffer, matchResult, IncludeRegex);

            if (matchResult.empty())
                continue;

            dependencies.insert(matchResult[1]);
        }
    }
}

int main()
{
    // first, Init the logger
    Logger::GetInstance().Initialize("../bin/shadercompiler_output.txt");

    InitializeGlobals();

    // TODO
    //InitializeShaderFilesDependencies();

    std::vector<std::string> allJSONs;
    GetFilesInDirectory(allJSONs, g_GlobalDirs.m_ShadersJSONSrcDir);

    tf::Executor executor;
    tf::Taskflow tf;

    ConcurrentVector<Shader> allShaders;

    // Parse all JSON files Multi-Threaded
    for (const std::string& file : allJSONs)
    {
        tf.emplace([&] { ProcessShaderJSONFile(allShaders, file); });
    }
    executor.run(tf).wait();
    tf.clear();

    // Compile all shader permutations Multi-Threaded
    for (Shader& shader : allShaders)
    {
        for (uint32_t i = 0; i < GfxShaderType_Count; ++i)
        {
            for (Shader::Permutation& permutation : shader.m_Permutations[i])
            {
                tf.emplace([&, i]
                    {
                        try
                        {
                            CompilePermutation(shader, permutation, (GfxShaderType)i);
                        }
                        catch (const std::exception& e)
                        {
                            PrintToConsoleAndLogFile(e.what());
                            gs_CompileFailureDetected = true;
                        }
                    });
            }
        }
    }
    executor.run(tf).wait();

    if (gs_CompileFailureDetected)
        PrintToConsoleAndLogFile("\n\nCompile failure(s) detected!\n\n");
    else
    {
        PrintToConsoleAndLogFile("\n\nAll Shader Compilation Succeeded!\n\n");

        // sort all shaders to have consistent shaderbytecodes.h output
        std::sort(allShaders.begin(), allShaders.end());

        PrintAutogenByteCodeHeadersFile(allShaders);
    }

    system("pause");
    return 0;
}
