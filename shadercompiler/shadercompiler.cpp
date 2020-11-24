
static void InitializeGlobals()
{
    const std::string tmpDir = StringFormat("%s..\\tmp\\", GetApplicationDirectory());

    // init dirs
    g_GlobalDirs.m_ShadersSrcDir                             = StringFormat("%s..\\shadercompiler\\shaders", GetApplicationDirectory());
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

static void ProcessShaderPermutations(concurrency::concurrent_vector<Shader>& allShaders, json baseJSON)
{
    json shaderJSON = baseJSON["Shader"];
    if (shaderJSON.empty())
        return;

    Shader& newShader = *allShaders.push_back(Shader{});
    newShader.m_Name = shaderJSON.at("ShaderName");
    newShader.m_FileName = shaderJSON.at("FileName");
    newShader.m_BaseShaderID = std::hash<std::string>{}(newShader.m_Name);

    // Add entry points
    for (uint32_t i = 0; i < GfxShaderType_Count; ++i)
    {
        const GfxShaderType shaderType = (GfxShaderType)i;
        const std::string entryPointName = StringFormat("%sEntryPoint", EnumToString(shaderType));
        if (shaderJSON.contains(entryPointName))
        {
            newShader.m_EntryPoints[i] = shaderJSON.at(entryPointName);

            // Base shader permutation always exists
            const std::string shaderObjCodeFileDir = StringFormat("%s%s_%s.h", g_GlobalDirs.m_ShadersTmpDir.c_str(), EnumToString(shaderType), newShader.m_Name.c_str());
            newShader.m_Permutations[shaderType].push_back({ 0, newShader.m_Name, shaderObjCodeFileDir });
        }
    }

    // add Shader Permutations
    json shaderPermsJSON = shaderJSON["ShaderPermutations"];
    for (uint32_t i = 0; i < GfxShaderType_Count; ++i)
    {
        const GfxShaderType shaderType = (GfxShaderType)i;
        json shaderPermsForTypeJSON = shaderPermsJSON[EnumToString(shaderType)];

        // no permutations for this shader type. go to next type
        if (shaderPermsForTypeJSON.empty())
            continue;

        AddValidPermutations(newShader, shaderType, shaderPermsForTypeJSON);
    }

    PrintAutogenFileForShaderPermutationStructs(newShader);
}

static void ProcessShaderInputs(json baseJSON)
{
    for (json shaderInput : baseJSON["ShaderInputs"])
    {
        ShaderInputs inputs{ shaderInput.at("Name") };

        // Constant Buffer
        json cBufferJSON = shaderInput["ConstantBuffer"];
        if (!cBufferJSON.empty())
        {
            inputs.m_ConstantBuffer.m_Register = cBufferJSON.at("Register");

            uint32_t totalBytes = 0;
            for (json constantJSON : cBufferJSON.at("Constants"))
            {
                inputs.m_ConstantBuffer.m_Constants.push_back({ constantJSON.at("Type"), constantJSON.at("Name") });
                totalBytes += gs_TypesTraitsMap.at(inputs.m_ConstantBuffer.m_Constants.back().m_Type).m_SizeInBytes;
            }

            // Add padding if necessary
            const uint32_t numPadVars = (AlignUp(totalBytes, 16) - totalBytes) / sizeof(uint32_t);
            for (uint32_t i = 0; i < numPadVars; ++i)
            {
                inputs.m_ConstantBuffer.m_Constants.push_back({ "uint", StringFormat("PADDING_%d", i) });
            }
        }

        // SRVs, UAVs
        for (json resource : shaderInput["Resources"])
        {
            const std::string& resourceTypeStr = resource.at("Type");
            const ResourceType resourceType = gs_ResourceTraitsMap.at(resourceTypeStr).m_Type;

            inputs.m_Resources[resourceType].push_back({ resourceTypeStr, resource.at("Register"), resource.at("Name") });
        }

        PrintAutogenFilesForShaderInput(inputs);
    }
}

static void ProcessShaderJSONFile(concurrency::concurrent_vector<Shader>& allShaders, const std::string& file)
{
    PrintToConsoleAndLogFile(StringFormat("Processing '%s'...", GetFileNameFromPath(file).c_str()));

    CFileWrapper jsonFile{ file.c_str() };
    json baseJSON = json::parse(jsonFile);

    ProcessShaderPermutations(allShaders, baseJSON);
    ProcessShaderInputs(baseJSON);
}

int main()
{
    // first, Init the logger
    Logger::GetInstance().Initialize("../bin/shadercompiler_output.txt");

    InitializeGlobals();

    std::vector<std::string> allJSONs;
    GetFilesInDirectory(allJSONs, g_GlobalDirs.m_ShadersSrcDir.c_str());

    tf::Executor executor;
    tf::Taskflow tf;

    concurrency::concurrent_vector<Shader> allShaders;
    allShaders.reserve(allJSONs.size());

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
                tf.emplace([&, i] { CompilePermutation(shader, permutation, (GfxShaderType)i); });
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
