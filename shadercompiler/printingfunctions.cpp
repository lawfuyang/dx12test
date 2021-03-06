
void PrintToConsoleAndLogFile(std::string_view str)
{
    printf("%s\n", str.data());
    g_Log.info("{}", str.data());
}

static void OverrideExistingFileIfNecessary(const std::string& generatedString, const std::string& dirFull)
{
    // write to tmp file to retrieve hash
    const std::string tmpOutputDir = StringFormat("%stmp_%d.h", g_GlobalDirs.m_ShadersTmpAutoGenDir.c_str(), std::this_thread::get_id());
    {
        CFileWrapper tmpFile{ tmpOutputDir.c_str(), false };
        fprintf(tmpFile, "%s", generatedString.c_str());
    }

    // hash both existing and newly generated contents
    const std::size_t existingHash = GetFileContentsHash(dirFull.c_str());
    const std::size_t newHash = GetFileContentsHash(tmpOutputDir.c_str());

    // if hashes are different, over ride with new contents
    if (existingHash != newHash)
    {
        PrintToConsoleAndLogFile(StringFormat("hash different for '%s'... over-riding with new contents", dirFull.c_str()));
        ::MoveFileExA(tmpOutputDir.c_str(), dirFull.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
    }
    else
    {
        PrintToConsoleAndLogFile(StringFormat("No change detected for '%s'", dirFull.c_str()));
    }
    ::DeleteFileA(tmpOutputDir.c_str());
}

static void PrintShaderInputCPPFile(const ShaderInputs& inputs)
{
    std::string generatedString;
    generatedString += "#pragma once\n\n";
    generatedString += "namespace AutoGenerated {\n\n";

    generatedString += StringFormat("struct %s {\n", inputs.m_Name.c_str());
    generatedString += StringFormat("    inline static const char* Name = \"%s GfxConstantBuffer\";\n", inputs.m_Name.c_str());
    generatedString += StringFormat("    static const uint32_t ConstantBufferRegister = %d;\n", inputs.m_ConstantBuffer.m_Register);
    generatedString += StringFormat("    static const uint32_t NbSRVs = %d;\n", inputs.m_Resources[SRV].size());
    generatedString += StringFormat("    static const uint32_t NbUAVs = %d;\n\n", inputs.m_Resources[UAV].size());

    // Constants
    for (const ShaderConstant& var : inputs.m_Consts)
    {
        generatedString += StringFormat("    static const %s %s = %s;\n", gs_TypesTraitsMap.at(var.m_Type).m_CPPType, var.m_Name.c_str(), var.m_ConstValue.c_str());
    }
    generatedString += "\n";

    // SRVs, UAVs
    auto PrintOffsetsStruct = [&inputs, &generatedString](ResourceType type)
    {
        if (inputs.m_Resources[type].size())
        {
            const char* offsetsStr = (type == SRV) ? "SRVOffsets" : "UAVOffsets";

            generatedString += StringFormat("    struct %s {\n", offsetsStr);
            for (const ShaderInputs::Resource& resource : inputs.m_Resources[type])
            {
                generatedString += StringFormat("        static const uint32_t %s = %d;\n", resource.m_Name.c_str(), resource.m_Register);
            }
            generatedString += "    };\n\n";
        }
    };

    PrintOffsetsStruct(SRV);
    PrintOffsetsStruct(UAV);

    // Constants
    for (const ShaderConstant& var : inputs.m_ConstantBuffer.m_Constants)
    {
        generatedString += StringFormat("    %s m_%s = {};\n", gs_TypesTraitsMap.at(var.m_Type).m_CPPType, var.m_Name.c_str());
    }

    generatedString += "};\n";
    generatedString += "}\n";

    std::string outputDir = g_GlobalDirs.m_ShadersTmpCPPShaderInputsAutogenDir + inputs.m_Name.c_str() + ".h";
    OverrideExistingFileIfNecessary(generatedString, outputDir);
}

static void PrintShaderInputHLSLFile(const ShaderInputs& inputs)
{
    std::string generatedString;

    generatedString += StringFormat("#ifndef __%s_H__\n", inputs.m_Name.c_str());
    generatedString += StringFormat("#define __%s_H__\n\n", inputs.m_Name.c_str());

    // print #include(s) for Global Structures
    for (const GlobalStructure& s : inputs.m_GlobalStructureDependencies)
    {
        generatedString += StringFormat("// struct %s\n", s.m_Name.c_str());
        generatedString += StringFormat("#include \"autogen/hlsl/%s.h\"\n\n", s.m_Name.c_str());
    }

    // CBV, SRVs & UAVs bindings
    if (inputs.m_ConstantBuffer.m_Constants.size())
    {
        generatedString += "// CBV Bindings\n";
        generatedString += StringFormat("struct %s__ConstantsStruct {\n", inputs.m_Name.c_str());
        for (const ShaderConstant& var : inputs.m_ConstantBuffer.m_Constants)
        {
            generatedString += StringFormat("    %s m_%s;\n", var.m_Type.c_str(), var.m_Name.c_str());
        }
        generatedString += "};\n";

        generatedString += StringFormat("ConstantBuffer<%s__ConstantsStruct> %s__Constants : register(b%d);\n", inputs.m_Name.c_str(), inputs.m_Name.c_str(), inputs.m_ConstantBuffer.m_Register);
    }

    for (uint32_t i = 0; i < ResourceType_Count; ++i)
    {
        generatedString += StringFormat("\n// %s Bindings\n", EnumToString((ResourceType)i));
        for (const ShaderInputs::Resource& resource : inputs.m_Resources[i])
        {
            const std::string UAVStructureTypeStr = gs_ResourceTraitsMap.at(resource.m_Type).m_IsStructured ? StringFormat("<%s>", resource.m_UAVStructureType.c_str()) : "";
            generatedString += StringFormat("%s%s %s__%s : register(%s%d);\n",
                resource.m_Type.c_str(), UAVStructureTypeStr.c_str(), inputs.m_Name.c_str(), resource.m_Name.c_str(), (i == SRV) ? "t" : "u", resource.m_Register);
        }
    }
    generatedString += "\n";

    // Public Interface
    generatedString += "// Public Interface\n";
    generatedString += StringFormat("struct %s {\n", inputs.m_Name.c_str());

    // Constants
    generatedString += "    // Consts\n";
    for (const ShaderConstant& var : inputs.m_Consts)
    {
        generatedString += StringFormat("    static const %s %s = %s;\n", var.m_Type.c_str(), var.m_Name.c_str(), var.m_ConstValue.c_str());
    }

    generatedString += "\n    // CBV Interfaces\n";
    for (const ShaderConstant& var : inputs.m_ConstantBuffer.m_Constants)
    {
        generatedString += StringFormat("    static %s Get%s() { return %s__Constants.m_%s; }\n", var.m_Type.c_str(), var.m_Name.c_str(), inputs.m_Name.c_str(), var.m_Name.c_str());
    }
    for (uint32_t i = 0; i < ResourceType_Count; ++i)
    {
        generatedString += StringFormat("\n    // %s Interfaces\n", EnumToString((ResourceType)i));
        for (const ShaderInputs::Resource& resource : inputs.m_Resources[i])
        {
            const std::string UAVStructureTypeStr = gs_ResourceTraitsMap.at(resource.m_Type).m_IsStructured ? StringFormat("<%s>", resource.m_UAVStructureType.c_str()) : "";
            generatedString += StringFormat("    static %s%s Get%s() { return %s__%s; };\n", 
                resource.m_Type.c_str(), UAVStructureTypeStr.c_str(), resource.m_Name.c_str(), inputs.m_Name.c_str(), resource.m_Name.c_str());
        }
    }
    generatedString += "};\n\n";

    generatedString += StringFormat("#endif // #define __%s_H__\n", inputs.m_Name.c_str());

    const std::string outputDir = g_GlobalDirs.m_ShadersTmpHLSLAutogenDir + inputs.m_Name.c_str() + ".h";
    OverrideExistingFileIfNecessary(generatedString, outputDir);
}

void PrintAutogenFilesForShaderInput(const ShaderInputs& inputs)
{
    PrintShaderInputCPPFile(inputs);
    PrintShaderInputHLSLFile(inputs);
}

static void PrintGlobalStructureCPPFile(const GlobalStructure& s)
{
    std::string generatedString;
    generatedString += "#pragma once\n\n";
    generatedString += "namespace AutoGenerated {\n\n";

    generatedString += StringFormat("struct %s {\n", s.m_Name.c_str());
    for (const ShaderConstant& var : s.m_Constants)
    {
        generatedString += StringFormat("    %s m_%s;\n", gs_TypesTraitsMap.at(var.m_Type).m_CPPType, var.m_Name.c_str());
    }
    generatedString += "};\n";
    generatedString += "}\n";

    std::string outputDir = g_GlobalDirs.m_ShadersTmpCPPShaderInputsAutogenDir + s.m_Name.c_str() + ".h";
    OverrideExistingFileIfNecessary(generatedString, outputDir);
}

static void PrintGlobalStructureHLSLFile(const GlobalStructure& s)
{
    std::string generatedString;
    generatedString += StringFormat("#ifndef __%s_H__\n", s.m_Name.c_str());
    generatedString += StringFormat("#define __%s_H__\n\n", s.m_Name.c_str());

    generatedString += StringFormat("struct %s {\n", s.m_Name.c_str());
    for (const ShaderConstant& var : s.m_Constants)
    {
        generatedString += StringFormat("    %s m_%s;\n", var.m_Type.c_str(), var.m_Name.c_str());
    }
    generatedString += "};\n";

    generatedString += StringFormat("#endif // #define __%s_H__\n", s.m_Name.c_str());

    std::string outputDir = g_GlobalDirs.m_ShadersTmpHLSLAutogenDir + s.m_Name.c_str() + ".h";
    OverrideExistingFileIfNecessary(generatedString, outputDir);
}

void PrintAutogenFilesForGlobalStructure(const GlobalStructure& s)
{
    PrintGlobalStructureCPPFile(s);
    PrintGlobalStructureHLSLFile(s);
}

void PrintAutogenFileForShaderPermutationStructs(const Shader& shader)
{
    std::string generatedString;
    generatedString += "#pragma once\n\n";
    generatedString += "namespace Shaders {\n\n";

    for (uint32_t i = 0; i < GfxShaderType_Count; ++i)
    {
        if (shader.m_EntryPoints[i].empty())
            continue;

        const char* shaderTypeStr = EnumToString((GfxShaderType)i);

        generatedString += StringFormat("struct %s_%s {\n", shaderTypeStr, shader.m_Name.c_str());
        generatedString += StringFormat("    static const uint32_t BaseShaderID = %u;\n", shader.m_BaseShaderID);
        generatedString += StringFormat("    static const GfxShaderType ShaderType = %s;\n", shaderTypeStr);
        generatedString += "    union {\n";
        generatedString += "        struct {\n";

        // start at '1', as '0' is the base permutation
        for (uint32_t j = 1; j < shader.m_Permutations[i].size(); ++j) 
        {
            // get rid of the shader name to retrieve the permutation name
            const std::string permutationName = std::string{ shader.m_Permutations[i][j].m_Name.begin() + shader.m_Name.length(), shader.m_Permutations[i][j].m_Name.end() };
            generatedString += StringFormat("           bool %s : 1;\n", permutationName.c_str());
        }

        generatedString += "        } Permutations;\n";
        generatedString += "        uint32_t m_ShaderKey = 0;\n";
        generatedString += "    };\n";
        generatedString += "};\n";
    }

    generatedString += "}\n";

    const std::string outputDir = StringFormat("%s%s.h", g_GlobalDirs.m_ShadersTmpCPPShaderPermutationsAutogenDir.c_str(), shader.m_Name.c_str());
    OverrideExistingFileIfNecessary(generatedString, outputDir);
}

void PrintAutogenByteCodeHeadersFile(const ConcurrentVector<Shader>& allShaders)
{
    std::string generatedString;
    generatedString += "#pragma once\n\n";

    // include all shader byte codes
    for (const Shader& shader : allShaders)
    {
        for (uint32_t i = 0; i < GfxShaderType_Count; ++i)
        {
            for (const Shader::Permutation& permutation : shader.m_Permutations[i])
            {
                generatedString += StringFormat("#include \"%s\"\n", permutation.m_ShaderObjCodeFileDir.c_str());
            }
        }
    }

    generatedString += "namespace AutoGenerated {\n\n";
    generatedString += "struct ShaderData {\n";
    generatedString += "    const unsigned char* m_ByteCodeArray;\n";
    generatedString += "    const uint32_t m_ByteCodeSize;\n";
    generatedString += "    const uint32_t m_ShaderKey;\n";
    generatedString += "    const uint32_t m_BaseShaderID;\n";
    generatedString += "    const GfxShaderType m_ShaderType;\n";
    generatedString += "};\n";

    generatedString += "static const ShaderData gs_AllShadersData[] = {\n";
    for (const Shader& shader : allShaders)
    {
        for (uint32_t i = 0; i < GfxShaderType_Count; ++i)
        {
            const char* shaderTypeStr = EnumToString((GfxShaderType)i);
            for (const Shader::Permutation& permutation : shader.m_Permutations[i])
            {
                const std::string byteCodeVarName = StringFormat("%s_%s_ObjCode", shaderTypeStr, permutation.m_Name.c_str());
                generatedString += StringFormat("    {\n");
                generatedString += StringFormat("        %s,\n", byteCodeVarName.c_str());
                generatedString += StringFormat("        _countof(%s),\n", byteCodeVarName.c_str());
                generatedString += StringFormat("        %u,\n", permutation.m_ShaderKey);
                generatedString += StringFormat("        %u,\n", shader.m_BaseShaderID);
                generatedString += StringFormat("        %s,\n", EnumToString((GfxShaderType)i));
                generatedString += StringFormat("    },\n");
            }
        }
    }
    generatedString += "};\n";
    generatedString += "}\n";

    OverrideExistingFileIfNecessary(generatedString, StringFormat("%sshaderbytecodes.h", g_GlobalDirs.m_ShadersTmpDir.c_str()));
}
