#!/usr/bin/env python3
"""Run a program under the Windows debug API and report where it crashes, without a debugger.

On the first fatal exception (access violation, illegal instruction, stack overflow, ...) this
prints the faulting function and source line from the PDB, plus a heuristic backtrace: values on
the stack that are return addresses into the program's own modules. Windows only.

    python tools/crashtrace.py build/windows-msvc-relwithdebinfo/dusklight.exe [args...]

With --sample N (before the program path), it also stops after N seconds, prints where the main
thread is (for hangs), and ends the program:

    python tools/crashtrace.py --sample 15 build/windows-msvc-relwithdebinfo/dusklight.exe [args...]
"""

import ctypes
import ctypes.wintypes as wt
import os
import subprocess
import sys
import time

k32 = ctypes.WinDLL("kernel32", use_last_error=True)
dbghelp = ctypes.WinDLL("dbghelp", use_last_error=True)

DEBUG_ONLY_THIS_PROCESS = 0x2
EXCEPTION_DEBUG_EVENT, CREATE_PROCESS_DEBUG_EVENT, EXIT_PROCESS_DEBUG_EVENT = 1, 3, 5
LOAD_DLL_DEBUG_EVENT, OUTPUT_DEBUG_STRING_EVENT = 6, 8
DBG_CONTINUE, DBG_EXCEPTION_NOT_HANDLED = 0x00010002, 0x80010001
# Exceptions that are not crashes: breakpoints, C++ throws, MSVC thread naming, debug output.
BENIGN = {0x80000003, 0x4000001F, 0xE06D7363, 0x406D1388, 0x40010006, 0x4001000A}
NAMES = {0xC0000005: "access violation", 0xC000001D: "illegal instruction",
         0xC00000FD: "stack overflow", 0xC0000094: "integer divide by zero",
         0xC0000409: "stack buffer overrun / fail fast", 0x80000003: "breakpoint"}


class EXCEPTION_RECORD(ctypes.Structure):
    _fields_ = [("ExceptionCode", wt.DWORD), ("ExceptionFlags", wt.DWORD),
                ("ExceptionRecord", ctypes.c_void_p), ("ExceptionAddress", ctypes.c_void_p),
                ("NumberParameters", wt.DWORD), ("ExceptionInformation", ctypes.c_uint64 * 15)]


class EXCEPTION_DEBUG_INFO(ctypes.Structure):
    _fields_ = [("ExceptionRecord", EXCEPTION_RECORD), ("dwFirstChance", wt.DWORD)]


class DEBUG_EVENT(ctypes.Structure):
    _fields_ = [("dwDebugEventCode", wt.DWORD), ("dwProcessId", wt.DWORD),
                ("dwThreadId", wt.DWORD), ("u", ctypes.c_uint64 * 21)]


class STARTUPINFO(ctypes.Structure):
    _fields_ = [("cb", wt.DWORD), ("lpReserved", wt.LPWSTR), ("lpDesktop", wt.LPWSTR),
                ("lpTitle", wt.LPWSTR), ("dwX", wt.DWORD), ("dwY", wt.DWORD),
                ("dwXSize", wt.DWORD), ("dwYSize", wt.DWORD), ("dwXCountChars", wt.DWORD),
                ("dwYCountChars", wt.DWORD), ("dwFillAttribute", wt.DWORD), ("dwFlags", wt.DWORD),
                ("wShowWindow", wt.WORD), ("cbReserved2", wt.WORD), ("lpReserved2", ctypes.c_void_p),
                ("hStdInput", wt.HANDLE), ("hStdOutput", wt.HANDLE), ("hStdError", wt.HANDLE)]


class PROCESS_INFORMATION(ctypes.Structure):
    _fields_ = [("hProcess", wt.HANDLE), ("hThread", wt.HANDLE),
                ("dwProcessId", wt.DWORD), ("dwThreadId", wt.DWORD)]


class SYMBOL_INFO(ctypes.Structure):
    _fields_ = [("SizeOfStruct", wt.ULONG), ("TypeIndex", wt.ULONG), ("Reserved", ctypes.c_uint64 * 2),
                ("Index", wt.ULONG), ("Size", wt.ULONG), ("ModBase", ctypes.c_uint64),
                ("Flags", wt.ULONG), ("Value", ctypes.c_uint64), ("Address", ctypes.c_uint64),
                ("Register", wt.ULONG), ("Scope", wt.ULONG), ("Tag", wt.ULONG),
                ("NameLen", wt.ULONG), ("MaxNameLen", wt.ULONG), ("Name", ctypes.c_char * 1024)]


