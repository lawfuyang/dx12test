#pragma once

using bbeVector2    = DirectX::SimpleMath::Vector2;
using bbeVector2I   = DirectX::XMINT2;
using bbeVector2U   = DirectX::XMUINT2;
using bbeVector3    = DirectX::SimpleMath::Vector3;
using bbeVector3I   = DirectX::XMINT3;
using bbeVector3U   = DirectX::XMUINT3;
using bbeVector4    = DirectX::SimpleMath::Vector4;
using bbeVector4I   = DirectX::XMINT4;
using bbeVector4U   = DirectX::XMUINT4;
using bbeMatrix     = DirectX::SimpleMath::Matrix;
using bbePlane      = DirectX::SimpleMath::Plane;
using bbeQuaternion = DirectX::SimpleMath::Quaternion;
using bbeColor      = DirectX::SimpleMath::Color;

#define CREATE_SIMD_FUNCTIONS( TYPE ) \
    static TYPE Sqrt( TYPE s ) { return TYPE(DirectX::XMVectorSqrt(s)); } \
    static TYPE Reciprocal( TYPE s ) { return TYPE(DirectX::XMVectorReciprocal(s)); } \
    static TYPE ReciprocalSqrt( TYPE s ) { return TYPE(DirectX::XMVectorReciprocalSqrt(s)); } \
    static TYPE Floor( TYPE s ) { return TYPE(DirectX::XMVectorFloor(s)); } \
    static TYPE Ceiling( TYPE s ) { return TYPE(DirectX::XMVectorCeiling(s)); } \
    static TYPE Round( TYPE s ) { return TYPE(DirectX::XMVectorRound(s)); } \
    static TYPE Exp( TYPE s ) { return TYPE(DirectX::XMVectorExp(s)); } \
    static TYPE Pow( TYPE b, TYPE e ) { return TYPE(DirectX::XMVectorPow(b, e)); } \
    static TYPE Log( TYPE s ) { return TYPE(DirectX::XMVectorLog(s)); } \
    static TYPE Sin( TYPE s ) { return TYPE(DirectX::XMVectorSin(s)); } \
    static TYPE Cos( TYPE s ) { return TYPE(DirectX::XMVectorCos(s)); } \
    static TYPE Tan( TYPE s ) { return TYPE(DirectX::XMVectorTan(s)); } \
    static TYPE ASin( TYPE s ) { return TYPE(DirectX::XMVectorASin(s)); } \
    static TYPE ACos( TYPE s ) { return TYPE(DirectX::XMVectorACos(s)); } \
    static TYPE ATan( TYPE s ) { return TYPE(DirectX::XMVectorATan(s)); } \
    static TYPE ATan2( TYPE y, TYPE x ) { return TYPE(DirectX::XMVectorATan2(y, x)); } \
    static TYPE Lerp( TYPE a, TYPE b, TYPE t ) { return TYPE(DirectX::XMVectorLerpV(a, b, t)); } \
    static TYPE operator<  ( TYPE lhs, TYPE rhs ) { return DirectX::XMVectorLess(lhs, rhs); } \
    static TYPE operator<= ( TYPE lhs, TYPE rhs ) { return DirectX::XMVectorLessOrEqual(lhs, rhs); } \
    static TYPE operator>  ( TYPE lhs, TYPE rhs ) { return DirectX::XMVectorGreater(lhs, rhs); } \
    static TYPE operator>= ( TYPE lhs, TYPE rhs ) { return DirectX::XMVectorGreaterOrEqual(lhs, rhs); } \
    static TYPE operator== ( TYPE lhs, TYPE rhs ) { return DirectX::XMVectorEqual(lhs, rhs); } \

CREATE_SIMD_FUNCTIONS(bbeVector3)
CREATE_SIMD_FUNCTIONS(bbeVector4)

#undef CREATE_SIMD_FUNCTIONS

