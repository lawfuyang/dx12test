
static void InitializeGlobals()
{
    g_GlobalDirs.m_TmpDir                                    = StringFormat("%s..\\tmp\\", GetApplicationDirectory());
    g_GlobalDirs.m_ShadersSrcDir                             = StringFormat("%s..\\src\\graphic\\shaders", GetApplicationDirectory());
    g_GlobalDirs.m_ShadersJSONSrcDir                         = StringFormat("%s..\\shadercompiler\\shaders", GetApplicationDirectory());
    g_GlobalDirs.m_ShadersTmpDir                             = StringFormat("%sshaders\\", g_GlobalDirs.m_TmpDir.c_str());
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

    const std::string jsonDir = StringFormat("%sshadercompiler_options.json", g_GlobalDirs.m_TmpDir.c_str());
    CFileWrapper jsonFile{ jsonDir.c_str() };

    // use default values if no json file found
    if (!jsonFile)
    {
        gs_FullRebuild = true;

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
        PrintToConsoleAndLogFile(StringFormat("shadercompiler_options.json parsing exception: %s", e.what()));
        system("pause");
        std::exit(-1);
    }
}

struct ShaderFileMetaData
{
    bool m_Dirty = false;
    std::size_t m_FullHash = 0;
    ConcurrentUnorderedSet<std::string> m_UniqueDependencies;
};

static void RetrieveFileDependencies(ShaderFileMetaData& fileMetaData, std::string_view fileDirFull)
{
    CFileWrapper file{ fileDirFull.data() };

    std::string buffer;
    buffer.resize(BBE_KB(1));
    while (fgets(buffer.data(), buffer.size(), file))
    {
        static const std::regex IncludeRegex{ "^\\s*\\#include\\s+[\" < ]([^ \">]+)*[\" > ]" };

        std::smatch matchResult;
        std::regex_search(buffer, matchResult, IncludeRegex);

        if (matchResult.empty())
            continue;

        std::string newFileDependency = matchResult[1];
        
        if (newFileDependency.find("autogen/") != std::string::npos)
            newFileDependency = StringFormat("%s%s", g_GlobalDirs.m_ShadersTmpDir.c_str(), newFileDependency.c_str());
        else
            newFileDependency = StringFormat("%s\\%s", g_GlobalDirs.m_ShadersSrcDir.c_str(), newFileDependency.c_str());

        fileMetaData.m_UniqueDependencies.insert(newFileDependency);

        RetrieveFileDependencies(fileMetaData, newFileDependency);
    }
}

static void InitializeShaderFilesDependencies()
{
    const std::string metaDataJSONDir = StringFormat("%sshadercompiler_shaderfiles_metadata.json", g_GlobalDirs.m_TmpDir.c_str());

    std::unordered_map<std::string, std::size_t> exitingFilesHashes; // TODO: FlatMap
    if (CFileWrapper metaDataJSONFile{ metaDataJSONDir.c_str(), true }; 
        metaDataJSONFile)
    {
        json metaDataJSON = json::parse(metaDataJSONFile);

        for (auto it = metaDataJSON.begin(); it != metaDataJSON.end(); ++it)
        {
            exitingFilesHashes[it.key()] = it.value();
        }
    }

    // get all shader hlsl files
    std::vector<std::string> allShaderFiles;
    GetFilesInDirectory(allShaderFiles, g_GlobalDirs.m_ShadersSrcDir);

    // retrieve all dependencies recursively
    std::unordered_map<std::string, ShaderFileMetaData> allShaderFilesMetaData; // TODO: FlatMap
    for (std::string_view shaderFileDir : allShaderFiles)
    {
        const std::string shaderFileName = GetFileNameFromPath(shaderFileDir.data());
        ShaderFileMetaData& thisShaderFileMetaData = allShaderFilesMetaData[shaderFileName];

        RetrieveFileDependencies(thisShaderFileMetaData, shaderFileDir);

        // hash the shader file itself
        thisShaderFileMetaData.m_FullHash = GetFileContentsHash(shaderFileDir.data());
    }

    // hash all dependencies
    json metaDataJSON;
    for (auto& elem : allShaderFilesMetaData)
    {
        // write out file hash first, in case it does not have any nested dependencies
        metaDataJSON[elem.first] = elem.second.m_FullHash;

        for (std::string_view shaderFileDir : elem.second.m_UniqueDependencies)
        {
             boost::hash_combine(elem.second.m_FullHash, GetFileContentsHash(shaderFileDir.data()));
             metaDataJSON[elem.first] = elem.second.m_FullHash;
        }
    }

    // output updated shader files' metadata
    std::ofstream outStream{ metaDataJSONDir };
    outStream << metaDataJSON;
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
                            PrintToConsoleAndLogFile(StringFormat("CompilePermutation exception: %s", e.what()));
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
