#pragma once

namespace UtilsPrivate
{
    template<class T>
    class MemberAutoUnset
    {
        T& m_MemberRef;
        T   m_BackupVal;

    public:
        MemberAutoUnset(T& member, T value)
            : m_MemberRef(member)
        {
            m_BackupVal = m_MemberRef;
            m_MemberRef = value;
        }
        ~MemberAutoUnset()
        {
            m_MemberRef = m_BackupVal;
        }
    };

    template <uint32_t c, int32_t k = 8>
    struct CompileTimeCRCTableBuilder : CompileTimeCRCTableBuilder<((c & 1) ? 0xedb88320 : 0) ^ (c >> 1), k - 1> {};
    template <uint32_t c>
    struct CompileTimeCRCTableBuilder<c, 0> { enum { value = c }; };

#define A(x) B(x) B(x + 128)
#define B(x) C(x) C(x +  64)
#define C(x) D(x) D(x +  32)
#define D(x) E(x) E(x +  16)
#define E(x) F(x) F(x +   8)
#define F(x) G(x) G(x +   4)
#define G(x) H(x) H(x +   2)
#define H(x) I(x) I(x +   1)
#define I(x) static_cast<uint32_t>(CompileTimeCRCTableBuilder<x>::value) ,

    constexpr uint32_t constexpr_crc_table[] = { A(0) };

#undef A
#undef B
#undef C
#undef D
#undef E
#undef F
#undef G
#undef H
#undef I

    constexpr uint32_t ConstExpr_CRC32(const char* p, uint32_t len, uint32_t crc)
    {
        return len ? ConstExpr_CRC32(p + 1, len - 1, (crc >> 8) ^ constexpr_crc_table[(crc & 0xFF) ^ *p]) : crc;
    }

    constexpr uint32_t ConstExpr_String_Length(const char* str)
    {
        return *str ? 1 + ConstExpr_String_Length(str + 1) : 0;
    }

#define DEFINE_ENUM_WITH_STRING_CONVERSIONS_TOSTRING_CASE(r, data, elem) \
    case elem : return BOOST_PP_STRINGIZE(elem);

#define DEFINE_STRING_TOENUM_CONVERSION_PAIR(r, data, elem) \
    { BOOST_PP_STRINGIZE(elem), elem },
}


#define DeclareSingletonFunctions(ClassName)         \
private:                                             \
    ClassName()                                      \
    {                                                \
        ms_Instance = this;                          \
    }                                                \
    inline static ClassName* ms_Instance = nullptr;  \
public:                                              \
    static ClassName& GetInstance()                  \
    {                                                \
        static ClassName s_Instance;                 \
        return s_Instance;                           \
    }                                                \
    ~ClassName() = default;                          \
    ClassName(const ClassName&) = delete;            \
    ClassName(ClassName&&) = delete;                 \
    ClassName& operator=(const ClassName&) = delete; \
    ClassName& operator=(ClassName&&) = delete;


#define DeclareObjectModelFunctions(ClassName)                                                    \
private:                                                                                          \
    friend class Scene;                                                                           \
    ObjectID m_ObjectID = GenerateObjectID();                                                     \
                                                                                                  \
public:                                                                                           \
    static constexpr ClassID GetClassID() { return GetCompileTimeCRC32(bbeTOSTRING(ClassName)); } \
                                                                                                  \
    ObjectID GetObjectID() const { return m_ObjectID; }                                           \
                                                                                                  \
    template <typename Archive>                                                                   \
    void Serialize(Archive&);



#define BBE_OPTIMIZE_OFF __pragma(optimize("",off))
#define BBE_OPTIMIZE_ON  __pragma(optimize("", on))

#define bbeTOSTRING_PRIVATE(x)              #x
#define bbeTOSTRING(x)                      bbeTOSTRING_PRIVATE(x)
#define bbeFILEandLINE                      __FILE__ "(" bbeTOSTRING(__LINE__) ")"
#define bbeDO_JOIN( Arg1, Arg2 )            Arg1##Arg2
#define bbeJOIN( Arg1, Arg2 )               bbeDO_JOIN( Arg1, Arg2 )
#define bbeUniqueVariable(basename)         bbeJOIN(basename, __COUNTER__)

template <typename EnterLambda, typename ExitLamda>
class AutoScopeCaller
{
public:
    AutoScopeCaller(EnterLambda&& enterLambda, ExitLamda&& exitLambda)
        : m_ExitLambda(std::forward<ExitLamda>(exitLambda))
    {
        enterLambda();
    }

