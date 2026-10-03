#include "dusk/trace.h"
#include "dusk/main.h"

#include <borealis/log.hpp>

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <mutex>

#if _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")
#endif

extern const char* const g_fpcPfLst_ProfileNames[];
extern const int g_fpcPfLst_ProfileNum;

namespace dusk {
namespace {
constexpr borealis::Log TraceLog{"trace"};
std::mutex sSymbolMutex;
}  // namespace

bool TraceEnabled = false;

void Trace(const char* fmt, ...) {
    char buf[512];
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    TraceLog.info("{}", buf);

    // Also write straight to trace.txt, flushed per line, so the last lines before a crash
    // survive (the log file is buffered).
    static std::mutex sFileMutex;
    std::lock_guard lock(sFileMutex);
    static FILE* sFile = [] {
        std::filesystem::path path = CachePath / "trace.txt";
#if _WIN32
        return _wfopen(path.c_str(), L"w");
#else
        return std::fopen(path.c_str(), "w");
#endif
    }();
    if (sFile != nullptr) {
        std::fputs(buf, sFile);
        std::fputc('\n', sFile);
        std::fflush(sFile);
    }
}

const char* TraceSymbol(const void* addr) {
    // Callers log the result immediately; one buffer per thread is enough.
    thread_local char name[256];
    std::snprintf(name, sizeof(name), "%p", addr);
#if _WIN32
    std::lock_guard lock(sSymbolMutex);  // DbgHelp is single-threaded
    static bool initialized = [] {
        SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
        // Fails harmlessly if borealis' crash handler already initialized it.
        SymInitialize(GetCurrentProcess(), nullptr, TRUE);
        return true;
    }();
    (void)initialized;
    alignas(SYMBOL_INFO) char storage[sizeof(SYMBOL_INFO) + 200];
    auto* sym = reinterpret_cast<SYMBOL_INFO*>(storage);
    sym->SizeOfStruct = sizeof(SYMBOL_INFO);
    sym->MaxNameLen = 200;
    // Incremental linking routes function pointers through "ILT" jump thunks (jmp rel32);
    // name the function the thunk jumps to.
    auto* code = static_cast<const unsigned char*>(addr);
    if (code != nullptr && code[0] == 0xE9) {
        int rel;
        std::memcpy(&rel, code + 1, sizeof(rel));
        addr = code + 5 + rel;
    }
    DWORD64 displacement = 0;
    if (SymFromAddr(GetCurrentProcess(), reinterpret_cast<DWORD64>(addr), &displacement, sym)) {
        std::snprintf(name, sizeof(name), "%s", sym->Name);
    }
#endif
    return name;
}

const char* TraceProcName(int procName) {
    if (procName >= 0 && procName < g_fpcPfLst_ProfileNum) {
        return g_fpcPfLst_ProfileNames[procName];
    }
    return "?";
}
}  // namespace dusk
