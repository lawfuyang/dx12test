#include <system/math.h>

uint8_t Log2(uint64_t value)
{
    unsigned long mssb; // most significant set bit
    unsigned long lssb; // least significant set bit

    // If perfect power of two (only one set bit), return index of bit.  Otherwise round up
    // fractional log by adding 1 to most signicant set bit's index.
    if (_BitScanReverse64(&mssb, value) > 0 && _BitScanForward64(&lssb, value) > 0)
        return uint8_t(mssb + (mssb == lssb ? 0 : 1));
    else
        return 0;
}

bbeMatrix CreateLookAtLH(const bbeVector3& eye, const bbeVector3& target, const bbeVector3& up)
{
    using namespace DirectX;

    const XMVECTOR eyev = XMLoadFloat3(&eye);
    const XMVECTOR targetv = XMLoadFloat3(&target);
    const XMVECTOR upv = XMLoadFloat3(&up);

    bbeMatrix R;
    XMStoreFloat4x4(&R, XMMatrixLookAtLH(eyev, targetv, upv));
    return R;
}

bbeMatrix CreatePerspectiveFieldOfViewLH(float fov, float aspectRatio, float nearPlane, float farPlane)
{
    using namespace DirectX;

    bbeMatrix R;
    XMStoreFloat4x4(&R, XMMatrixPerspectiveFovLH(fov, aspectRatio, nearPlane, farPlane));
    return R;
}
