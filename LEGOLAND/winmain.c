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

#ifdef LEGOLAND_PORTABLE
/* No structured exception handling off Win32 (same stand-in as exceptlog.c):
 * the guarded block runs unguarded and the filter is dead code. */
#define __try if (1)
#define __except(x) else if (0)
#endif

/* Gate note: the try body is straight-line and ends in `jmp` over the
 * filter/handler blocks, which only the .rdata scope table (0x004ab4e0)
 * reaches, so the extent walker used to stop at 31i/93B and report ESCAPES.
 * match.py's true_extent now reads the SEH scope table: entering trylevel K
 * (`mov [ebp-4], K`) makes entry K's filter and handler branch targets, and
 * the body walks its full 48i/143B (audit [OK], 0 mismatch). The
 * `mov [ebp-4], esi` after GameMain restores trylevel -1 through the
 * `r = -1` register, not an immediate. */
// FUNCTION: LEGOLAND 0x00453d10
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