static bbeVector4 Normalize3(const bbeVector4& v4)
{
    bbeVector3 v3 = v4;
    v3.Normalize();
    return bbeVector4{ v3.x, v3.y, v3.z, 1.0f };
}

#define bbeBIG_Float (1e10f) // use instead of FLT_MAX when overflowing is a concern

template <typename T>
inline constexpr T bbeSaturate(T v)
{
    return std::clamp(v, static_cast<T>(0), static_cast<T>(1));
}

//damps a value using a deltatime and a speed
template <typename T> inline T bbeDamp(T val, T target, T speed, float dt)
{
    T maxDelta = speed * dt;
    return val + std::clamp(target - val, -maxDelta, maxDelta);
}

template <typename T> T bbeSmoothStep(T min, T max, T f)
{
    f = std::clamp((f - min) / (max - min), T(0.f), T(1.f));
    return f * f * (T(3.f) - T(2.f) * f);
}

template <typename T> T bbeSmootherStep(T min, T max, T f)
{
    f = std::clamp((f - min) / (max - min), T(0.f), T(1.f));
    return f * f * f * (f * (f * T(6.f) - T(15.f)) + T(10.f));
}

template <typename T> constexpr bool bbeIsAligned(T value, size_t alignment)
{
    return ((size_t)value & (alignment - 1)) == 0;
}

template <typename T> constexpr T AlignUpWithMask(T value, size_t mask)
{
    return (T)(((size_t)value + mask) & ~mask);
}

template <typename T> constexpr T AlignDownWithMask(T value, size_t mask)
{
    return (T)((size_t)value & ~mask);
}

template <typename T> constexpr T AlignUp(T value, size_t alignment)
{
    return AlignUpWithMask(value, alignment - 1);
}

template <typename T> constexpr T AlignDown(T value, size_t alignment)
{
    return AlignDownWithMask(value, alignment - 1);
}

template <typename T> constexpr bool IsAligned(T value, size_t alignment)
{
    return 0 == ((size_t)value & (alignment - 1));
}

template <typename T> constexpr T DivideByMultiple(T value, size_t alignment)
{
    return (T)((value + alignment - 1) / alignment);
}

template <typename T> constexpr bool IsPowerOfTwo(T value)
{
    return 0 == (value & (value - 1));
}

template <typename T> constexpr bool IsDivisible(T value, T divisor)
{
    return (value / divisor) * divisor == value;
}

uint8_t Log2(uint64_t value);

template <typename T> constexpr T AlignPowerOfTwo(T value)
{
    return value == 0 ? 0 : 1 << Log2(value);
}

// round given value to next multiple, not limited to pow2 multiples...
constexpr uint32_t bbeRoundToNextMultiple(uint32_t val, uint32_t multiple)
{
    uint32_t t = val + multiple - 1;
    return t - (t % multiple);
}

constexpr uint32_t bbeGetNextPow2(uint32_t n)
{
    --n;
    n = n | (n >> 1);
    n = n | (n >> 2);
    n = n | (n >> 4);
    n = n | (n >> 8);
    n = n | (n >> 16);
    ++n;

    return n;
}

constexpr uint32_t bbeGetLog2(uint32_t val)
{
    uint32_t pow2 = 0;
    for (pow2 = 0; pow2 < 31; ++pow2)
        if (val <= (1u << pow2))
            return pow2;

    return val;
}

bbeMatrix CreateLookAtLH(const bbeVector3& position, const bbeVector3& target, const bbeVector3& up);
bbeMatrix CreatePerspectiveFieldOfViewLH(float fov, float aspectRatio, float nearPlane, float farPlane);

