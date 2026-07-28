// Copyright (c) Red Alert 4 project.
#include "RA4Navigation/Formation.h"

namespace RA4
{

Vec2 RotateOffset(const Vec2& Offset, int32_t Facing)
{
    // 2D rotation by the fixed-point angle. FxCos/FxSin are in Fixed.h.
    const Fixed C = FxCos(Facing);
    const Fixed S = FxSin(Facing);
    return Vec2(Offset.X * C - Offset.Y * S, Offset.X * S + Offset.Y * C);
}

} // namespace RA4