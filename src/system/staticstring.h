#pragma once

template <uint32_t BufferSize, class _Ty>
class StaticStringAllocator 
{
public:
    using _From_primary = StaticStringAllocator;

    using value_type = _Ty;

    _CXX17_DEPRECATE_OLD_ALLOCATOR_MEMBERS typedef _Ty* pointer;
    _CXX17_DEPRECATE_OLD_ALLOCATOR_MEMBERS typedef const _Ty* const_pointer;

    _CXX17_DEPRECATE_OLD_ALLOCATOR_MEMBERS typedef _Ty& reference;
    _CXX17_DEPRECATE_OLD_ALLOCATOR_MEMBERS typedef const _Ty& const_reference;

    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    using propagate_on_container_move_assignment = std::true_type;
    using is_always_equal = std::true_type;

    template <class _Other>
    struct _CXX17_DEPRECATE_OLD_ALLOCATOR_MEMBERS rebind {
        using other = StaticStringAllocator<BufferSize, _Other>;
    };

    _NODISCARD _CXX17_DEPRECATE_OLD_ALLOCATOR_MEMBERS _Ty* address(_Ty& _Val) const noexcept {
        return _STD addressof(_Val);
    }

    _NODISCARD _CXX17_DEPRECATE_OLD_ALLOCATOR_MEMBERS const _Ty* address(const _Ty& _Val) const noexcept {
        return _STD addressof(_Val);
    }

    constexpr StaticStringAllocator() noexcept {}

    constexpr StaticStringAllocator(const StaticStringAllocator&) noexcept = default;
    template <class _Other>
    constexpr StaticStringAllocator(const StaticStringAllocator<BufferSize, _Other>&) noexcept {}

    void deallocate(_Ty* const _Ptr, const size_t _Count) {
        m_DataPtr -= _Count;
    }

    _NODISCARD _DECLSPEC_ALLOCATOR _Ty* allocate(_CRT_GUARDOVERFLOW const size_t _Count) {
        _Ty* toReturn = m_DataPtr;
        m_DataPtr += _Count;
        assert(m_DataPtr <= &m_Data[BufferSize - 1]);
        return toReturn;
    }

    _NODISCARD _CXX17_DEPRECATE_OLD_ALLOCATOR_MEMBERS _DECLSPEC_ALLOCATOR _Ty* allocate(
        _CRT_GUARDOVERFLOW const size_t _Count, const void*) {
        return allocate(_Count);
    }

    template <class _Objty, class... _Types>
    _CXX17_DEPRECATE_OLD_ALLOCATOR_MEMBERS void construct(_Objty* const _Ptr, _Types &&... _Args) {
        ::new (const_cast<void*>(static_cast<const volatile void*>(_Ptr))) _Objty(_STD forward<_Types>(_Args)...);
    }

    template <class _Uty>
    _CXX17_DEPRECATE_OLD_ALLOCATOR_MEMBERS void destroy(_Uty* const _Ptr) {}

    _NODISCARD _CXX17_DEPRECATE_OLD_ALLOCATOR_MEMBERS size_t max_size() const noexcept { return static_cast<size_t>(-1) / sizeof(_Ty); }

private:
    _Ty  m_Data[BufferSize];
    _Ty* m_DataPtr = m_Data;
};

template <uint32_t BufferSize>
using StaticString = std::basic_string<char, std::char_traits<char>, StaticStringAllocator<BufferSize, char>>;

template <uint32_t BufferSize>
using StaticWString = std::basic_string<wchar_t, std::char_traits<wchar_t>, StaticStringAllocator<BufferSize, wchar_t>>;