#define bbePI           3.1415926535897932384626433832795f
#define bbe2PI          6.2831853071795864769252867665590f
#define bbePIBy2        1.5707963267948966192313216916398f
#define bbePIBy3        1.0471975511965977461542144610932f
#define bbePIBy4        0.78539816339744830961566084581988f
#define bbePIBy6        0.52359877559829887307710723054658f
#define bbePIBy8        0.39269908169872415480783042290994f
#define bbe3PIBy4       2.3561944901923449288469825374596f
#define bbe3PIBy2       4.7123889803846898576939650749185f
#define bbe3PIBy8       1.1780972450961724644234912687298f
#define bbe5PIBy8       1.9634954084936207740391521145497f
#define bbe7PIBy8       2.7488935718910690836548129603696f
#define bbe1ByPI        0.31830988618379067153776752674508f

constexpr float ConvertToRadians(float fDegrees) { return fDegrees * (bbePI / 180.0f); }
constexpr float ConvertToDegrees(float fRadians) { return fRadians * (180.0f / bbePI); }

#define bbeCos0         1.0f
#define bbeCos1         0.99984769515639123915701155881391f
#define bbeCos2         0.99939082701909573000624344004393f
#define bbeCos3         0.99862953475457387378449205843944f
#define bbeCos4         0.99756405025982424761316268064426f
#define bbeCos5         0.99619469809174553229501040247389f
#define bbeCos6         0.99452189536827333692269194498057f
#define bbeCos10        0.98480775301220805936674302458952f
#define bbeCos15        0.9659258262890682867497431997289f
#define bbeCos20        0.93969262078590838405410927732473f
#define bbeCos22_5      0.92387953251128675612818318939679f
#define bbeCos25        0.90630778703664996324255265675432f
#define bbeCos30        0.86602540378443864676372317075294f
#define bbeCos35        0.81915204428899178968448838591684f
#define bbeCos40        0.76604444311897803520239265055542f
#define bbeCos45        0.70710678118654752440084436210485f
#define bbeCos46        0.69465837045899728665640629942269f
#define bbeCos50        0.64278760968653932632264340990726f
#define bbeCos55        0.57357643635104609610803191282616f
#define bbeCos60        0.5f
#define bbeCos65        0.42261826174069943618697848964773f
#define bbeCos67_5      0.3826834323650897717284599840304f
#define bbeCos70        0.34202014332566873304409961468226f
#define bbeCos75        0.25881904510252076234889883762405f
#define bbeCos80        0.17364817766693034885171662676931f
#define bbeCos85        0.08715574274765817355806427083747f
#define bbeCos87        0.05233595624294383272211862960908f
#define bbeCos90        0.0f
#define bbeCos95        -0.08715574274765817355806427083747f
#define bbeCos100       -0.17364817766693034885171662676931f
#define bbeCos105       -0.25881904510252076234889883762405f
#define bbeCos110       -0.3420201433256687330440996146822f
#define bbeCos112_5     -0.3826834323650897717284599840304f
#define bbeCos115       -0.4226182617406994361869784896477f
#define bbeCos120       -0.5f
#define bbeCos125       -0.57357643635104609610803191282616f
#define bbeCos130       -0.64278760968653932632264340990726f
#define bbeCos135       -0.7071067811865475244008443621048f
#define bbeCos140       -0.76604444311897803520239265055542f
#define bbeCos145       -0.81915204428899178968448838591684f
#define bbeCos150       -0.8660254037844386467637231707529f
#define bbeCos155       -0.90630778703664996324255265675432f
#define bbeCos157_5     -0.92387953251128675612818318939679f
#define bbeCos160       -0.93969262078590838405410927732473f
#define bbeCos165       -0.9659258262890682867497431997289f
#define bbeCos170       -0.98480775301220805936674302458952f
#define bbeCos175       -0.99619469809174553229501040247389f
#define bbeCos180       -1.0f

#define bbeSin10        bbeCos80
#define bbeSin15        bbeCos75
#define bbeSin20        bbeCos70
#define bbeSin30        bbeCos60
#define bbeSin40        bbeCos50
#define bbeSin45        bbeCos45
#define bbeSin50        bbeCos40
#define bbeSin60        bbeCos30
#define bbeSin70        bbeCos20
#define bbeSin75        bbeCos15
#define bbeSin80        bbeCos10
#define bbeSin150       bbeSin30
#define bbeSin165       bbeSin15

