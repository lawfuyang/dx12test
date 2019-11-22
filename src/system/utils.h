#pragma once

namespace UtilsPrivate
{
    template<typename FunctionType>
    class ScopeGuard
    {
    public:
        explicit ScopeGuard(const FunctionType& fn)
            : m_Function(fn) {}

        explicit ScopeGuard(FunctionType&& fn)
            : m_Function(std::move(fn)) {}

        void Execute() { m_Function(); }

        ~ScopeGuard() {}

    private:
        FunctionType m_Function;
    };

    template<typename FunctionType>
    class ExitScopeGuard : public ScopeGuard<FunctionType>
    {
        typedef ScopeGuard<FunctionType> BaseType;

    public:
        explicit ExitScopeGuard(const FunctionType& fn)
            : BaseType(fn) {}

        explicit ExitScopeGuard(FunctionType&& fn)
            : BaseType(fn) {}

        ~ExitScopeGuard() { BaseType::Execute(); }
    };

    enum class ScopeGuardOnExit {};

    template<typename ExitFunction>
    ExitScopeGuard<typename std::decay<ExitFunction>::type> operator+(ScopeGuardOnExit, ExitFunction&& fn)
    {
        return ExitScopeGuard<typename std::decay<ExitFunction>::type>(std::forward<ExitFunction>(fn));
    }

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
}

#define DeclareSingletonFunctions(ClassName)             \
    public:                                              \
        static ClassName& GetInstance()                  \
        {                                                \
            static ClassName s_Instance;                 \
            ms_Instance = &s_Instance;                   \
            return s_Instance;                           \
        }                                                \
    private:                                             \
        ClassName() = default;                           \
        ~ClassName() = default;                          \
        ClassName(const ClassName&) = delete;            \
        ClassName(ClassName&&) = delete;                 \
        ClassName& operator=(const ClassName&) = delete; \
        ClassName& operator=(ClassName&&) = delete;      \
        inline static ClassName* ms_Instance = nullptr;  \
    public:

#define BBE_OPTIMIZE_OFF __pragma(optimize("",off))
#define BBE_OPTIMIZE_ON  __pragma(optimize("", on))

#define bbeTOSTRING_PRIVATE(x)              #x
#define bbeTOSTRING(x)                      bbeTOSTRING_PRIVATE(x)
#define bbeFILEandLINE                      __FILE__ "(" bbeTOSTRING(__LINE__) ")"
#define bbeDO_JOIN( Arg1, Arg2 )            Arg1##Arg2
#define bbeJOIN( Arg1, Arg2 )               bbeDO_JOIN( Arg1, Arg2 )
#define bbeUniqueVariable(basename)         bbeJOIN(basename, __COUNTER__)

#define bbeOnExitScope                         \
    auto bbeUniqueVariable(AutoOnExitVar)      \
    = UtilsPrivate::ScopeGuardOnExit() + [&]()

#define BBE_SCOPED_UNSET(type, var, val) UtilsPrivate::MemberAutoUnset<type> bbeUniqueVariable(autoUnset){var, val};

#define BBE_DEFINE_ENUM_OPERATOR(enumType)                                                                                                                         \
    inline enumType operator|( enumType a, enumType b )                                                                                                            \
    {                                                                                                                                                              \
        return static_cast< enumType >( static_cast< std::underlying_type< enumType >::type >( a ) | static_cast< std::underlying_type< enumType >::type >( b ) ); \
    }                                                                                                                                                              \
    inline enumType operator&( enumType a, enumType b )                                                                                                            \
    {                                                                                                                                                              \
        return static_cast< enumType >( static_cast< std::underlying_type< enumType >::type >( a ) & static_cast< std::underlying_type< enumType >::type >( b ) ); \
    }                                                                                                                                                              \
    inline enumType operator^( enumType a, enumType b )                                                                                                            \
    {                                                                                                                                                              \
        return static_cast< enumType >( static_cast< std::underlying_type< enumType >::type >( a ) ^ static_cast< std::underlying_type< enumType >::type >( b ) ); \
    }                                                                                                                                                              \
    inline enumType operator~( enumType a )                                                                                                                        \
    {                                                                                                                                                              \
        return static_cast< enumType >( ~static_cast< std::underlying_type< enumType >::type >( a ) );                                                             \
    }                                                                                                                                                              \
    inline enumType& operator&=( enumType& a, enumType b )                                                                                                         \
    {                                                                                                                                                              \
        a = a & b;                                                                                                                                                 \
        return a;                                                                                                                                                  \
    }                                                                                                                                                              \
    inline enumType& operator|=( enumType& a, enumType b )                                                                                                         \
    {                                                                                                                                                              \
        a = a | b;                                                                                                                                                 \
        return a;                                                                                                                                                  \
    }                                                                                                                                                              \
    inline enumType& operator^=( enumType& a, enumType b )                                                                                                         \
    {                                                                                                                                                              \
        a = a ^ b;                                                                                                                                                 \
        return a;                                                                                                                                                  \
    }                                                                                                                                                              \
    inline enumType operator<<( enumType a, uint32_t shift )                                                                                                       \
    {                                                                                                                                                              \
        return static_cast< enumType >( static_cast< std::underlying_type< enumType >::type >( a ) << shift );                                                     \
    }                                                                                                                                                              \
    inline enumType operator<<=( enumType& a, uint32_t shift )                                                                                                     \
    {                                                                                                                                                              \
        a = a << shift;                                                                                                                                            \
        return a;                                                                                                                                                  \
    }                                                                                                                                                              \
    inline bool IsSet( enumType a )                                                                                                                                \
    {                                                                                                                                                              \
        return (static_cast<uint32_t>(a) != 0);                                                                                                                    \
    }

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

#define BBE_UNUSED_PARAM(p) ((void)&p)

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
const std::string utf8_encode(const std::wstring& wstr);

// Convert an UTF8 string to a wide Unicode String
const std::wstring utf8_decode(const std::string& str);

const std::string GetLastErrorAsString();
