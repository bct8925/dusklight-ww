// Forwards the decomp's Dolphin SDK header path to the aurora header that provides it.
// The decomp's GX.h also brings in the matrix and vector types.
#ifndef SDK_COMPAT_DOLPHIN_GX_GX_H
#define SDK_COMPAT_DOLPHIN_GX_GX_H
#include <dolphin/gx.h>
#include <dolphin/mtx.h>
#endif
