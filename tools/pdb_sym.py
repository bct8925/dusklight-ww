"""Resolve image offsets (RVAs) of an exe to symbols through its PDB (dbghelp, no debugger needed).

    py tools/pdb_sym.py build/windows-msvc-relwithdebinfo/dusklight.exe 0x123456 [more RVAs...]

An RVA is runtime address minus the module base (log it with GetModuleHandle(NULL)).
"""
import ctypes
import ctypes.wintypes as wt
import os
import sys

dbghelp = ctypes.WinDLL("dbghelp")
k32 = ctypes.WinDLL("kernel32")


class SYMBOL_INFO(ctypes.Structure):
    _fields_ = [("SizeOfStruct", wt.ULONG), ("TypeIndex", wt.ULONG), ("Reserved", ctypes.c_uint64 * 2),
                ("Index", wt.ULONG), ("Size", wt.ULONG), ("ModBase", ctypes.c_uint64),
                ("Flags", wt.ULONG), ("Value", ctypes.c_uint64), ("Address", ctypes.c_uint64),
                ("Register", wt.ULONG), ("Scope", wt.ULONG), ("Tag", wt.ULONG),
                ("NameLen", wt.ULONG), ("MaxNameLen", wt.ULONG), ("Name", ctypes.c_char * 1024)]


dbghelp.SymLoadModuleEx.argtypes = [wt.HANDLE, wt.HANDLE, ctypes.c_char_p, ctypes.c_char_p,
                                    ctypes.c_uint64, wt.DWORD, ctypes.c_void_p, wt.DWORD]
dbghelp.SymLoadModuleEx.restype = ctypes.c_uint64
dbghelp.SymFromAddr.argtypes = [wt.HANDLE, ctypes.c_uint64, ctypes.POINTER(ctypes.c_uint64),
                                ctypes.POINTER(SYMBOL_INFO)]
k32.GetCurrentProcess.restype = wt.HANDLE
dbghelp.SymInitialize.argtypes = [wt.HANDLE, ctypes.c_char_p, wt.BOOL]
dbghelp.SymSetOptions.argtypes = [wt.DWORD]


def main() -> int:
    exe = os.path.abspath(sys.argv[1])
    proc = k32.GetCurrentProcess()
    dbghelp.SymSetOptions(0x2 | 0x4)  # UNDNAME | DEFERRED_LOADS
    dbghelp.SymInitialize(proc, None, False)
    base = 0x180000000
    size = os.path.getsize(exe)
    if not dbghelp.SymLoadModuleEx(proc, None, exe.encode(), None, base, size, None, 0):
        print("SymLoadModuleEx failed", ctypes.get_last_error())
        return 1
    for arg in sys.argv[2:]:
        sym = SYMBOL_INFO(SizeOfStruct=88, MaxNameLen=1024)
        disp = ctypes.c_uint64()
        if dbghelp.SymFromAddr(proc, base + int(arg, 0), ctypes.byref(disp), ctypes.byref(sym)):
            print(f"{arg}: {sym.Name.decode(errors='replace')}+0x{disp.value:x} (size {sym.Size})")
        else:
            print(f"{arg}: ?")
    return 0


if __name__ == "__main__":
    sys.exit(main())
