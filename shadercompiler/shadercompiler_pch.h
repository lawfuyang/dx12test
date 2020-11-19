#pragma once

static const D3D_SHADER_MODEL gs_HighestD3D12ShaderModel = D3D_SHADER_MODEL_6_6;

static const uint32_t NbShaderBits = 4;

DEFINE_ENUM_WITH_STRING_CONVERSIONS(GfxShaderType, (VS)(PS)(CS));

DEFINE_ENUM_WITH_STRING_CONVERSIONS(ResourceType, (SRV)(UAV));

DEFINE_ENUM_WITH_STRING_CONVERSIONS(PermutationRule, (AnyBitSet)(AllBitsSet)(OnlyOneBitSet)(MaxOneBitSet));

struct HLSLTypesTraits
{
    const char* m_CPPType;
    uint32_t m_SizeInBytes;
};

static const std::unordered_map<std::string_view, HLSLTypesTraits> gs_TypesTraitsMap =
{
    { "int"      , { "int32_t", 4 } },
    { "int2"     , { "bbeVector2I", 8 } },
    { "int3"     , { "bbeVector3I", 12 } },
    { "int4"     , { "bbeVector4I", 16} },
    { "uint"     , { "uint32_t", 4 } },
    { "uint2"    , { "bbeVector2U", 8 } },
    { "uint3"    , { "bbeVector3U", 12 } },
    { "uint4"    , { "bbeVector4U", 16 } },
    { "float"    , { "float", 4 } },
    { "float2"   , { "bbeVector2", 8 } },
    { "float3"   , { "bbeVector3", 12 } },
    { "float4"   , { "bbeVector4", 16 } },
    { "float4x4" , { "bbeMatrix", 64 } },
};

struct ResourceTraits
{
    ResourceType m_Type;
};

static const std::unordered_map<std::string_view, ResourceTraits> gs_ResourceTraitsMap =
{
    { "Texture2D", { SRV } },
};

struct ShaderInputs
{
    struct Constant
    {
        std::string m_Type;
        std::string m_Name;
    };
    struct ConstantBuffer
    {
        uint32_t m_Register = 0xDEADBEEF;
        std::vector<Constant> m_Constants;
    };
    struct Resource
    {
        std::string m_Type;
        uint32_t m_Register;
        std::string m_Name;
    };

    std::string m_Name;
    ConstantBuffer m_ConstantBuffer;

    std::vector<Resource> m_Resources[ResourceType_Count];
};

struct Shader
{
    std::size_t m_ID;
    std::string m_Name;
    std::string m_FileName;
    std::string m_EntryPoints[GfxShaderType_Count];

    struct Permutation
    {
        std::size_t m_ID;
        std::string m_Name;
        GfxShaderType m_Type;
    };
    std::vector<Permutation> m_Permutations;
};

struct PermutationsProcessingContext
{
    struct RuleProperty
    {
        PermutationRule m_Rule;
        uint32_t m_AffectedBits = 0;
    };

    Shader& m_Shader;
    GfxShaderType m_Type;
    std::vector<std::string> m_AllPermutationsDefines;
    std::vector<RuleProperty> m_RuleProperties;
};

void AddValidPermutations(PermutationsProcessingContext& context);
void PrintToConsoleAndLogFile(const std::string& str);
void PrintAutogenFilesForShaderInput(const ShaderInputs& inputs);
void PrintShaderPermutationStructs();
void PrintAutogenByteCodeHeadersFile();

struct ShaderCompileJob
{
    void StartJob();

    GfxShaderType m_ShaderType;
    uint32_t m_BaseShaderID;
    uint32_t m_ShaderKey;
    std::string m_ShaderFilePath;
    std::string m_EntryPoint;
    std::string m_ShaderName;
    std::string m_BaseShaderName;
    std::vector<std::string> m_Defines;

    // These 2 will be set internally
    std::string m_ShaderObjCodeVarName;
    std::string m_ShaderObjCodeFileDir;
};

struct DXCProcessWrapper
{
    DXCProcessWrapper(const std::string& inputCommandLine, const ShaderCompileJob& job);
};

struct PopulateJobParams
{
    std::string m_ShaderFilePath;
    std::string m_ShaderName;
    std::string m_EntryPoint;
    GfxShaderType m_ShaderType;
    std::vector<std::string> m_DefineStrings;
    bool* m_KeysArray;
    uint32_t m_KeysArraySz;
    uint32_t m_ShaderID;
};

struct ShaderPermutationsPrintJob
{
    GfxShaderType m_ShaderType;
    uint32_t m_BaseShaderID;
    std::string m_BaseShaderName;
    std::vector<std::string> m_Defines;
};

struct Globals
{
    DeclareSingletonFunctions(Globals);

    std::string m_AppDir;
    std::string m_ShadersJSONDir;
    std::string m_ShadersTmpDir;
    std::string m_ShadersTmpHLSLAutogenDir;
    std::string m_ShadersTmpCPPShaderInputsAutogenDir;
    std::string m_ShadersTmpCPPShaderPermutationsAutogenDir;
    std::string m_ShadersDir;
    std::string m_DXCDir;
    std::string m_ShadersByteCodesDir;

    std::mutex m_AllShaderCompileJobsLock;
    std::mutex m_AllShaderPermutationsPrintJobsLock;

    std::vector<ShaderCompileJob>           m_AllShaderCompileJobs;
    std::vector<ShaderPermutationsPrintJob> m_AllShaderPermutationsPrintJobs;
};
#define g_Globals Globals::GetInstance()

static bool gs_CompileFailureDetected = false;
