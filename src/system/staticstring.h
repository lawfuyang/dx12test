#pragma once

template <typename CharType>
class StaticStringAllocator
{
public:
    using value_type = CharType;

    StaticStringAllocator() noexcept {}

    template <class U> 
    StaticStringAllocator(const StaticStringAllocator<U>&) noexcept {}

    explicit StaticStringAllocator(value_type* bufferSource) noexcept
        : m_BufferSource(bufferSource)
    {}

    value_type* allocate(std::size_t n) noexcept { return m_BufferSource; }
    constexpr void deallocate(value_type* p, std::size_t n) noexcept  {}

    value_type* GetBuffer() const noexcept { return m_BufferSource; }

private:
    value_type* m_BufferSource;
};

template <typename CharType>
bool operator==(const StaticStringAllocator<CharType>& lhs, const StaticStringAllocator<CharType>& rhs) noexcept
{
    return lhs.GetBuffer() == rhs.GetBuffer();
}

template <typename CharType>
bool operator!=(const StaticStringAllocator<CharType>& lhs, const StaticStringAllocator<CharType>& rhs) noexcept
{
    return !(lhs == rhs);
}

template <uint32_t BufferSize, typename CharType>
class StaticStringBase : protected StaticStringAllocator<CharType>,
                         public std::basic_string<CharType, std::char_traits<CharType>, StaticStringAllocator<CharType>>
{
public:
    using AllocatorType = StaticStringAllocator<CharType>;
    using Base          = std::basic_string<CharType, std::char_traits<CharType>, AllocatorType>;

    operator Base() { return (Base)*this; }

    template <typename ...Params>
    StaticStringBase(Params&&... params)
        : AllocatorType(m_Buffer)
        , Base(std::forward<Params>(params)..., (AllocatorType)*this)
    {
        Base::reserve(BufferSize);
    }

private:
    CharType m_Buffer[BufferSize];
};

template <uint32_t BufferSize, typename CharType>
bool operator==(const StaticStringBase<BufferSize, CharType>& lhs, const StaticStringBase<BufferSize, CharType>& rhs) noexcept
{
    using Base = StaticStringBase<BufferSize, CharType>::Base;
    return static_cast<const Base&>(lhs) == static_cast<const Base&>(rhs);
}

template <uint32_t BufferSize, typename CharType>
bool operator!=(const StaticStringBase<BufferSize, CharType>& lhs, const StaticStringBase<BufferSize, CharType>& rhs) noexcept
{
    return !(lhs == rhs);
}

template <uint32_t BufferSize>
using StaticString = StaticStringBase<BufferSize, char>;

template <uint32_t BufferSize>
using StaticWString = StaticStringBase<BufferSize, wchar_t>;
