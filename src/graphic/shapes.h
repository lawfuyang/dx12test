#pragma once

struct AABB;
struct Sphere;

struct Frustum
{
    void Create(const bbeMatrix& viewProjection);

    const bbePlane& GetNearPlane()   const { return planes[0]; }
    const bbePlane& GetFarPlane()    const { return planes[1]; }
    const bbePlane& GetLeftPlane()   const { return planes[2]; }
    const bbePlane& GetRightPlane()  const { return planes[3]; }
    const bbePlane& GetTopPlane()    const { return planes[4]; }
    const bbePlane& GetBottomPlane() const { return planes[5]; }

private:
    bbePlane planes[6];
};

bool Intersect(const AABB& aabb, const bbeVector3& point);
bool Intersect(const AABB& aabb, const Sphere& sphere);

bool Intersect(const Sphere& sphere, const AABB& b);
bool Intersect(const Sphere& sphere, const Sphere& b);

bool Intersect(const Frustum& frustum, const bbeVector3& point);
bool Intersect(const Frustum& frustum, const Sphere& sphere);
bool Intersect(const Frustum& frustum, const AABB& b);
