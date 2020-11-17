#pragma once

DEFINE_ENUM_WITH_STRING_CONVERSIONS(GfxShaderType, (VS)(PS)(CS));
static const uint32_t NbShaderTypes = GfxShaderType::CS + 1;

struct HLSLTypesTraits
{
    const char* m_CPPType;
    uint32_t m_SizeInBytes;
};

static const std::unordered_map<std::string_view, HLSLTypesTraits> g_TypesTraitsMap =
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

//struct ConstantBuffer
//{
//    struct CPPTypeVarNamePair
//    {
//        std::string m_HLLSVarType;
//        std::string m_CPPVarType;
//        std::string m_VarName;
//    };
//
//    void AddVariable(const std::string& hlslType, const std::string& varName);
//    void SanityCheck();
//
//    const char* m_Name = "";
//    uint32_t    m_Register = 99;
//
//    std::vector<CPPTypeVarNamePair> m_Variables;
//};

struct ShaderPermutationsPrintJob
{
    GfxShaderType m_ShaderType;
    uint32_t m_BaseShaderID;
    std::string m_BaseShaderName;
    std::vector<std::string> m_Defines;
};

void PopulateJobsArray(const PopulateJobParams& params);

static bool NoFilterForKeys(uint32_t key) { return true; }

template <typename... Ts>
static bool AnyBitSet(uint32_t key, Ts&&... masks)
{
    return (key & ((masks) | ...)) != 0;
}

template <typename... Ts>
static bool AllBitsSet(uint32_t key, Ts&&... masks)
{
    return key == ((masks) | ...);
}

static uint32_t CountBits(uint32_t key)
{
    return std::bitset<64>{ key }.count();
}

template <typename... Ts>
static bool OnlyOneBitSet(uint32_t key, Ts&&... masks)
{
    return CountBits(key & ((masks) | ...)) == 1;
}

template <typename... Ts>
static bool MaxOneBitSet(uint32_t key, Ts&&... masks)
{
    return CountBits(key & ((masks) | ...)) <= 1;
}

template <uint32_t ArraySize, typename FilterFunc>
static void GetValidShaderKeys(bool(&validKeys)[ArraySize], FilterFunc func, uint32_t currentShaderKey = 0)
{
    // Reached end of shader key configs. Return
    if (currentShaderKey >= ArraySize)
        return;

    validKeys[currentShaderKey] = func(currentShaderKey);

    GetValidShaderKeys<ArraySize>(validKeys, func, currentShaderKey + 1);
}

struct Globals
{
    DeclareSingletonFunctions(Globals);

    std::string m_AppDir;
    std::string m_ShadersJSONDir;
    std::string m_ShadersTmpDir;
    std::string m_ShadersTmpHLSLAutogenDir;
    std::string m_ShadersTmpCPPAutogenDir;
    std::string m_ShadersDir;
    std::string m_DXCDir;
    std::string m_ShadersByteCodesDir;

    std::vector<std::function<void()>> m_JobsPopulators;

    std::mutex m_AllShaderCompileJobsLock;
    std::mutex m_AllConstantBuffersLock;
    std::mutex m_AllShaderPermutationsPrintJobsLock;

    std::vector<ShaderCompileJob>           m_AllShaderCompileJobs;
    std::vector<ShaderPermutationsPrintJob> m_AllShaderPermutationsPrintJobs;

    D3D_SHADER_MODEL m_HighestShaderModel;
};
#define g_Globals Globals::GetInstance()

static bool g_CompileFailureDetected = false;
