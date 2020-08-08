#pragma once

#include <inttypes.h>
#include <string>
#include <utility>
#include <time.h>
#include <mutex>
#include <vector>
#include <atomic>

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
    ObjectID m_ObjectID = ID_InvalidObject;                                                       \
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

template<typename... Args>
static const std::string StringFormat(const char* format, Args&&... args)
{
    const size_t size = snprintf(nullptr, 0, format, args...) + 1; // Extra space for '\0'
    char* buf = (char*)_malloca(size);
    snprintf(buf, size, format, args ...);
    return std::string{ buf, buf + size - 1 }; // We don't want the '\0' inside
}

#define bbePrefetchData(address, offset_in_bytes) _mm_prefetch((const char*)(address) + offset_in_bytes, _MM_HINT_T0);

#define bbeSimpleSwitchCaseString(x) case(x): return bbeTOSTRING(x);

// Size for convenience...
#define BBE_KB(Nb)                   ((Nb)*1024ULL)           ///< Define for kilobytes prefix (2 ^ 10)
#define BBE_MB(Nb)                   BBE_KB((Nb)*1024ULL)     ///< Define for megabytes prefix (2 ^ 20)
#define BBE_GB(Nb)                   BBE_MB((Nb)*1024ULL)     ///< Define for gigabytes prefix (2 ^ 30)
#define BBE_TB(Nb)                   BBE_GB((Nb)*1024ULL)     ///< Define for terabytes prefix (2 ^ 40)
#define BBE_PB(Nb)                   BBE_TB((Nb)*1024ULL)     ///< Define for petabytes prefix (2 ^ 50)

void BreakIntoDebugger();

// Returning the time in this format (yyyy_mm_dd_hh_mm_ss) allows for easy sorting of filenames
const std::string GetTimeStamp();

constexpr uint32_t GetCompileTimeCRC32(const char* str)
{
    return ~UtilsPrivate::ConstExpr_CRC32(str, UtilsPrivate::ConstExpr_String_Length(str), ~uint32_t(0));
}

// Convert a wide Unicode string to an UTF8 string
const std::string MakeStrFromWStr(const std::wstring& wstr);
static const std::wstring MakeWStrFromStr(const std::string& str) { return std::wstring{ str.begin(), str.end() }; }

const std::string GetLastErrorAsString();

const std::string& GetApplicationDirectory();
const std::string& GetTempDirectory();
const std::string& GetAssetsDirectory();
const std::string GetDirectoryFromPath(const std::string& fullPath);
void SplitPath(const std::string& fullPath, std::string& dir, std::string& fileName);
void GetFilesInDirectory(std::vector<std::string>& out, const std::string& directory);
const std::string GetFileNameFromPath(const std::string& fullPath);
const std::string GetFileExtensionFromPath(const std::string& fullPath);

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
    CFileWrapper(const std::string& fileName, bool isReadMode);
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
    void Enter(std::thread::id newID);
    void Exit();

private:
    std::atomic<std::thread::id> m_CurrentID = {};
};

#define bbeMultiThreadDetector() \
    static MultithreadDetector __s_bbe_MTDetector__; \
    const AutoScopeCaller bbeUniqueVariable(mtDetectorScoped){ [&](){ __s_bbe_MTDetector__.Enter(std::this_thread::get_id()); }, [&](){ __s_bbe_MTDetector__.Exit(); } }; 

// generic hasher func that uses Boost lib to hash the entire type
// NOTE: Make sure every variable in type is of a POD type!
template <typename T>
static std::size_t GenericTypeHash(const T& s)
{
    std::size_t hash = 0;
    const std::byte* rawMem = reinterpret_cast<const std::byte*>(&s);
    boost::hash_range(hash, rawMem, rawMem + sizeof(T));
    return hash;
}

void ReadDataFromFile(const char* filename, std::vector<std::byte>& data);

template <typename Functor>
static void RunOnAllBits(uint32_t mask, Functor&& func)
{
    unsigned long idx;
    while (_BitScanForward(&idx, mask))
    {
        mask ^= (1 << idx);
        func(idx);
    }
}

#define DEFINE_ENUM_WITH_STRING_CONVERSIONS(name, enumerators)                \
    enum name {                                                               \
        BOOST_PP_SEQ_ENUM(enumerators)                                        \
    };                                                                        \
                                                                              \
    static const char* EnumToString(name v)                                   \
    {                                                                         \
        switch (v)                                                            \
        {                                                                     \
            BOOST_PP_SEQ_FOR_EACH(                                            \
                DEFINE_ENUM_WITH_STRING_CONVERSIONS_TOSTRING_CASE,            \
                name,                                                         \
                enumerators                                                   \
            )                                                                 \
            default: return "[Unknown " BOOST_PP_STRINGIZE(name) "]";         \
        }                                                                     \
    }

static ObjectID GenerateObjectID() { return boost::uuids::random_generator{}(); }
static ObjectID ID_InvalidObject = boost::uuids::nil_generator{}();
static std::string ToString(ObjectID id) { return boost::uuids::to_string(id); }