    ~AutoScopeCaller()
    {
        m_ExitLambda();
    }

private:
    ExitLamda&& m_ExitLambda;
};

#define bbeOnExitScope(lambda) const AutoScopeCaller bbeUniqueVariable(AutoOnExitVar){ [](){}, lambda };

#define BBE_SCOPED_UNSET(type, var, val) UtilsPrivate::MemberAutoUnset<type> bbeUniqueVariable(autoUnset){var, val};

#define bbeMemZeroArray(dst)  memset(dst, 0, sizeof(dst))
#define bbeMemZeroStruct(dst) memset(&dst, 0, sizeof(dst))

#define bbePrefetchData(address, offset_in_bytes) _mm_prefetch((const char*)(address) + offset_in_bytes, _MM_HINT_T0);

#define bbeSimpleSwitchCaseString(x) case(x): return bbeTOSTRING(x);

// Size for convenience...
#define BBE_KB(Nb)                   ((Nb)*1024ULL)           // Define for kilobytes prefix (2 ^ 10)
#define BBE_MB(Nb)                   BBE_KB((Nb)*1024ULL)     // Define for megabytes prefix (2 ^ 20)
#define BBE_GB(Nb)                   BBE_MB((Nb)*1024ULL)     // Define for gigabytes prefix (2 ^ 30)
#define BBE_TB(Nb)                   BBE_GB((Nb)*1024ULL)     // Define for terabytes prefix (2 ^ 40)
#define BBE_PB(Nb)                   BBE_TB((Nb)*1024ULL)     // Define for petabytes prefix (2 ^ 50)

#define BBE_TO_KB(Nb)                ((Nb) * (1.0f / 1024))          // Define to convert bytes to kilobyte
#define BBE_TO_MB(Nb)                (BBE_TO_KB(Nb) * (1.0f / 1024)) // Define to convert bytes to megabytes

const char* StringFormat(const char* format, ...);

// In case you need to format huge strings > 1kb (but <1mb) in length
template<typename... Args>
static const char* StringFormatBig(const char* format, Args&&... args)
{
    thread_local std::string tl_Result;
    const size_t size = snprintf(nullptr, 0, format, args...) + 1; // Extra space for '\0'
    tl_Result.resize(size);
    snprintf(tl_Result.data(), size, format, args ...);
    return tl_Result.c_str();
}

void BreakIntoDebugger();

// Returning the time in this format (yyyy_mm_dd_hh_mm_ss) allows for easy sorting of filenames
const char* GetTimeStamp();

constexpr uint32_t GetCompileTimeCRC32(const char* str)
{
    return ~UtilsPrivate::ConstExpr_CRC32(str, UtilsPrivate::ConstExpr_String_Length(str), ~uint32_t(0));
}

const char* GetLastErrorAsString();
const char* GetApplicationDirectory();
const char* GetTempDirectory();
const char* GetAssetsDirectory();
const std::string GetDirectoryFromPath(const char* fullPath);
void SplitPath(const std::string& fullPath, std::string& dir, std::string& fileName);
void GetFilesInDirectory(std::vector<std::string>& out, const std::string& directory);
const std::string GetFileNameFromPath(const std::string& fullPath);
const std::string GetFileExtensionFromPath(const std::string& fullPath);
void ReadDataFromFile(const char* filename, std::vector<std::byte>& data);
std::size_t GetFileContentsHash(const char* dir);

struct WindowsHandleWrapper
{
    WindowsHandleWrapper(::HANDLE hdl = nullptr)
        : m_Handle(hdl)
    {}

    ~WindowsHandleWrapper()
    {
        ::FindClose(m_Handle);
        ::CloseHandle(m_Handle);
    }

    operator ::HANDLE() const { return m_Handle; }

    ::HANDLE m_Handle = nullptr;
};

struct CFileWrapper
{
    CFileWrapper(const char*fileName, bool isReadMode = true);
    ~CFileWrapper();

    CFileWrapper(const CFileWrapper&) = delete;
    CFileWrapper& operator=(const CFileWrapper&) = delete;

    operator bool() const { return m_File; }
    operator FILE* () const { return m_File; }

    FILE* m_File = nullptr;
};

class MultithreadDetector
{
public:
    void Enter(std::thread::id newID)
    {
        if (m_CurrentID != std::thread::id{} && newID != m_CurrentID)
            assert(false); // Multi-thread detected!
        m_CurrentID = newID;
    }

