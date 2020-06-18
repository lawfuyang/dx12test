#include <graphic/shapes.h>

void Frustum::Create(const bbeMatrix& viewProjection)
{
    // We are interested in columns of the matrix, so transpose because we can access only rows:
    const bbeMatrix mat = viewProjection.Transpose();

    // Near plane:
    new(&planes[0]) bbePlane{ mat.m[2] };
    planes[0].Normalize();

    // Far plane:
    new(&planes[1]) bbePlane{ bbeVector4{ mat.m[3] } - bbeVector4{ mat.m[2] } };
    planes[1].Normalize();

    // Left plane:
    new(&planes[2]) bbePlane{ bbeVector4{ mat.m[3] } + bbeVector4{ mat.m[0] } };
    planes[2].Normalize();

    // Right plane:
    new(&planes[3]) bbePlane{ bbeVector4{ mat.m[3] } - bbeVector4{ mat.m[0] } };
    planes[3].Normalize();

    // Top plane:
    new(&planes[4]) bbePlane{ bbeVector4{ mat.m[3] } - bbeVector4{ mat.m[1] } };
    planes[4].Normalize();

    // Bottom plane:
    new(&planes[5]) bbePlane{ bbeVector4{ mat.m[3] } + bbeVector4{ mat.m[1] } };
    planes[5].Normalize();
}
