/* LEGOLAND -- scope U: the exception-report writer (exceptlog.txt).
 *
 * WinMain (0x00453d10) wraps GameMain in __try/__except and its handler
 * calls WriteExceptionReport with the exception pointers and "main thread".
 * The report goes to exceptlog.txt beside the exe: the exception's name and
 * the faulting module, the machine (time, exe version, user, computer,
 * processors, memory), the access-violation address, the register dump,
 * sixteen code bytes at CS:EIP, up to 0x800 stack dwords eight per line,
 * and every committed module with its size, link stamp and file date. The
 * byte and stack dumps run under their own __try so a bad pointer in the
 * crashed context cannot lose the report.
 *
 * VC6 SP3 /O2 /Gy /Gd. The Win32 types are declared locally with only the
 * fields used; the imports are the IAT slots in the trailing comments.
 * Names are ours (the integrator's provisional readings in
 * docs/SCOPE_U_exception_report_objdesc.md). Verification and notes:
 * docs/lanes/scope-u.md.
 */

/* ---- Win32 types (only what is used) ------------------------------------- */
typedef struct FILETIME { unsigned long lo, hi; } FILETIME;

typedef struct SYSTEM_INFO {
    unsigned long  dwOemId;                  /* +0x00 */
    unsigned long  dwPageSize;               /* +0x04 */
    void*          lpMinimumApplicationAddress; /* +0x08 */
    void*          lpMaximumApplicationAddress; /* +0x0c */
    unsigned long  dwActiveProcessorMask;    /* +0x10 */
    unsigned long  dwNumberOfProcessors;     /* +0x14 */
    unsigned long  dwProcessorType;          /* +0x18 */
    unsigned long  dwAllocationGranularity;  /* +0x1c */
    unsigned short wProcessorLevel;          /* +0x20 */
    unsigned short wProcessorRevision;       /* +0x22 */
} SYSTEM_INFO;                               /* 0x24 */

typedef struct MEMORYSTATUS {
    unsigned long dwLength;                  /* +0x00 */
    unsigned long dwMemoryLoad;              /* +0x04 */
    unsigned long dwTotalPhys;               /* +0x08 */
    unsigned long dwAvailPhys;               /* +0x0c */
    unsigned long dwTotalPageFile;           /* +0x10 */
    unsigned long dwAvailPageFile;           /* +0x14 */
    unsigned long dwTotalVirtual;            /* +0x18 */
    unsigned long dwAvailVirtual;            /* +0x1c */
} MEMORYSTATUS;                              /* 0x20 */

typedef struct MEMORY_BASIC_INFORMATION {
    void*         BaseAddress;               /* +0x00 */
    void*         AllocationBase;            /* +0x04 */
    unsigned long AllocationProtect;         /* +0x08 */
    unsigned long RegionSize;                /* +0x0c */
    unsigned long State;                     /* +0x10 */
    unsigned long Protect;                   /* +0x14 */
    unsigned long Type;                      /* +0x18 */
} MEMORY_BASIC_INFORMATION;                  /* 0x1c */

typedef struct IMAGE_DOS_HEADER {
    unsigned short e_magic;                  /* +0x00  'MZ' */
    char           pad02[0x3a];              /* +0x02 */
    int            e_lfanew;                 /* +0x3c */
} IMAGE_DOS_HEADER;

typedef struct IMAGE_NT_HEADERS {
    unsigned long  Signature;                /* +0x00  'PE\0\0' */
    unsigned short Machine;                  /* +0x04 */
    unsigned short NumberOfSections;         /* +0x06 */
    unsigned long  TimeDateStamp;            /* +0x08 */
} IMAGE_NT_HEADERS;

typedef struct EXCEPTION_RECORD {
    unsigned long ExceptionCode;             /* +0x00 */
    unsigned long ExceptionFlags;            /* +0x04 */
    struct EXCEPTION_RECORD* ExceptionRecord; /* +0x08 */
    void*         ExceptionAddress;          /* +0x0c */
    unsigned long NumberParameters;          /* +0x10 */
    unsigned long ExceptionInformation[15];  /* +0x14 */
} EXCEPTION_RECORD;

