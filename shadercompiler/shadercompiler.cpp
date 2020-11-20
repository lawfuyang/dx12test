
static void InitializeGlobals()
{
    // init dirs
    g_GlobalDirs.m_ShadersTmpDir                             = GetApplicationDirectory() + "..\\tmp\\shaders\\";
    g_GlobalDirs.m_ShadersTmpAutoGenDir                      = g_GlobalDirs.m_ShadersTmpDir + "autogen\\";
    g_GlobalDirs.m_ShadersTmpCPPAutogenDir                   = g_GlobalDirs.m_ShadersTmpAutoGenDir + "cpp\\";
    g_GlobalDirs.m_ShadersTmpHLSLAutogenDir                  = g_GlobalDirs.m_ShadersTmpAutoGenDir + "hlsl\\";
    g_GlobalDirs.m_ShadersTmpCPPShaderInputsAutogenDir       = g_GlobalDirs.m_ShadersTmpAutoGenDir + "cpp\\ShaderInputs\\";
    g_GlobalDirs.m_ShadersTmpCPPShaderPermutationsAutogenDir = g_GlobalDirs.m_ShadersTmpAutoGenDir + "cpp\\ShaderPermutations\\";

    CreateDirectory(StringFormat("%s..\\tmp", GetApplicationDirectory().c_str()), nullptr);
    CreateDirectory(g_GlobalDirs.m_ShadersTmpDir.c_str(), nullptr);
    CreateDirectory(g_GlobalDirs.m_ShadersTmpAutoGenDir.c_str(), nullptr);
    CreateDirectory(g_GlobalDirs.m_ShadersTmpCPPAutogenDir.c_str(), nullptr);
    CreateDirectory(g_GlobalDirs.m_ShadersTmpHLSLAutogenDir.c_str(), nullptr);
    CreateDirectory(g_GlobalDirs.m_ShadersTmpCPPShaderInputsAutogenDir.c_str(), nullptr);
    CreateDirectory(g_GlobalDirs.m_ShadersTmpCPPShaderPermutationsAutogenDir.c_str(), nullptr);
}

int main()
{
    // first, Init the logger
    Logger::GetInstance().Initialize("../bin/shadercompiler_output.txt");

    InitializeGlobals();

    std::vector<std::string> allJSONs;
    GetFilesInDirectory(allJSONs, StringFormat("%s%s", GetApplicationDirectory().c_str(), "..\\shadercompiler\\shaders"));

    // tf::Executor executor;

    concurrency::concurrent_vector<Shader> allShaders;
    allShaders.reserve(allJSONs.size());
    for (const std::string& file : allJSONs)
    {
        PrintToConsoleAndLogFile(StringFormat("Processing '%s'...", GetFileNameFromPath(file).c_str()));

        CFileWrapper jsonFile{ file.c_str() };
        json baseJSON = json::parse(jsonFile);

        json shaderJSON = baseJSON["Shader"];
        if (!shaderJSON.empty())
        {
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
                    newShader.m_Permutations[shaderType].push_back({ 0, newShader.m_Name });
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

                PermutationsProcessingContext context{ newShader, shaderType };

                // get all permutation macro define strings
                for (const json permutationJSON : shaderPermsForTypeJSON.at("Permutations"))
                {
                    context.m_AllPermutationsDefines.push_back(permutationJSON.get<std::string>());
                }

                // get all permutation rules
                for (const json ruleJSON : shaderPermsForTypeJSON["Rules"])
                {
                    PermutationsProcessingContext::RuleProperty& newProperty = context.m_RuleProperties.emplace_back();
                    newProperty.m_Rule = StringToPermutationRule(ruleJSON.at("Rule"));

                    for (const json affectedBitsJSON : ruleJSON.at("AffectedBits"))
                        newProperty.m_AffectedBits |= (1 << affectedBitsJSON.get<uint32_t>());
                }

                AddValidPermutations(context);
            }

            PrintAutogenFileForShaderPermutationStructs(newShader);
        }

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

    // Finally, compile all shader permutations
    for (Shader& shader : allShaders)
    {
        for (uint32_t i = 0; i < GfxShaderType_Count; ++i)
        {
            for (Shader::Permutation& permutation : shader.m_Permutations[i])
            {
                CompilePermutation(shader, permutation, (GfxShaderType)i);
            }
        }
    }

    // sort all shaders to have consistent shaderbytecodes.h output
    std::sort(allShaders.begin(), allShaders.end());

    PrintAutogenByteCodeHeadersFile(allShaders);

    if (gs_CompileFailureDetected)
        PrintToConsoleAndLogFile("\n\nCompile failure(s) detected!\n\n");
    else
        PrintToConsoleAndLogFile("\n\nAll Shader Compilation Succeeded!\n\n");

    system("pause");
    return 0;
}
