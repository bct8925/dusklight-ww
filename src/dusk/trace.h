#pragma once

// Boot tracing for bringing up Wind Waker (--trace): process creation, loading phase steps and
// DVD commands are written to the log. Off by default; DUSK_TRACE costs one branch when off.

#include <cstddef>

namespace dusk {
extern bool TraceEnabled;

void Trace(const char* fmt, ...);
// Name of the function containing `addr` (from the PDB), or its address if unknown.
const char* TraceSymbol(const void* addr);
// Name of a process profile (f_pc_name.h), e.g. "LOGO_SCENE".
const char* TraceProcName(int procName);
}  // namespace dusk

#define DUSK_TRACE(...)                                                                            \
    do {                                                                                           \
        if (dusk::TraceEnabled) {                                                                  \
            dusk::Trace(__VA_ARGS__);                                                              \
        }                                                                                          \
    } while (0)