typedef struct CONTEXT {
    unsigned long ContextFlags;              /* +0x00 */
    unsigned long Dr[6];                     /* +0x04 */
    unsigned char FloatSave[0x70];           /* +0x1c */
    unsigned long SegGs, SegFs, SegEs, SegDs;            /* +0x8c +0x90 +0x94 +0x98 */
    unsigned long Edi, Esi, Ebx, Edx, Ecx, Eax;          /* +0x9c +0xa0 +0xa4 +0xa8 +0xac +0xb0 */
    unsigned long Ebp, Eip, SegCs, EFlags, Esp, SegSs;   /* +0xb4 +0xb8 +0xbc +0xc0 +0xc4 +0xc8 */
} CONTEXT;

typedef struct EXCEPTION_POINTERS {
    EXCEPTION_RECORD* ExceptionRecord;       /* +0x00 */
    CONTEXT*          ContextRecord;         /* +0x04 */
} EXCEPTION_POINTERS;

/* ---- imports (IAT slots) ------------------------------------------------- */
__declspec(dllimport) unsigned long __stdcall GetModuleFileNameA(void* module, char* buf, unsigned long size); /* [0x4ab1ec] */
__declspec(dllimport) void* __stdcall CreateFileA(const char* name, unsigned long access, unsigned long share,
                                                  void* sec, unsigned long disp, unsigned long attrs, void* tmpl); /* [0x4ab258] */
__declspec(dllimport) void  __stdcall OutputDebugStringA(const char* s);                       /* [0x4ab1f0] */
__declspec(dllimport) unsigned long __stdcall VirtualQuery(const void* p, MEMORY_BASIC_INFORMATION* mbi, unsigned long n); /* [0x4ab1f4] */
__declspec(dllimport) int   __stdcall wvsprintfA(char* buf, const char* fmt, char* args);      /* [0x4ab2a8] */
__declspec(dllimport) int   __stdcall lstrlenA(const char* s);                                  /* [0x4ab1e8] */
__declspec(dllimport) char* __stdcall lstrcpyA(char* dst, const char* src);                     /* [0x4ab240] */
__declspec(dllimport) int   __stdcall WriteFile(void* f, const void* buf, unsigned long n, unsigned long* written, void* ov); /* [0x4ab254] */
__declspec(dllimport) int   __stdcall CloseHandle(void* h);                                     /* [0x4ab260] */
__declspec(dllimport) void  __stdcall GetSystemInfo(SYSTEM_INFO* si);                           /* [0x4ab1a8] */
__declspec(dllimport) unsigned long __stdcall GetFileSize(void* f, unsigned long* hi);          /* [0x4ab25c] */
__declspec(dllimport) int   __stdcall GetFileTime(void* f, FILETIME* c, FILETIME* a, FILETIME* w); /* [0x4ab1dc] */
__declspec(dllimport) int   __stdcall FileTimeToLocalFileTime(const FILETIME* ft, FILETIME* local); /* [0x4ab12c] */
__declspec(dllimport) int   __stdcall FileTimeToDosDateTime(const FILETIME* ft, unsigned short* date, unsigned short* time); /* [0x4ab1d0] */
__declspec(dllimport) void  __stdcall GetSystemTimeAsFileTime(FILETIME* ft);                    /* [0x4ab120] */
__declspec(dllimport) int   __stdcall GetUserNameA(char* buf, unsigned long* size);             /* [0x4ab000] */
__declspec(dllimport) int   __stdcall GetComputerNameA(char* buf, unsigned long* size);         /* [0x4ab130] */
__declspec(dllimport) void  __stdcall GlobalMemoryStatus(MEMORYSTATUS* ms);                     /* [0x4ab134] */
extern int __declspec(dllimport) __cdecl wsprintfA(char* buf, const char* fmt, ...);            /* [0x4ab298] */

