// Definitions the decomp's own Dolphin SDK and MSL headers provide but aurora's SDK does not.
// Included from global.h on PC only.
#ifndef SDK_COMPAT_PC_SDK_EXTRAS_H
#define SDK_COMPAT_PC_SDK_EXTRAS_H

#include <dolphin/types.h>

#include <stddef.h>
#include <stdint.h>

// dolphin/types.h
typedef unsigned int uint;

#define READU32_BE(ptr, offset)                                                                    \
    (((u32)ptr[offset] << 24) | ((u32)ptr[offset + 1] << 16) | ((u32)ptr[offset + 2] << 8) |       \
     (u32)ptr[offset + 3]);

#ifndef AT_ADDRESS
#define AT_ADDRESS(addr)
#endif
#ifndef ATTRIBUTE_ALIGN
#if defined(_MSC_VER)
#define ATTRIBUTE_ALIGN(num)
#else
#define ATTRIBUTE_ALIGN(num) __attribute__((aligned(num)))
#endif
#endif
#ifndef DECL_WEAK
#if defined(_MSC_VER)
#define DECL_WEAK
#else
#define DECL_WEAK __attribute__((weak))
#endif
#endif
#define __REGISTER

#define FLOAT_MIN (1.175494351e-38f)
#define FLOAT_MAX (3.40282346638528860e+38f)

// dolphin/os/OSAlloc.h. The pointer versions go through uintptr_t instead of u32.
#ifndef OSRoundUp32B
#define OSRoundUp32B(x) (((u32)(x) + 0x1F) & ~(0x1F))
#define OSRoundDown32B(x) (((u32)(x)) & ~(0x1F))
#endif
#ifndef OSRoundUp
#define OSRoundUp(x, align) (((x) + (align)-1) & (-(align)))
#define OSRoundDown(x, align) ((x) & (-(align)))
#endif
#define OSRoundUpPtr(x, align) ((void*)((((uintptr_t)(x)) + (align)-1) & (~((uintptr_t)(align)-1))))
#define OSRoundDownPtr(x, align) ((void*)(((uintptr_t)(x)) & (~((uintptr_t)(align)-1))))

// MSL math.h
#ifndef DEG_TO_RAD
#define DEG_TO_RAD(degrees) (degrees * (3.14159265358979323846f / 180.0f))
#endif
#ifndef RAD_TO_DEG
#define RAD_TO_DEG(radians) (radians * (180.0f / 3.14159265358979323846f + 0.000005f))
#endif

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

// GX hardware register enums from the decomp SDK.
#include "pc_gx_hw_enums.h"

// dolphin/dvd/dvd.h
#include <dolphin/dvd.h>
typedef DVDDir DVDDirectory;
typedef DVDDirEntry DVDDirectoryEntry;
#define DVDGetLength(fi) (fi)->length

#ifdef __cplusplus
#include <cstdarg>
// MSL stdarg.h wraps va_list in a struct, which JUTDirectPrint and JUTConsole pass around.
namespace std {
struct __tag_va_List {
    va_list list;
};
}  // namespace std
#endif

#ifdef __cplusplus
#include <cmath>

// MSL math.h: the PowerPC reciprocal square root estimate. Callers refine the estimate with
// Newton-Raphson steps, so returning the exact value only makes those steps no-ops.
inline double __frsqrte(double x) {
    return 1.0 / std::sqrt(x);
}

// MSL math.h: the PowerPC reciprocal estimate, likewise exact here.
inline float __fres(float x) {
    return 1.0f / x;
}
#endif

#endif
