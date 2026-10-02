// Definitions the decomp's own Dolphin SDK and MSL headers provide but aurora's SDK does not.
// Included from global.h on PC only.
#ifndef SDK_COMPAT_PC_SDK_EXTRAS_H
#define SDK_COMPAT_PC_SDK_EXTRAS_H

#include <dolphin/types.h>

// dolphin/types.h
typedef unsigned int uint;

// dolphin/mtx/vec.h (aurora only has the anonymous S16Vec)
typedef struct SVec {
    s16 x, y, z;
} SVec;

// dolphin/mtx/mtx.h
typedef f32 Mtx33[3][3];
typedef f32 Mtx23[2][3];
typedef f32 (*MtxP)[4];
typedef f32 (*Mtx3P)[3];
typedef const f32 (*CMtxP)[4];

#ifdef __cplusplus
#include <cmath>

// MSL math.h: the PowerPC reciprocal square root estimate. Callers refine the estimate with
// Newton-Raphson steps, so returning the exact value only makes those steps no-ops.
inline double __frsqrte(double x) {
    return 1.0 / std::sqrt(x);
}
#endif

#endif