/* ---- CRT ----------------------------------------------------------------- */
extern char* strrchr(const char* s, int c);                                                     /* 0x004a0010 (CRT) */

/* ---- globals ------------------------------------------------------------- */
extern int  g_report_written;        /* 0x00667528  one report per process (first named here) */
extern char g_exe_version[];         /* 0x0066752c  ProductVersion, filled by ReadExeVersionString (gameframe.c) */
extern int  g_report_code_bytes;     /* 0x004b8a88  = 16   bytes dumped at CS:EIP */
extern int  g_report_stack_dwords;   /* 0x004b8a8c  = 0x800 stack dwords dumped at most */
extern int  g_report_per_line;       /* 0x004b8a90  = 8    stack dwords per line */

/* ---- this file ----------------------------------------------------------- */
void        ReportWrite(void* f, const char* fmt, ...);
void        ReportModuleLine(void* f);
void        ReportModuleDetails(void* f, char* base);
void        FormatFileTime(char* out, FILETIME ft);
void        ReportSystemInfo(void* f);
const char* ExceptionCodeName(unsigned int code);
char*       PathFileName(char* path);

/* ========================================================================= */

/* wvsprintf the line and write it to the report file. */
// FUNCTION: LEGOLAND 0x00454290
void ReportWrite(void* f, const char* fmt, ...)
{
    unsigned long written;
    char          buf[2000];

    wvsprintfA(buf, fmt, (char*)(&fmt + 1));
    WriteFile(f, buf, lstrlenA(buf), &written, 0);
}

/* The module list: walk the address space a page at a time with
 * VirtualQuery and report each committed allocation base once. */
// FUNCTION: LEGOLAND 0x004542e0
void ReportModuleLine(void* f)
{
    MEMORY_BASIC_INFORMATION mbi;
    SYSTEM_INFO   si;
    char*         last = 0;
    unsigned int  page = 0;
    unsigned int  pagesize;
    unsigned int  npages;

    ReportWrite(f, "\r\n\tModule list: names, addresses, sizes, time stamps and file times:\r\n");
    GetSystemInfo(&si);
    pagesize = si.dwPageSize;
    npages = (0x40000000 / pagesize) * 4;
    while (page < npages) {
        if (VirtualQuery((void*)(pagesize * page), &mbi, sizeof(mbi)) && mbi.RegionSize > 0) {
            page += mbi.RegionSize / pagesize;
            if (mbi.State == 0x1000 && (char*)mbi.AllocationBase > last) {
                last = (char*)mbi.AllocationBase;
                ReportModuleDetails(f, last);
            }
        } else {
            page += 0x10000 / pagesize;
        }
    }
}

/* One module line: file name, base, file size, PE time stamp and the file's
 * write time. Guarded by __try because the base may not be a PE image.
 *
 * Gate note: 110 of 110 instructions and 380 of 380 bytes agree; the 3
 * residual "mismatches" are the SEH prologue/epilogue's `fs:[0]` accesses,
 * which our object encodes as a relocation against the absolute CRT symbol
 * __except_list (value 0) and match.py patches to its sentinel, while the
 * linked original carries the resolved 0. Nothing in the C can change that;
 * the marker stays WIP until the gate normalises `fs:[<abs>]` to `fs:[0]`.
 * Block-scope local names are load-bearing (frame slot order follows the
 * symbol-hash bucket): `pe` sits after `h`, `dos` before `date`. */