class IMAGEHLP_LINE64(ctypes.Structure):
    _fields_ = [("SizeOfStruct", wt.DWORD), ("Key", ctypes.c_void_p), ("LineNumber", wt.DWORD),
                ("FileName", ctypes.c_char_p), ("Address", ctypes.c_uint64)]


dbghelp.SymFromAddr.argtypes = [wt.HANDLE, ctypes.c_uint64, ctypes.POINTER(ctypes.c_uint64),
                                ctypes.POINTER(SYMBOL_INFO)]
dbghelp.SymGetLineFromAddr64.argtypes = [wt.HANDLE, ctypes.c_uint64, ctypes.POINTER(wt.DWORD),
                                         ctypes.POINTER(IMAGEHLP_LINE64)]
dbghelp.SymLoadModuleEx.argtypes = [wt.HANDLE, wt.HANDLE, ctypes.c_char_p, ctypes.c_char_p,
                                    ctypes.c_uint64, wt.DWORD, ctypes.c_void_p, wt.DWORD]
dbghelp.SymLoadModuleEx.restype = ctypes.c_uint64
k32.ReadProcessMemory.argtypes = [wt.HANDLE, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t,
                                  ctypes.POINTER(ctypes.c_size_t)]
k32.OpenThread.restype = wt.HANDLE
k32.GetThreadContext.argtypes = [wt.HANDLE, ctypes.c_void_p]
k32.GetFinalPathNameByHandleA.argtypes = [wt.HANDLE, ctypes.c_char_p, wt.DWORD, wt.DWORD]

CONTEXT_FULL_AMD64 = 0x10000B
CTX_FLAGS, CTX_RSP, CTX_RIP = 0x30, 0x98, 0xF8


def describe(proc, addr):
    sym = SYMBOL_INFO(SizeOfStruct=88, MaxNameLen=1024)  # sizeof(SYMBOL_INFO) in C
    disp = ctypes.c_uint64()
    if not dbghelp.SymFromAddr(proc, addr, ctypes.byref(disp), ctypes.byref(sym)):
        return None
    text = f"{sym.Name.decode(errors='replace')}+0x{disp.value:x}"
    line = IMAGEHLP_LINE64(SizeOfStruct=ctypes.sizeof(IMAGEHLP_LINE64))
    ldisp = wt.DWORD()
    if dbghelp.SymGetLineFromAddr64(proc, addr, ctypes.byref(ldisp), ctypes.byref(line)):
        text += f"  ({line.FileName.decode(errors='replace')}:{line.LineNumber})"
    return text


def read(proc, addr, size):
    buf = ctypes.create_string_buffer(size)
    got = ctypes.c_size_t()
    k32.ReadProcessMemory(proc, addr, buf, size, ctypes.byref(got))
    return buf.raw[:got.value]


def image_size(proc, base):
    header = read(proc, base, 0x400)
    if len(header) < 0x40:
        return 0
    nt = int.from_bytes(header[0x3C:0x40], "little")
    return int.from_bytes(header[nt + 0x50:nt + 0x54], "little") if nt + 0x54 <= len(header) else 0


def module_path(handle):
    buf = ctypes.create_string_buffer(1024)
    n = k32.GetFinalPathNameByHandleA(handle, buf, 1024, 0)
    return buf.value[4:] if buf.value.startswith(b"\\\\?\\") else buf.value if n else None


def report(proc, tid, rec, modules):
    code = rec.ExceptionCode
    print(f"\n*** {NAMES.get(code, 'exception')} (0x{code:08X}) at 0x{rec.ExceptionAddress or 0:x}")
    if code == 0xC0000005 and rec.NumberParameters >= 2:
        kind = {0: "reading", 1: "writing", 8: "executing"}.get(rec.ExceptionInformation[0], "?")
        print(f"    {kind} address 0x{rec.ExceptionInformation[1]:x}")
    print(f"    in {describe(proc, rec.ExceptionAddress or 0) or '?'}")
    print_stack(proc, tid, modules)


