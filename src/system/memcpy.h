#pragma once

void SIMDMemCopy(void* __restrict Dest, const void* __restrict Source, size_t NumQuadwords);
void SIMDMemFill(void* __restrict Dest, const bbeVector4& FillVector, size_t NumQuadwords);
