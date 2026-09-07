/* LEGOLAND -- scope AG: CRT → GameMain wrapper.
 *
 * The PE entry calls this stdcall WinMain. It fills g_exe_version and runs
 * GameMain (startup.c) under __try; the filter writes exceptlog.txt via
 * WriteExceptionReport (exceptlog.c) and returns 0 so the exception keeps
 * searching (the empty handler is never taken).
 *
 * VC6 SP3 /O2 /Gy /Gd. Verification and notes: docs/lanes/scope-ag.md.
 */

typedef struct EXCEPTION_RECORD EXCEPTION_RECORD;
typedef struct CONTEXT CONTEXT;
typedef struct EXCEPTION_POINTERS {
    EXCEPTION_RECORD* ExceptionRecord;
    CONTEXT*          ContextRecord;
} EXCEPTION_POINTERS;

extern char g_exe_version[];         /* 0x0066752c  ProductVersion */
extern void ReadExeVersionString(char* out);                                 /* 0x00458830 */
extern int  GameMain(void* hinst, void* hprev, char* cmdline, int ncmdshow); /* 0x0047fd10 */
extern int  WriteExceptionReport(EXCEPTION_POINTERS* ep, const char* where); /* 0x00453da0 */

void* __cdecl _exception_info(void);

/* matchfull 48/48, 143B. audit walks 31i/93B ESCAPES: the try body ends in
 * `jmp` over the filter/handler, which only the .rdata scope table reaches
 * (scope N). WriteExceptionReport / ReportModuleDetails walk in full because
 * earlier forwards already set `furthest` past that jmp. No C spelling moves
 * the original's terminator. */
// WIP-FUNCTION: LEGOLAND 0x00453d10  (100% matchfull 48/48; audit 31i/93B ESCAPES, SEH jmp-over-filter)
int __stdcall WinMain(void* hinst, void* hprev, char* cmdline, int ncmdshow)
{
    int r = -1;

    __try {
        ReadExeVersionString(g_exe_version);
        r = GameMain(hinst, hprev, cmdline, ncmdshow);
    } __except (WriteExceptionReport((EXCEPTION_POINTERS*)_exception_info(), "main thread")) {
    }
    return r;
}