def print_stack(proc, tid, modules, show_pc=False):
    thread = k32.OpenThread(0x1FFFFF, False, tid)
    ctx = (ctypes.c_byte * (1232 + 16))()
    base = (ctypes.addressof(ctx) + 15) & ~15
    ctypes.c_uint32.from_address(base + CTX_FLAGS).value = CONTEXT_FULL_AMD64
    if not k32.GetThreadContext(thread, base):
        return
    if show_pc:
        rip = ctypes.c_uint64.from_address(base + CTX_RIP).value
        print(f"    at {describe(proc, rip) or hex(rip)}")
    rsp = ctypes.c_uint64.from_address(base + CTX_RSP).value
    stack = read(proc, rsp, 0x4000)
    print("    probable callers (return addresses on the stack):")
    shown = 0
    for off in range(0, len(stack) - 7, 8):
        val = int.from_bytes(stack[off:off + 8], "little")
        if not any(lo <= val < hi for lo, hi in modules):
            continue
        text = describe(proc, val)
        if text:
            print(f"      [rsp+0x{off:04x}] {text}")
            shown += 1
            if shown >= 40:
                break


def main() -> int:
    args = sys.argv[1:]
    sample_after = None
    if len(args) >= 2 and args[0] == "--sample":
        sample_after = float(args[1])
        args = args[2:]
    if not args:
        print(__doc__)
        return 2
    cmdline = subprocess.list2cmdline([os.path.abspath(args[0])] + args[1:])
    si, pi = STARTUPINFO(cb=ctypes.sizeof(STARTUPINFO)), PROCESS_INFORMATION()
    if not k32.CreateProcessW(None, ctypes.create_unicode_buffer(cmdline), None, None, False,
                              DEBUG_ONLY_THIS_PROCESS, None, None, ctypes.byref(si), ctypes.byref(pi)):
        raise OSError(ctypes.get_last_error(), "CreateProcess failed")
    proc = pi.hProcess
    dbghelp.SymSetOptions(0x2 | 0x10)  # UNDNAME | LOAD_LINES
    dbghelp.SymInitialize(proc, None, False)
    modules = []  # (start, end) of the program's own modules (exe and non-system DLLs)
    ev = DEBUG_EVENT()
    deadline = time.monotonic() + sample_after if sample_after is not None else None
    while True:
        if not k32.WaitForDebugEvent(ctypes.byref(ev), 200):
            if deadline is not None and time.monotonic() >= deadline:
                k32.SuspendThread(pi.hThread)
                print(f"\n*** main thread after {sample_after:g} s:")
                print_stack(proc, pi.dwThreadId, modules, show_pc=True)
                k32.TerminateProcess(proc, 1)
                deadline = None
            continue
        status = DBG_CONTINUE
        code = ev.dwDebugEventCode
        if code in (CREATE_PROCESS_DEBUG_EVENT, LOAD_DLL_DEBUG_EVENT):
            # hFile, then (process: hProcess, hThread) base address
            words = (ctypes.c_uint64 * 4).from_buffer(ev.u)
            hfile, base = words[0], words[3] if code == CREATE_PROCESS_DEBUG_EVENT else words[1]
            path = module_path(hfile) if hfile else None
            dbghelp.SymLoadModuleEx(proc, hfile or None, path, None, base, 0, None, 0)
            if path and b"\\windows\\" not in path.lower():
                modules.append((base, base + image_size(proc, base)))
            if hfile:
                k32.CloseHandle(wt.HANDLE(hfile))
        elif code == OUTPUT_DEBUG_STRING_EVENT:
            pass
        elif code == EXCEPTION_DEBUG_EVENT:
            info = EXCEPTION_DEBUG_INFO.from_buffer(ev.u)
            rec = info.ExceptionRecord
            if rec.ExceptionCode not in BENIGN or not info.dwFirstChance:
                if rec.ExceptionCode not in BENIGN:
                    report(proc, ev.dwThreadId, rec, modules)
                    k32.TerminateProcess(proc, 1)
            status = DBG_CONTINUE if rec.ExceptionCode == 0x80000003 else DBG_EXCEPTION_NOT_HANDLED
        elif code == EXIT_PROCESS_DEBUG_EVENT:
            exit_code = ctypes.c_uint32.from_buffer(ev.u).value
            print(f"\nprocess exited with code {exit_code} (0x{exit_code:08X})")
            return 0
        k32.ContinueDebugEvent(ev.dwProcessId, ev.dwThreadId, status)


if __name__ == "__main__":
    sys.exit(main())