// WIP-FUNCTION: LEGOLAND 0x00454380  (100% by instruction and byte count; 3 residual = the relocated fs:[__except_list] displacement the gate's normaliser reads as fs:[<abs>] against the original's fs:[0])
void ReportModuleDetails(void* f, char* base)
{
    __try {
        char path[0x104];

        if (GetModuleFileNameA(base, path, 0x104) > 0) {
            IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER*)base;
            char              date[100] = "";
            unsigned long     size = 0;
            FILETIME          ft;
            void*             h;
            IMAGE_NT_HEADERS* pe;

            if (dos->e_magic == 0x5a4d) {
                pe = (IMAGE_NT_HEADERS*)((char*)dos + dos->e_lfanew);
                if (pe->Signature == 0x4550) {
                    h = CreateFileA(path, 0x80000000, 1, 0, 3, 0x80, 0);
                    if (h != (void*)-1) {
                        size = GetFileSize(h, 0);
                        if (GetFileTime(h, 0, 0, &ft)) {
                            wsprintfA(date, " - file date is ");
                            FormatFileTime(date + lstrlenA(date), ft);
                        }
                        CloseHandle(h);
                    }
                    ReportWrite(f, "%s, loaded at 0x%08x - %d bytes - %08x%s\r\n",
                                path, base, size, pe->TimeDateStamp, date);
                }
            }
        }
    } __except (1) {
    }
}

/* "m/d/yyyy hh:mm:ss" of a FILETIME in local time, or "" when it cannot be
 * converted. The FILETIME is passed by value and converted in place. */
// FUNCTION: LEGOLAND 0x00454500
void FormatFileTime(char* out, FILETIME ft)
{
    unsigned short date, time;

    if (FileTimeToLocalFileTime(&ft, &ft) && FileTimeToDosDateTime(&ft, &date, &time))
        wsprintfA(out, "%d/%d/%d %02d:%02d:%02d",
                  (date >> 5) & 0xf, date & 0x1f, (date >> 9) + 1980,
                  time >> 11, (time >> 5) & 0x3f, (time & 0x1f) * 2);
    else
        out[0] = 0;
}

/* Time, exe name and version, user and machine, processors, memory. */
// FUNCTION: LEGOLAND 0x004545a0
void ReportSystemInfo(void* f)
{
    unsigned long compsize;
    unsigned long usersize;
    FILETIME      now;
    MEMORYSTATUS  ms;
    SYSTEM_INFO   si;
    char          timebuf[100];
    char          computer[200];
    char          user[200];
    char          module[0x104];

    GetSystemTimeAsFileTime(&now);
    FormatFileTime(timebuf, now);
    ReportWrite(f, "Error occurred at %s.\r\n", timebuf);
    if (GetModuleFileNameA(0, module, 0x104) <= 0)
        lstrcpyA(module, "Unknown");
    usersize = 200;
    if (!GetUserNameA(user, &usersize))
        lstrcpyA(user, "Unknown");
    compsize = 200;
    if (!GetComputerNameA(computer, &compsize))
        lstrcpyA(computer, "Unknown");
    ReportWrite(f, "%s (Version %s)\r\n", module, g_exe_version);
    ReportWrite(f, "Run by %s on machine %s.\r\n", user, computer);
    GetSystemInfo(&si);
    ReportWrite(f, "%d processor(s), type %d.\r\n", si.dwNumberOfProcessors, si.dwProcessorType);
    ms.dwLength = sizeof(ms);
    GlobalMemoryStatus(&ms);
    ReportWrite(f, "%d MBytes physical memory.\r\n", (ms.dwTotalPhys + 0xfffff) >> 20);
}

