
static void ProcessShaderPermutations(ConcurrentVector<Shader>& allShaders, json baseJSON)
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

static void ProcessGlobalStructures(json baseJSON)
{
    for (json structJSON : baseJSON["GlobalStructures"])
    {
        GlobalStructure newStruct{ structJSON.at("Name") };
        for (json constantJSON : structJSON["Constants"])
        {
            newStruct.m_Constants.push_back({ constantJSON["Type"], constantJSON["Name"] });
        }

        PrintAutogenFilesForGlobalStructure(newStruct);
    }
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
        for (json resourceJSON : shaderInput["Resources"])
        {
            const std::string resourceTypeStr = resourceJSON.at("Type");
            const ResourceType resourceType = gs_ResourceTraitsMap.at(resourceTypeStr).m_Type;

            std::string UAVStructureType;
            json UAVStructureTypeJSON = resourceJSON["UAVStructureType"];
            if (!UAVStructureTypeJSON.empty())
            {
                UAVStructureType = UAVStructureTypeJSON.get<std::string>();
                inputs.m_GlobalStructureDependencies.insert(GlobalStructure{ UAVStructureType });
            }

            inputs.m_Resources[resourceType].push_back({ resourceTypeStr, UAVStructureType, resourceJSON.at("Register"), resourceJSON.at("Name") });
        }

        // consts
        for (json constsJSON : shaderInput["Consts"])
        {
            inputs.m_Consts.push_back({ constsJSON.at("Type"), constsJSON.at("Name"), constsJSON.at("Value") });
        }

        PrintAutogenFilesForShaderInput(inputs);
    }
}

void ProcessShaderJSONFile(ConcurrentVector<Shader>& allShaders, const std::string& file)
{
    PrintToConsoleAndLogFile(StringFormat("Processing '%s'...", GetFileNameFromPath(file).c_str()));

    try
    {
        CFileWrapper jsonFile{ file.c_str() };
        json baseJSON = json::parse(jsonFile);

        ProcessShaderPermutations(allShaders, baseJSON);
        ProcessGlobalStructures(baseJSON);
        ProcessShaderInputs(baseJSON);
    }
    catch (const std::exception& e)
    {
        PrintToConsoleAndLogFile(e.what());
        gs_CompileFailureDetected = true;
    }
}
