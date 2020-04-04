#pragma once

template <typename CharType>
class StackAllocator
{
public:
    using value_type = CharType;

    StackAllocator() noexcept {}

    template <class U> 
    StackAllocator(const StackAllocator<U>&) noexcept {}

    explicit StackAllocator(std::byte* bufferSource)
        : m_BufferSource((value_type*)bufferSource)
    {}

    value_type* allocate(std::size_t n)
    {
        assert(m_BufferSource);
        return m_BufferSource;
    }

    constexpr void deallocate(value_type* p, std::size_t n) {}

    value_type* m_BufferSource = nullptr;
};

template <uint32_t BufferSize, typename CharT>
class StaticStringBase
{
public:
    using UnderlyingStringType = std::basic_string<CharT, std::char_traits<CharT>, StackAllocator<CharT>>;

    UnderlyingStringType* operator->() { return &m_UnderlyingStringType; }
    const UnderlyingStringType* operator->() const { return &m_UnderlyingStringType; }

    operator UnderlyingStringType() const { return m_UnderlyingStringType; }

    StaticStringBase()
        : m_Allocator(m_Buffer)
        , m_UnderlyingStringType(m_Allocator)
    {
        m_UnderlyingStringType.reserve(BufferSize);
    }

    StaticStringBase(const CharT* other)
        : StaticStringBase()
    {
        if (other)
        {
            m_UnderlyingStringType = other;
        }
    }

    StaticStringBase(const StaticStringBase& other)
        : StaticStringBase()
    {
        m_UnderlyingStringType = other.m_UnderlyingStringType;
    }

    StaticStringBase& operator=(const StaticStringBase& other)
    {
        m_UnderlyingStringType = other.m_UnderlyingStringType;
        return *this;
    }

    StaticStringBase& operator+=(const StaticStringBase& other)
    {
        m_UnderlyingStringType += other.m_UnderlyingStringType;
        return *this;
    }

    bool operator<(const StaticStringBase& other) const { return m_UnderlyingStringType < other.m_UnderlyingStringType; }

    bool operator==(const StaticStringBase& other) const { return m_UnderlyingStringType == other.m_UnderlyingStringType; }
    bool operator!=(const StaticStringBase& other) const { return m_UnderlyingStringType != other.m_UnderlyingStringType; }

    CharT& operator[](std::size_t idx) { return m_UnderlyingStringType[idx]; }
    const CharT operator[](std::size_t idx) const { return m_UnderlyingStringType[idx]; }

private:
    StackAllocator<CharT> m_Allocator;
    UnderlyingStringType  m_UnderlyingStringType;
    std::byte             m_Buffer[BufferSize];
};

template <uint32_t BufferSize>
using StaticString = StaticStringBase<BufferSize, char>;

template <uint32_t BufferSize>
using StaticWString = StaticStringBase<BufferSize, wchar_t>;