#define bbeTan30        0.57735026918962576450914878050196f
#define bbeTan55        1.4281480067421145021606184849985f

#define bbeRad0         0.0f
#define bbeRad1         0.01745329251994329576923690768488f
#define bbeRad2_5       0.04363323129985823942309226921221f
#define bbeRad5         0.08726646259971647884618453842443f
#define bbeRad9         0.15707963267948966192313216916398f
#define bbeRad10        0.17453292519943295769236907684886f
#define bbeRad12_5      0.21816615649929119711546134606108f
#define bbeRad15        0.26179938779914943653855361527329f
#define bbeRad17_5      0.30543261909900767596164588448551f
#define bbeRad20        0.34906585039886591538473815369772f
#define bbeRad22_5      0.39269908169872415480783042290994f
#define bbeRad25        0.43633231299858239423092269212215f
#define bbeRad30        0.52359877559829887307710723054658f
#define bbeRad35        0.61086523819801535192329176897101f
#define bbeRad40        0.69813170079773183076947630739545f
#define bbeRad44        0.76794487087750501384642393813499f
#define bbeRad45        0.78539816339744830961566084581988f
#define bbeRad50        0.87266462599716478846184538424431f
#define bbeRad55        0.95993108859688126730802992266874f
#define bbeRad60        1.0471975511965977461542144610932f
#define bbeRad65        1.1344640137963142250003989995176f
#define bbeRad67_5      1.1780972450961724644234912687298f
#define bbeRad70        1.221730476396030703846583537942f
#define bbeRad75        1.3089969389957471826927680763665f
#define bbeRad80        1.3962634015954636615389526147909f
#define bbeRad90        1.5707963267948966192313216916398f
#define bbeRad100       1.7453292519943295769236907684886f
#define bbeRad105       1.832595714594046055769875306913f
#define bbeRad112_5     1.9634954084936207740391521145497f
#define bbeRad115       2.0071286397934790134622443837619f
#define bbeRad120       2.0943951023931954923084289221863f
#define bbeRad125       2.1816615649929119711546134606108f
#define bbeRad130       2.2689280275926284500007979990352f
#define bbeRad135       2.3561944901923449288469825374596f
#define bbeRad140       2.4434609527920614076931670758841f
#define bbeRad145       2.5307274153917778865393516143085f
#define bbeRad150       2.6179938779914943653855361527329f
#define bbeRad157_5     2.7488935718910690836548129603696f
#define bbeRad160       2.7925268031909273230779052295818f
#define bbeRad165       2.8797932657906438019240897680062f
#define bbeRad170       2.9670597283903602807702743064306f
#define bbeRad180       bbePI
#define bbeRad360       bbe2PI

#define bbeLn2          0.6931471805599453094172321214581f
#define bbeSqrt2        1.4142135623730950488016887242097f
#define bbeSqrt3        1.7320508075688772935274463415059f
#define bbeSqrt5        2.2360679774997896964091736687313f
#define bbeSqrt15       3.8729833462074168851792653997824f
#define bbeSqrtPi       1.772453850905516027298167483341fs

#define bbeNullWithEpsilon(a)                   std::fabs(a) <= FLT_EPSILON
#define bbeEqualWithEpsilon(a, b)               std::fabs(a - b) <= FLT_EPSILON
#define bbeGreaterWithEpsilon(a, b)             ((a) - (b) > popTolerance)
#define bbeGreaterOrEqualWithEpsilon(a, b)      ((b) - (a) < popTolerance)
#define bbeLesserWithEpsilon(a, b)              ((b) - (a) > popTolerance)
#define bbeLesserOrEqualWithEpsilon(a, b)       ((a) - (b) < popTolerance)