/* The exception code's name, from a table built on the stack. */
// FUNCTION: LEGOLAND 0x00454700
const char* ExceptionCodeName(unsigned int code)
{
    struct { unsigned int code; const char* name; } tab[24] = {
        { 0x40010005, "a Control-C" },
        { 0x40010008, "a Control-Break" },
        { 0x80000002, "a Datatype Misalignment" },
        { 0x80000003, "a Breakpoint" },
        { 0xc0000005, "an Access Violation" },
        { 0xc0000006, "an In Page Error" },
        { 0xc0000017, "a No Memory" },
        { 0xc000001d, "an Illegal Instruction" },
        { 0xc0000025, "a Noncontinuable Exception" },
        { 0xc0000026, "an Invalid Disposition" },
        { 0xc000008c, "a Array Bounds Exceeded" },
        { 0xc000008d, "a Float Denormal Operand" },
        { 0xc000008e, "a Float Divide by Zero" },
        { 0xc000008f, "a Float Inexact Result" },
        { 0xc0000090, "a Float Invalid Operation" },
        { 0xc0000091, "a Float Overflow" },
        { 0xc0000092, "a Float Stack Check" },
        { 0xc0000093, "a Float Underflow" },
        { 0xc0000094, "an Integer Divide by Zero" },
        { 0xc0000095, "an Integer Overflow" },
        { 0xc0000096, "a Privileged Instruction" },
        { 0xc00000fd, "a Stack Overflow" },
        { 0xc0000142, "a DLL Initialization Failed" },
        { 0xe06d7363, "a Microsoft C++ Exception" },
    };
    unsigned int i;

    for (i = 0; i < 24; i++)
        if (code == tab[i].code)
            return tab[i].name;
    return "Unknown exception type";
}

/* The file-name part of a path (after the last backslash). */
// FUNCTION: LEGOLAND 0x004548f0
char* PathFileName(char* path)
{
    char* p = strrchr(path, '\\');

    if (p)
        return p + 1;
    return path;
}

/* Write exceptlog.txt for the exception; once per process. `where` names
 * the thread ("main thread"). Returns 0 always.
 *
 * Local NAMES are load-bearing here: an EBP/SEH frame lays its locals out
 * by VC6's symbol-hash bucket (0..15 from ebp down, same bucket most
 * recently declared first), so each name below was chosen for the bucket
 * the original's slot order needs -- see docs/lanes/scope-u.md. The
 * declaration order also carries the initialiser order (progname, culprit,
 * column, textline, leeway) and the within-bucket ties.
 *
 * Gate note: 345 of 345 instructions and 1256 of 1256 bytes agree; the 4
 * residual "mismatches" are the SEH frame's `fs:[0]` accesses (prologue,
 * both epilogues), encoded in our object as a relocation against the
 * absolute CRT symbol __except_list (value 0) that match.py patches to its
 * sentinel, where the linked original carries the resolved 0. */
