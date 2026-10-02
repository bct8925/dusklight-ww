// Forwards the decomp's Dolphin SDK header path to the aurora header that provides it.
// The decomp's OS.h also brings in stdarg.h and the DVD API.
#ifndef SDK_COMPAT_DOLPHIN_OS_OS_H
#define SDK_COMPAT_DOLPHIN_OS_OS_H
#include <stdarg.h>
#include <dolphin/os.h>
#include <dolphin/dvd.h>
#endif