    void Exit() { m_CurrentID = std::thread::id{}; }

private:
    std::atomic<std::thread::id> m_CurrentID = {};
};

#define bbeMultiThreadDetector() \
    static MultithreadDetector __s_bbe_MTDetector__; \
    const AutoScopeCaller bbeUniqueVariable(mtDetectorScoped){ [&](){ __s_bbe_MTDetector__.Enter(std::this_thread::get_id()); }, [&](){ __s_bbe_MTDetector__.Exit(); } }; 

// generic hasher func that uses Boost lib to hash the entire type. 
template <typename T>
static std::size_t GenericTypeHash(const T& s)
{
    const std::byte* rawMem = reinterpret_cast<const std::byte*>(&s);
    return boost::hash_range(rawMem, rawMem + sizeof(T));
}

template <typename Functor, bool ForwardScan = true>
static void RunOnAllBits(uint32_t mask, Functor&& func)
{
    std::bitset<sizeof(uint32_t) * CHAR_BIT> bitSet{ mask };
    unsigned long idx;
    auto ForwardScanFunc = [&]() { return _BitScanForward(&idx, bitSet.to_ulong()); };
    auto ReverseScanFunc = [&]() { return _BitScanReverse(&idx, bitSet.to_ulong()); };

    while (ForwardScan ? ForwardScanFunc() : ReverseScanFunc())
    {
        bitSet.set(idx, false);
        func(idx);
    }
}

#define DEFINE_ENUM_WITH_STRING_CONVERSIONS(name, enumerators)                             \
    enum name {                                                                            \
        BOOST_PP_SEQ_ENUM(enumerators),                                                    \
        name##_Count                                                                       \
    };                                                                                     \
                                                                                           \
    static constexpr char* EnumToString(name v)                                            \
    {                                                                                      \
        switch (v)                                                                         \
        {                                                                                  \
            BOOST_PP_SEQ_FOR_EACH(                                                         \
                DEFINE_ENUM_WITH_STRING_CONVERSIONS_TOSTRING_CASE,                         \
                name,                                                                      \
                enumerators                                                                \
            )                                                                              \
            default: return "[Unknown " BOOST_PP_STRINGIZE(name) "]";                      \
        }                                                                                  \
    }                                                                                      \
                                                                                           \
    static name StringTo##name(const std::string& str)                                     \
    {                                                                                      \
        static const std::unordered_map<std::string_view, name> s_StrToEnumMap =           \
        {                                                                                  \
            BOOST_PP_SEQ_FOR_EACH(DEFINE_STRING_TOENUM_CONVERSION_PAIR, name, enumerators) \
        };                                                                                 \
        return s_StrToEnumMap.at(str);                                                     \
    }

static ObjectID GenerateObjectID() { return boost::uuids::random_generator{}(); }
static const ObjectID ID_InvalidObject = boost::uuids::nil_generator{}();
static std::string ToString(ObjectID id) { return boost::uuids::to_string(id); }

namespace StringUtils
{
    const wchar_t* Utf8ToWide(std::string_view strView);
    const char* WideToUtf8(std::wstring_view strView);

    // In case you need to convert huge strings > 1kb in length
    const wchar_t* Utf8ToWideBig(std::string_view strView);
    const char* WideToUtf8Big(std::wstring_view strView);

    using ConvertFuncType = int(int); // std::tolower & std::toupper has same signature

    template <typename T>
    static void TransformStrInplace(T& str, ConvertFuncType converterFunc)
    {
        using CharType = T::traits_type::char_type;
        static_assert(std::is_same_v<CharType, char> || std::is_same_v<CharType, wchar_t>);

        using IntType = std::conditional_t<std::is_same_v<CharType, char>, uint8_t, wchar_t>;
        std::transform(str.begin(), str.end(), str.begin(), [converterFunc](CharType c) { return static_cast<CharType>(converterFunc(static_cast<IntType>(c))); });
    }

    template <typename StringType>
    static void ToLower(StringType& str) { TransformStrInplace(str, std::tolower); }

    template <typename StringType>
    static void ToUpper(StringType& str) { TransformStrInplace(str, std::toupper); }
}

float RandomFloat(float range = 1.0f);
uint32_t RandomUInt(uint32_t range = std::numeric_limits<uint32_t>::max());
int32_t RandomInt(uint32_t range = std::numeric_limits<int32_t>::max());