// WIP-FUNCTION: LEGOLAND 0x00453da0  (100% by instruction and byte count; 4 residual = the relocated fs:[__except_list] displacement the gate's normaliser reads as fs:[<abs>] against the original's fs:[0])
int WriteExceptionReport(EXCEPTION_POINTERS* ep, const char* where)
{
    void*       report;                     /* the log file           bucket 3 */
    char        path[0x104];                /* exe path, then the log's   3 */
    char        progname[0x104] = "Unknown"; /* exe name without extension 3 */
    const char* culprit = "Unknown";        /* faulting module's file name 1 */
    int         column = 0;                 /* stack dwords on this line  5 */
    char        textline[1000] = "";        /* the stack line being built 4 */
    char*       end;                        /* flush point in textline    4 */
    int         leeway = 50;                /* bytes kept free at its end 9 */
    char*       lpos;                       /* write cursor in textline   9 */
    MEMORY_BASIC_INFORMATION memq;          /* VirtualQuery of EIP        9 */
    CONTEXT*    ctx;                        /*                            0 */
    char*       cut;                        /* the '.' cut off progname   0 */
    char*       fname;                      /* file part of path          0 */
    const char* eol;                        /* " " or "\r\n" after a dword 0 */
    EXCEPTION_RECORD* ex;                   /*                            2 */
    unsigned char* pc;                      /* bytes at EIP              10 */
    int         k;                          /* byte index                11 */
    char        blamefile[0x104];           /* faulting module's path    11 */
    char        violation[1000];            /* the access-violation line 12 */
    unsigned long* dwords;                  /* stack cursor              13 */
    const char* dir;                        /* "Read from" / "Write to"  13 */
    unsigned long* stackend;                /* dump limit                14 */
    unsigned long* nxt;                     /* dwords + 1                15 */

    if (g_report_written)
        return 0;
    g_report_written = 1;
    if (GetModuleFileNameA(0, path, 0x104) <= 0)
        path[0] = 0;
    fname = PathFileName(path);
    lstrcpyA(progname, fname);
    cut = strrchr(progname, '.');
    if (cut)
        *cut = 0;
    lstrcpyA(fname, "exceptlog.txt");
    report = CreateFileA(path, 0x40000000, 0, 0, 4, 0x80000080, 0);
    if (report == (void*)-1) {
        OutputDebugStringA("Error creating exception report");
        return 0;
    }
    ex = ep->ExceptionRecord;
    ctx = ep->ContextRecord;
    if (VirtualQuery((void*)ctx->Eip, &memq, sizeof(memq)) &&
        GetModuleFileNameA(memq.AllocationBase, blamefile, 0x104) > 0)
        culprit = PathFileName(blamefile);
    ReportWrite(report, "%s caused %s in module %s at %04x:%08x.\r\n",
                progname, ExceptionCodeName(ex->ExceptionCode), culprit, ctx->SegCs, ctx->Eip);
    ReportWrite(report, "Exception handler called in %s.\r\n", where);
    ReportSystemInfo(report);
    if (ex->ExceptionCode == 0xc0000005 && ex->NumberParameters >= 2) {
        dir = "Read from";
        if (ex->ExceptionInformation[0])
            dir = "Write to";
        wsprintfA(violation, "%s location %08x caused an access violation.\r\n", dir, ex->ExceptionInformation[1]);
        ReportWrite(report, "%s", violation);
    }
    ReportWrite(report, "\r\n");
    ReportWrite(report, "Registers:\r\n");
    ReportWrite(report, "EAX=%08x CS=%04x EIP=%08x EFLGS=%08x\r\n", ctx->Eax, ctx->SegCs, ctx->Eip, ctx->EFlags);
    ReportWrite(report, "EBX=%08x SS=%04x ESP=%08x EBP=%08x\r\n", ctx->Ebx, ctx->SegSs, ctx->Esp, ctx->Ebp);
    ReportWrite(report, "ECX=%08x DS=%04x ESI=%08x FS=%04x\r\n", ctx->Ecx, ctx->SegDs, ctx->Esi, ctx->SegFs);
    ReportWrite(report, "EDX=%08x ES=%04x EDI=%08x GS=%04x\r\n", ctx->Edx, ctx->SegEs, ctx->Edi, ctx->SegGs);
    ReportWrite(report, "Bytes at CS:EIP:\r\n");
    pc = (unsigned char*)ctx->Eip;
    for (k = 0; k < g_report_code_bytes; k++) {
        __try {
            ReportWrite(report, "%02x ", pc[k]);
        } __except (1) {
            ReportWrite(report, "?? ");
        }
    }
    ReportWrite(report, "\r\nStack dump:\r\n");
    __try {
        dwords = (unsigned long*)ctx->Esp;
        __asm mov eax, dword ptr fs:[4]     /* NT_TIB.StackBase */
        __asm mov stackend, eax
        if (stackend > dwords + g_report_stack_dwords)
            stackend = dwords + g_report_stack_dwords;
        end = textline + sizeof(textline) - leeway;
        lpos = textline;
        while ((nxt = dwords + 1) <= stackend) {
            if (column % g_report_per_line == 0)
                lpos += wsprintfA(lpos, "%08x: ", dwords);
            eol = " ";
            column++;
            if (column % g_report_per_line == 0 || dwords + 2 > stackend)
                eol = "\r\n";
            lpos += wsprintfA(lpos, "%08x%s", *dwords, eol);
            dwords = nxt;
            if (lpos > end) {
                ReportWrite(report, "%s", textline);
                textline[0] = 0;
                lpos = textline;
            }
        }
        ReportWrite(report, "%s", textline);
    } __except (1) {
        ReportWrite(report, "Exception encountered during stack dump.\r\n");
    }
    ReportModuleLine(report);
    CloseHandle(report);
    return 0;
}
