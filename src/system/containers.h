#pragma once

// Windows Concurrent Containers
#include <concurrent_unordered_map.h>
#include <concurrent_unordered_set.h>
#include <concurrent_vector.h>

// Boost Containers
#include <extern/boost/circular_buffer.hpp>
#include <extern/boost/container/flat_map.hpp>
#include <extern/boost/container/flat_set.hpp>
#include <extern/boost/container/small_vector.hpp>
#include <extern/boost/container/static_vector.hpp>
#include <extern/boost/pool/object_pool.hpp>

template <typename T>
using ConcurrentVector = concurrency::concurrent_vector<T>;

template <typename KeyType, typename ValueType>
using ConcurrentUnorderedMap = concurrency::concurrent_unordered_map<KeyType, ValueType>;

template <typename T>
using ConcurrentUnorderedSet = concurrency::concurrent_unordered_set<T>;

template <typename T, uint32_t N>
using InplaceArray = boost::container::small_vector<T, N>;

template <typename T, uint32_t N>
using FixedSizeArray = boost::container::static_vector<T, N>;

template <typename KeyType, typename ValueType>
using FlatMap = boost::container::flat_map<KeyType, ValueType>;

template <typename KeyType>
using FlatSet = boost::container::flat_set<KeyType>;

template <typename KeyType, typename ValueType, std::size_t N>
using InplaceFlatMap = boost::container::flat_map<KeyType, ValueType, std::less<KeyType>, InplaceArray<std::pair<KeyType, ValueType>, N>>;

template <typename ValueType, uint32_t N>
using InplaceFlatSet = boost::container::flat_set< ValueType, std::less<ValueType>, InplaceArray<ValueType, N>>;

template <typename KeyType, typename ValueType, std::size_t N>
using FixedSizeFlatMap = boost::container::flat_map<KeyType, ValueType, std::less<KeyType>, boost::container::static_vector<std::pair<KeyType, ValueType>, N>>;

template <typename ValueType, uint32_t N>
using FixedSizeFlatSet = boost::container::flat_set< ValueType, std::less<ValueType>, boost::container::static_vector<ValueType, N>>;

template<std::size_t N>
using StaticString = boost::static_strings::static_string<N>;

template<std::size_t N>
using StaticWString = boost::static_strings::static_wstring<N>;

template <typename T>
using ObjectPool = boost::object_pool<T>;

template <typename T>
using CircularBuffer = boost::circular_buffer<T>;
