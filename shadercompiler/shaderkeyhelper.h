#pragma once

void PrintToConsoleAndLogFile(const std::string& str);

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

static bool NoFilterForKeys(uint32_t key) { return true; }

template <uint32_t ArraySize, typename FilterFunc>
static void GetValidShaderKeys(bool (&validKeys)[ArraySize], FilterFunc func, uint32_t currentShaderKey = 0)
{
    // Reached end of shader key configs. Return
    if (currentShaderKey >= ArraySize)
        return;

    validKeys[currentShaderKey] = func(currentShaderKey);

    GetValidShaderKeys<ArraySize>(validKeys, func, currentShaderKey + 1);
}
