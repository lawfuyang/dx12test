#pragma once

#include <regex>

// Quick & Easy JSON parsing
#include <extern/json/json.hpp>
using json = nlohmann::json;

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
    bool m_IsStructured;
    ResourceType m_Type;
};
static const std::unordered_map<std::string_view, ResourceTraits> gs_ResourceTraitsMap =
{
    { "Texture2D"         , { false, SRV } },
    { "StructuredBuffer"  , { true, SRV } },
    { "RWStructuredBuffer", { true, UAV } },
};

struct ShaderConstant
{
    std::string m_Type;
    std::string m_Name;
    std::string m_ConstValue;
};

struct GlobalStructure
{
    std::string m_Name;
    std::vector<ShaderConstant> m_Constants;
};
namespace std
{
    template<> struct equal_to<GlobalStructure> { bool operator()(const GlobalStructure& lhs, const GlobalStructure& rhs) const { return lhs.m_Name == rhs.m_Name; } };
    template<> struct hash<GlobalStructure> { std::size_t operator()(const GlobalStructure& s) const { return std::hash<std::string>{}(s.m_Name); } }; 
}

struct ShaderInputs
{
    struct ConstantBuffer
    {
        uint32_t m_Register = 0xDEADBEEF;
        std::vector<ShaderConstant> m_Constants;
    };
    struct Resource
    {
        std::string m_Type;
        std::string m_UAVStructureType;
        uint32_t m_Register = 0xDEADBEEF;
        std::string m_Name;
    };

    std::string m_Name;
    std::vector<Resource> m_Resources[ResourceType_Count];
    ConstantBuffer m_ConstantBuffer;
    std::unordered_set<GlobalStructure> m_GlobalStructureDependencies;
    std::vector<ShaderConstant> m_Consts;
};

struct Shader
{
    uint32_t m_BaseShaderID = 0;
    std::string m_Name;
    std::string m_FileName;
    std::string m_EntryPoints[GfxShaderType_Count];

    struct Permutation
    {
        uint32_t m_ShaderKey;
        std::string m_Name;
        std::string m_ShaderObjCodeFileDir;
        std::vector<std::string> m_Defines;
        std::size_t m_Hash = 0;
    };
    std::vector<Permutation> m_Permutations[GfxShaderType_Count];

    bool operator<(const Shader& rhs) const { return m_Name < rhs.m_Name; }
};

struct GlobalDirs
{
    DeclareSingletonFunctions(GlobalDirs);

    std::string m_TmpDir;
    std::string m_ShadersSrcDir;
    std::string m_ShadersJSONSrcDir;
    std::string m_ShadersTmpDir;
    std::string m_ShadersTmpAutoGenDir;
    std::string m_ShadersTmpCPPAutogenDir;
    std::string m_ShadersTmpHLSLAutogenDir;
    std::string m_ShadersTmpPDBAutogenDir;
    std::string m_ShadersTmpCPPShaderInputsAutogenDir;
    std::string m_ShadersTmpCPPShaderPermutationsAutogenDir;
};
#define g_GlobalDirs GlobalDirs::GetInstance()

void PrintToConsoleAndLogFile(std::string_view str);
void ProcessShaderJSONFile(ConcurrentVector<Shader>& allShaders, const std::string& file);
void AddValidPermutations(Shader& newShader, GfxShaderType shaderType, json shaderPermsForTypeJSON);
void PrintAutogenFilesForShaderInput(const ShaderInputs& inputs);
void PrintAutogenFilesForGlobalStructure(const GlobalStructure& s);
void PrintAutogenFileForShaderPermutationStructs(const Shader& shader);
void CompilePermutation(const Shader& parentShader, Shader::Permutation& permutation, GfxShaderType shaderType);
void PrintAutogenByteCodeHeadersFile(const ConcurrentVector<Shader>& allShaders);

static bool gs_FullRebuild = false;
static bool gs_CompileFailureDetected = false;
static std::string gs_ShaderModelToUse = "6_4";
