#ifndef _global_h_
#define _global_h_

#include "dolphin/types.h"
#if TARGET_PC
#include "pc_sdk_extras.h"
#endif

#define ARRAY_SIZE(o) (sizeof(o) / sizeof(o[0]))
#define ARRAY_SSIZE(o) ((int)(sizeof(o) / sizeof(o[0])))

// Align X to the previous N bytes (N must be power of two)
#define ALIGN_PREV(X, N) ((X) & ~((N)-1))
// Align X to the next N bytes (N must be power of two)
#define ALIGN_NEXT(X, N) ALIGN_PREV(((X) + (N-1)), N)
#define IS_ALIGNED(X, N) (((X) & ((N)-1)) == 0)
#define IS_NOT_ALIGNED(X, N) (((X) & ((N)-1)) != 0)

// Silence unused parameter warnings.
// TP debug suggests the original devs used something like this.
#define UNUSED(x) ((void)(x))

#define JUT_EXPECT(...)
#define ASSERT(...)
#define LOGF(FMT, ...)

#define _SDA_BASE_(dummy) 0
#define _SDA2_BASE_(dummy) 0

#ifndef offsetof
#define offsetof(type, member) ((size_t) & (((type*)0)->member))
#endif

#define SQUARE(x) ((x) * (x))

#ifdef __MWERKS__
#define GLUE(a, b) a##b
#define GLUE2(a, b) GLUE(a, b)
#define STATIC_ASSERT(cond) typedef char GLUE2(static_assertion_failed, __LINE__)[(cond) ? 1 : -1]
#define ALIGN_DECL(alignment, decl) decl ATTRIBUTE_ALIGN(alignment)
#define SECTION_DATA __declspec(section ".data")
#define SECTION_INIT __declspec(section ".init")
#define ASM asm
#define WEAKFUNC __declspec(weak)
#else
#define STATIC_ASSERT(...)
#define ALIGN_DECL(alignment, decl) ATTRIBUTE_ALIGN(alignment) decl
#define SECTION_DATA
#define SECTION_INIT
#define ASM
#define WEAKFUNC
#endif

// Intrinsics
#if !TARGET_PC
extern int __cntlzw(uint);
extern int __rlwimi(int, int, int, int, int);
extern void __dcbz(void*, int);
extern void __sync();
#else
// Implemented in C by src/dusk/extras.c.
#ifdef __cplusplus
extern "C" {
#endif
extern int __cntlzw(unsigned int);
extern int __rlwimi(int, int, int, int, int);
extern void __dcbf(void*, int);
extern void __dcbz(void*, int);
extern void __sync();
extern int __abs(int);
#ifdef __cplusplus
}
#endif
#endif

#define VERSION_DEMO 0
#define VERSION_JPN 1
#define VERSION_USA 2
#define VERSION_PAL 3

#if VERSION == VERSION_DEMO
    #define VERSION_SELECT(DEMO, JPN, USA, PAL) (DEMO)
    #define DEMO_SELECT(DEMO, RETAIL) (DEMO)
#elif VERSION <= VERSION_JPN
    #define VERSION_SELECT(DEMO, JPN, USA, PAL) (JPN)
    #define DEMO_SELECT(DEMO, RETAIL) (RETAIL)
#elif VERSION == VERSION_USA
    #define VERSION_SELECT(DEMO, JPN, USA, PAL) (USA)
    #define DEMO_SELECT(DEMO, RETAIL) (RETAIL)
#elif VERSION == VERSION_PAL
    #define VERSION_SELECT(DEMO, JPN, USA, PAL) (PAL)
    #define DEMO_SELECT(DEMO, RETAIL) (RETAIL)
#endif

#ifdef __MWERKS__
#define SJIS(character, value) character
#else
#define SJIS(character, value) ((u32)value)
#endif

// Hack to make strings with no references appear in the string pool for matching.
#define DEAD_STRING(s) OSReport(s)

// Hack to trick the compiler into not inlining functions that use this macro.
#define FORCE_DONT_INLINE \
    (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; \
    (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; \
    (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; \
    (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; \
    (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; \
    (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; \
    (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; \
    (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; \
    (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; \
    (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; \
    (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; \
    (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; \
    (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; \
    (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; \
    (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; \
    (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0; (void*)0;

#if TARGET_PC
// Porting helpers, adapted from dusklight's global.h. (DEBUG is deliberately left undefined:
// TWW tests it with both #ifdef and #if.)

#if defined(_MSVC_LANG) && !defined(__clang__)
#define __memcpy memcpy
inline int __builtin_clz(unsigned int v) {
    int count = 32;
    while (v != 0) {
        count--;
        v >>= 1;
    }
    return count;
}
#define COMPOUND_LITERAL(x)
#else
#define __memcpy __builtin_memcpy
#define COMPOUND_LITERAL(x) (x)
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// Exe data exports only matter to dusklight's code mods, which this port does not build.
#define DUSK_GAME_DATA
#define DUSK_GAME_EXTERN extern

#if defined(_MSC_VER)
#define DUSK_NOINLINE __declspec(noinline)
#elif defined(__GNUC__)
#define DUSK_NOINLINE __attribute__((noinline))
#else
#define DUSK_NOINLINE
#endif

#if __cplusplus
#define TYPEOF(value) decltype(value)
#else
#define TYPEOF(value) __typeof__(value)
#endif
#define POINTER_ADD_TYPE(type_, ptr_, offset_) ((type_)((uintptr_t)(ptr_) + (uintptr_t)(offset_)))
#define POINTER_ADD(ptr_, offset_) POINTER_ADD_TYPE(TYPEOF(ptr_), ptr_, offset_)

// Multi-character literals longer than 4 bytes (e.g. 'ari_os'). MWCC packs every character
// big-endian into the integer; GCC, Clang and MSVC truncate to int, so build the value here.
#if __cplusplus
template <int N>
inline constexpr unsigned long long MultiCharLiteral(const char (&buf)[N]) {
    static_assert(N - 1 >= 3 && N - 1 <= 10, "MULTI_CHAR literal must be 1-8 characters");
    unsigned long long out = 0;
    for (int i = 1; i < N - 2; i++) {
        out = (out << 8) | static_cast<unsigned char>(buf[i]);
    }
    return out;
}
#define MULTI_CHAR(x) MultiCharLiteral(#x)

#include <cmath>
#include <math.h>
using std::isnan;
#endif

// Comparing a reference's address to NULL is required to match on MWCC but is always false
// for valid references; modern compilers warn about it.
#define IS_REF_NULL(r) (0)
#define IS_REF_NONNULL(r) (1)

#define CRASH(msg) OSPanic(__FILE__, __LINE__, "%s", msg)

#define IF_DUSK(statement) statement
#define IF_DUSK_BLOCK(cond) if (cond) {
#define IF_DUSK_BLOCK_END }
#define IF_DUSK_ARG(expr) , expr
#define IF_NOT_DUSK(statement)
#define DUSK_IF_ELSE(dusk, orig) dusk
#else
#define COMPOUND_LITERAL(x) (x)
#define MULTI_CHAR(x) (x)
#define IS_REF_NULL(r) (&(r) == NULL)
#define IS_REF_NONNULL(r) (&(r) != NULL)
#define IF_DUSK(statement)
#define IF_DUSK_BLOCK(cond)
#define IF_DUSK_BLOCK_END
#define IF_DUSK_ARG(expr)
#define IF_NOT_DUSK(statement) statement
#define DUSK_IF_ELSE(dusk, orig) orig
#endif

#define DUSK_CONST IF_DUSK(const)
#define DUSK_CONSTEXPR IF_DUSK(constexpr)

#endif
