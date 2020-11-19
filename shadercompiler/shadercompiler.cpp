
BBE_OPTIMIZE_OFF;

tf::Executor g_Executor;

struct GlobalsInitializer
{
    GlobalsInitializer()
    {
        // init dirs
        g_Globals.m_AppDir                                    = GetApplicationDirectory();
        g_Globals.m_ShadersJSONDir                            = g_Globals.m_AppDir + "..\\shadercompiler\\shaders";
        g_Globals.m_ShadersTmpDir                             = g_Globals.m_AppDir + "..\\tmp\\shaders\\";
        g_Globals.m_ShadersTmpHLSLAutogenDir                  = g_Globals.m_ShadersTmpDir + "autogen\\hlsl\\";
        g_Globals.m_ShadersTmpCPPShaderInputsAutogenDir       = g_Globals.m_ShadersTmpDir + "autogen\\cpp\\ShaderInputs\\";
        g_Globals.m_ShadersTmpCPPShaderPermutationsAutogenDir = g_Globals.m_ShadersTmpDir + "autogen\\cpp\\ShaderPermutations\\";
        g_Globals.m_ShadersDir                                = g_Globals.m_AppDir + "..\\src\\graphic\\shaders\\";
        g_Globals.m_ShadersByteCodesDir                       = g_Globals.m_ShadersTmpDir + "shaderbytecodes.h";
        g_Globals.m_DXCDir                                    = g_Globals.m_AppDir + "..\\extern\\dxc\\dxc.exe";

        CreateDirectory(StringFormat("%s..\\tmp", g_Globals.m_AppDir.c_str()).c_str(), nullptr);
        CreateDirectory(g_Globals.m_ShadersTmpDir.c_str(), nullptr);
        CreateDirectory((g_Globals.m_ShadersTmpDir + "autogen\\").c_str(), nullptr);
        CreateDirectory(g_Globals.m_ShadersTmpHLSLAutogenDir.c_str(), nullptr);
        CreateDirectory(g_Globals.m_ShadersTmpCPPShaderInputsAutogenDir.c_str(), nullptr);
        CreateDirectory(g_Globals.m_ShadersTmpCPPShaderPermutationsAutogenDir.c_str(), nullptr);
    }
};
static const GlobalsInitializer g_GlobalsInitializer;

int main()
{
    // first, Init the logger
    Logger::GetInstance().Initialize("../bin/shadercompiler_output.txt");

    std::vector<std::string> allJSONs;
    GetFilesInDirectory(allJSONs, g_Globals.m_ShadersJSONDir);

    concurrency::concurrent_vector<Shader> allShaders;
    allShaders.reserve(allJSONs.size());
    for (const std::string& file : allJSONs)
    {
        PrintToConsoleAndLogFile(StringFormat("Processing '%s'...", GetFileNameFromPath(file).c_str()));

        CFileWrapper jsonFile{ file.c_str() };

        try
        {
            json baseJSON = json::parse(jsonFile);

            json shaderJSON = baseJSON["Shader"];
            if (!shaderJSON.empty())
            {
                Shader& newShader = *allShaders.push_back(Shader{});
                newShader.m_Name = shaderJSON.at("ShaderName");
                newShader.m_FileName = shaderJSON.at("FileName");
                newShader.m_ID = std::hash<std::string>{}(newShader.m_Name);

                // Add entry points
                for (uint32_t i = 0; i < GfxShaderType_Count; ++i)
                {
                    const GfxShaderType shaderType = (GfxShaderType)i;
                    const std::string entryPointName = StringFormat("%sEntryPoint", EnumToString(shaderType));
                    if (shaderJSON.contains(entryPointName))
                    {
                        newShader.m_EntryPoints[i] = shaderJSON.at(entryPointName);

                        // Base shader permutation always exists
                        newShader.m_Permutations.push_back({ newShader.m_ID, newShader.m_Name, shaderType });
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
                    for (const json permutationJSON : shaderPermsForTypeJSON.at("Defines"))
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
                        inputs.m_ConstantBuffer.m_Constants.push_back({ "uint", StringFormat("PADDING_%d", i).c_str() });
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
        catch (const std::exception& e)
        {
            PrintToConsoleAndLogFile(e.what());
            assert(false);
        }
    }

    // Compile all shader permutations


    if (gs_CompileFailureDetected)
    {
        PrintToConsoleAndLogFile("Compile failure(s) detected!\n\n");
    }
    else
    {
        //PrintGeneratedByteCodeHeadersFile();
        //PrintShaderPermutationStructs();
    }

    system("pause");
    return 0;
}
