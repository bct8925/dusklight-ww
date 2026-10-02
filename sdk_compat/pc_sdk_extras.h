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
// The decomp writes ATTRIBUTE_ALIGN after the declarator (GCC style). aurora defines it as
// __declspec(align()) on MSVC, which must come first, so use the decomp's empty MSVC definition.
#if defined(_MSC_VER)
#undef ATTRIBUTE_ALIGN
#define ATTRIBUTE_ALIGN(num)
#elif !defined(ATTRIBUTE_ALIGN)
#define ATTRIBUTE_ALIGN(num) __attribute__((aligned(num)))
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
#ifndef M_SQRT2
#define M_SQRT2 1.41421356237309504880f
#endif
#ifndef M_SQRT1_2
#define M_SQRT1_2 0.70710678118654752440f
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

// dolphin/card.h: the decomp names the CARD result codes CARD_ERROR_*.
#include <dolphin/card.h>
#define CARD_ERROR_READY CARD_RESULT_READY
#define CARD_ERROR_BROKEN CARD_RESULT_BROKEN
#define CARD_ERROR_ENCODING CARD_RESULT_ENCODING
#define CARD_ERROR_EXIST CARD_RESULT_EXIST
#define CARD_ERROR_FATAL_ERROR CARD_RESULT_FATAL_ERROR
#define CARD_ERROR_IOERROR CARD_RESULT_IOERROR
#define CARD_ERROR_NOCARD CARD_RESULT_NOCARD
#define CARD_ERROR_NOFILE CARD_RESULT_NOFILE
#define CARD_ERROR_WRONGDEVICE CARD_RESULT_WRONGDEVICE

// GX immediate-mode color, as the decomp names it.
#define GXColor4x8 GXColor4u8

// The decomp's GXSetDrawSync waits for the GPU to reach a token; aurora has no equivalent and
// orders work itself.
static inline void GXSetDrawSync(GXBool enable) {
    (void)enable;
}

#ifdef __cplusplus
// The decomp passes thread entry points as void*.
inline BOOL OSCreateThread(OSThread* thread, void* func, void* param, void* stack, u32 stackSize,
                           OSPriority priority, u16 attr) {
    return OSCreateThread(thread, (void* (*)(void*))func, param, stack, stackSize, priority, attr);
}
#endif

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
