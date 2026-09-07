/* LEGOLAND -- scope AG: controller teardown and the certificate print path.
 *
 * 0x00451740 reads an existing BMP (the caller passes "EGC.bmp") and prints
 * it to the first local printer, then stamps the message and timestamp with
 * the "Lego" face. render5.c already named it SaveScreenshotBmp.
 *
 * VC6 SP3 /O2 /Gy /Gd. Types describe only the fields used here.
 * Verification and recovered mechanics: docs/lanes/scope-ag.md.
 */

#pragma intrinsic(strlen)
extern unsigned int strlen(const char* s);

/* ---- types -------------------------------------------------------------- */

typedef struct PrinterInfo2 {
    char* pServerName;               /* +0x00 */
    char* pPrinterName;              /* +0x04 */
} PrinterInfo2;

#pragma pack(push, 2)
typedef struct BitmapFileHeader {
    unsigned short type;             /* +0x00 */
    unsigned long  size;             /* +0x02 */
    unsigned short res1;             /* +0x06 */
    unsigned short res2;             /* +0x08 */
    unsigned long  offBits;          /* +0x0a */
} BitmapFileHeader;                  /* 0x0e */
#pragma pack(pop)

typedef struct BitmapInfoHeader {
    unsigned long  biSize;           /* +0x00 */
    long           biWidth;          /* +0x04 */
    long           biHeight;         /* +0x08 */
    unsigned short biPlanes;         /* +0x0c */
    unsigned short biBitCount;       /* +0x0e */
    unsigned long  biCompression;    /* +0x10 */
    unsigned long  biSizeImage;      /* +0x14 */
    long           biXPelsPerMeter;  /* +0x18 */
    long           biYPelsPerMeter;  /* +0x1c */
    unsigned long  biClrUsed;        /* +0x20 */
    unsigned long  biClrImportant;   /* +0x24 */
} BitmapInfoHeader;                  /* 0x28 */

typedef struct DevMode {
    char           dmDeviceName[32]; /* +0x00 */
    unsigned short dmSpecVersion;    /* +0x20 */
    unsigned short dmDriverVersion;  /* +0x22 */
    unsigned short dmSize;           /* +0x24 */
    unsigned short dmDriverExtra;    /* +0x26 */
    unsigned long  dmFields;         /* +0x28 */
    short          dmOrientation;    /* +0x2c */
    char           rest[0x94 - 0x2e];
} DevMode;                           /* 0x94 */

typedef struct DocInfo {
    int         cbSize;              /* +0x00 */
    const char* lpszDocName;         /* +0x04 */
    const char* lpszOutput;          /* +0x08 */
    const char* lpszDatatype;        /* +0x0c */
    unsigned long fwType;            /* +0x10 */
} DocInfo;                           /* 0x14 */

typedef struct LogFont {
    long          lfHeight;          /* +0x00 */
    long          lfWidth;           /* +0x04 */
    long          lfEscapement;      /* +0x08 */
    long          lfOrientation;     /* +0x0c */
    long          lfWeight;          /* +0x10 */
    unsigned char lfItalic;          /* +0x14 */
    unsigned char lfUnderline;       /* +0x15 */
    unsigned char lfStrikeOut;       /* +0x16 */
    unsigned char lfCharSet;         /* +0x17 */
    unsigned char lfOutPrecision;    /* +0x18 */
    unsigned char lfClipPrecision;   /* +0x19 */
    unsigned char lfQuality;         /* +0x1a */
    unsigned char lfPitchAndFamily;  /* +0x1b */
    char          lfFaceName[32];    /* +0x1c */
} LogFont;                           /* 0x3c */

typedef struct BitmapInfo {
    BitmapInfoHeader header;
    unsigned long    colors[1];
} BitmapInfo;

/* ---- imports ------------------------------------------------------------ */

__declspec(dllimport) int   __stdcall EnumPrintersA(unsigned long flags, char* name, unsigned long level,
                                                    void* buf, unsigned long cb, unsigned long* needed,
                                                    unsigned long* returned);              /* [0x4ab344] */
__declspec(dllimport) void* __stdcall CreateDCA(const char* driver, const char* device,
                                                const char* port, DevMode* dm);            /* [0x4ab0a8] */
__declspec(dllimport) void* __stdcall GlobalAlloc(unsigned int flags, unsigned long bytes); /* [0x4ab1fc] */
__declspec(dllimport) void* __stdcall GlobalLock(void* h);                                 /* [0x4ab144] */
__declspec(dllimport) int   __stdcall GlobalUnlock(void* h);                               /* [0x4ab234] */
__declspec(dllimport) void* __stdcall GlobalFree(void* h);                                 /* [0x4ab14c] */
__declspec(dllimport) void* __stdcall LocalAlloc(unsigned int flags, unsigned int bytes);  /* [0x4ab13c] */
__declspec(dllimport) void* __stdcall LocalFree(void* h);                                  /* [0x4ab244] */
__declspec(dllimport) int   __stdcall DeleteDC(void* dc);                                  /* [0x4ab0a4] */
__declspec(dllimport) void* __stdcall CreateCompatibleDC(void* dc);                        /* [0x4ab094] */
__declspec(dllimport) void* __stdcall SelectObject(void* dc, void* obj);                   /* [0x4ab080] */
__declspec(dllimport) int   __stdcall DeleteObject(void* obj);                             /* [0x4ab09c] */
__declspec(dllimport) int   __stdcall GetDeviceCaps(void* dc, int index);                  /* [0x4ab098] */
__declspec(dllimport) void* __stdcall CreateDIBitmap(void* dc, BitmapInfoHeader* bmih,
                                                     unsigned long init, const void* bits,
                                                     BitmapInfo* bmi, unsigned int usage); /* [0x4ab0ac] */
__declspec(dllimport) int   __stdcall StartDocA(void* dc, DocInfo* di);                    /* [0x4ab0a0] */
__declspec(dllimport) int   __stdcall StartPage(void* dc);                                 /* [0x4ab090] */
__declspec(dllimport) int   __stdcall EndPage(void* dc);                                   /* [0x4ab084] */
__declspec(dllimport) int   __stdcall EndDoc(void* dc);                                    /* [0x4ab08c] */
__declspec(dllimport) int   __stdcall StretchDIBits(void* dc, int xDest, int yDest, int destW, int destH,
                                                    int xSrc, int ySrc, int srcW, int srcH,
                                                    const void* bits, BitmapInfo* bmi,
                                                    unsigned int usage, unsigned long rop); /* [0x4ab088] */
__declspec(dllimport) int   __stdcall SetBkMode(void* dc, int mode);                       /* [0x4ab074] */
__declspec(dllimport) unsigned int __stdcall SetTextAlign(void* dc, unsigned int mode);    /* [0x4ab07c] */
__declspec(dllimport) void* __stdcall CreateFontIndirectA(const LogFont* lf);              /* [0x4ab078] */
__declspec(dllimport) int   __stdcall TextOutA(void* dc, int x, int y, const char* s, int n); /* [0x4ab06c] */
__declspec(dllimport) char* __stdcall lstrcpyA(char* dst, const char* src);                /* [0x4ab240] */
__declspec(dllimport) int   __stdcall MulDiv(int n, int num, int den);                     /* [0x4ab140] */

extern void* memset(void* p, int c, unsigned int n);
extern int   _open(const char* path, int oflag, ...);           /* 0x0049f6c0 */
extern int   _read(int fd, void* buf, unsigned int n);          /* 0x0049f4ca */
extern int   _close(int fd);                                    /* 0x0049f417 */

extern const char g_font_face[];                                /* 0x004b86e0 "Lego" */

extern void* g_controller;           /* 0x00813b00  CONTROLLERBUFFER (input2.c) */
extern int   g_controllers_ready;    /* 0x00667104 */
extern void  HeapFree_w(void* p);    /* 0x0049e4d0 */

/* Free the controller record allocated by SetupControllers. Does not null
 * g_controller — only clears the ready flag. */
// FUNCTION: LEGOLAND 0x00451f40
void KillControllers(void)
{
    if (g_controllers_ready) {
        HeapFree_w(g_controller);
        g_controllers_ready = 0;
    }
}

/* Print `path` (an existing BMP) to the first local printer and stamp
 * `msg` / `stamp` in the Lego face. Non-zero on success.
 *
 * Residual: frame 0xb88 and the addressed run match (returned/pBits/needed
 * at 0x28/0x2c/0x30, DEVMODE 0x84, printers 0x118) once those three live in
 * one 16-byte struct with memdc. StretchDIBits still precomputes the signed
 * pageW/8 and pageH/64 margins instead of splitting the /64 across the
 * stdcall pushes (orig starts cdq/and while pageH is in eax, finishes
 * sar ecx,6 after the src pushes). Inlining the expressions dropped the
 * score (96.5 → 94.6). */
// WIP-FUNCTION: LEGOLAND 0x00451740  (96.5%, StretchDIBits margin schedule; 22 residual)
int SaveScreenshotBmp(const char* path, char* msg, const char* stamp)
{
    struct { unsigned long returned; void* pBits; unsigned long needed; void* memdc; } rpn;
    int               fd;
    void*             hdc;
    void*             hDib;
    void*             pBmi;
    void*             hBits;
    void*             hbmp;
    void*             oldbmp;
    void*             font1;
    void*             font2;
    void*             oldfont;
    LogFont*          lf;
    int               pageW;
    int               pageH;
    int               xDest;
    int               yDest;
    int               destW;
    int               destH;
    int               nColors;
    BitmapInfoHeader  bih;
    BitmapFileHeader  bfh;
    DocInfo           di;
    DevMode           dm;
    char              printers[0xA80];

    rpn.needed = 0;
    rpn.returned = 0;
    if (EnumPrintersA(1, 0, 2, printers, 0x540, &rpn.needed, &rpn.returned) <= 0 || rpn.returned <= 0)
        return 0;

    memset(&dm, 0, sizeof(dm));
    dm.dmSize = 0x94;
    dm.dmFields = 1;
    dm.dmOrientation = 2;
    hdc = CreateDCA(0, ((PrinterInfo2*)printers)->pPrinterName, 0, &dm);
    if (hdc == 0)
        return 0;

    fd = _open(path, 0x8000, 0x100);
    if (fd < 0) {
        DeleteDC(hdc);
        return 0;
    }

    _read(fd, &bfh, 0xe);
    _read(fd, &bih, 0x28);
    if (bih.biBitCount > 8)
        nColors = 0;
    else
        nColors = 1 << bih.biBitCount;

    hDib = GlobalAlloc(0x42, nColors * 4 + 0x28);
    if (hDib == 0) {
        _close(fd);
        DeleteDC(hdc);
        return 0;
    }
    pBmi = GlobalLock(hDib);
    if (pBmi == 0) {
        GlobalFree(hDib);
        _close(fd);
        DeleteDC(hdc);
        return 0;
    }

    ((BitmapInfoHeader*)pBmi)->biSize = bih.biSize;
    ((BitmapInfoHeader*)pBmi)->biWidth = bih.biWidth;
    ((BitmapInfoHeader*)pBmi)->biHeight = bih.biHeight;
    ((BitmapInfoHeader*)pBmi)->biPlanes = bih.biPlanes;
    ((BitmapInfoHeader*)pBmi)->biBitCount = bih.biBitCount;
    ((BitmapInfoHeader*)pBmi)->biCompression = bih.biCompression;
    ((BitmapInfoHeader*)pBmi)->biSizeImage = bih.biSizeImage;
    ((BitmapInfoHeader*)pBmi)->biXPelsPerMeter = bih.biXPelsPerMeter;
    ((BitmapInfoHeader*)pBmi)->biYPelsPerMeter = bih.biYPelsPerMeter;
    ((BitmapInfoHeader*)pBmi)->biClrUsed = bih.biClrUsed;
    ((BitmapInfoHeader*)pBmi)->biClrImportant = bih.biClrImportant;
    if (((BitmapInfoHeader*)pBmi)->biBitCount < 9)
        _read(fd, (char*)pBmi + 0x28, (1 << (unsigned char)bih.biBitCount) * 4);

    hBits = GlobalAlloc(0x42, bfh.size - bfh.offBits);
    if (hBits == 0) {
        GlobalUnlock(hDib);
        GlobalFree(hDib);
        _close(fd);
        DeleteDC(hdc);
        return 0;
    }
    rpn.pBits = GlobalLock(hBits);
    if (rpn.pBits == 0) {
        GlobalUnlock(hDib);
        GlobalFree(hDib);
        GlobalFree(hBits);
        _close(fd);
        DeleteDC(hdc);
        return 0;
    }

    _read(fd, rpn.pBits, bfh.size - bfh.offBits);
    hbmp = CreateDIBitmap(hdc, &bih, 4, rpn.pBits, (BitmapInfo*)pBmi, 0);
    if (hbmp == 0) {
        GlobalUnlock(hDib);
        GlobalUnlock(hBits);
        GlobalFree(hDib);
        GlobalFree(hBits);
        DeleteObject(hbmp);
        DeleteDC(hdc);
        return 0;
    }

    GlobalUnlock(hDib);
    GlobalUnlock(hBits);
    _close(fd);

    if (!(GetDeviceCaps(hdc, 0x26) & 1)) {
        GlobalFree(hDib);
        GlobalFree(hBits);
        DeleteObject(hbmp);
        DeleteDC(hdc);
        return 0;
    }

    di.cbSize = 0x14;
    di.lpszOutput = 0;
    di.lpszDatatype = 0;
    di.fwType = 0;
    di.lpszDocName = "Lego certificate";
    if (StartDocA(hdc, &di) == -1) {
        GlobalFree(hDib);
        GlobalFree(hBits);
        DeleteObject(hbmp);
        DeleteDC(hdc);
        return 0;
    }
    if (StartPage(hdc) <= 0) {
        GlobalFree(hDib);
        GlobalFree(hBits);
        DeleteObject(hbmp);
        EndDoc(hdc);
        DeleteDC(hdc);
        return 0;
    }

    rpn.memdc = CreateCompatibleDC(hdc);
    if (rpn.memdc == 0) {
        GlobalFree(hDib);
        GlobalFree(hBits);
        DeleteObject(hbmp);
        EndPage(hdc);
        EndDoc(hdc);
        DeleteDC(hdc);
        return 0;
    }
    oldbmp = SelectObject(rpn.memdc, hbmp);
    if (oldbmp == 0) {
        GlobalFree(hDib);
        GlobalFree(hBits);
        DeleteObject(hbmp);
        EndPage(hdc);
        EndDoc(hdc);
        DeleteDC(hdc);
        return 0;
    }

    pageW = GetDeviceCaps(hdc, 8);
    pageH = GetDeviceCaps(hdc, 0xa);
    xDest = pageW / 8;
    yDest = pageH / 64;
    destW = pageW - 2 * xDest;
    destH = pageH - 2 * yDest;
    if (StretchDIBits(hdc, xDest, yDest, destW, destH,
                      0, 0, bih.biWidth, bih.biHeight,
                      rpn.pBits, (BitmapInfo*)pBmi, 0, 0xcc0020) == 0) {
        GlobalFree(hDib);
        GlobalFree(hBits);
        DeleteObject(hbmp);
        EndPage(hdc);
        EndDoc(hdc);
        DeleteDC(hdc);
        return 0;
    }

    lf = (LogFont*)LocalAlloc(0x40, 0x3c);
    lf->lfHeight = -MulDiv(0x14, GetDeviceCaps(hdc, 0x5a), 0x48);
    lf->lfWeight = 0x190;
    lstrcpyA(lf->lfFaceName, g_font_face);
    font1 = CreateFontIndirectA(lf);
    if (font1 == 0) {
        GlobalFree(hDib);
        GlobalFree(hBits);
        DeleteObject(hbmp);
        DeleteDC(hdc);
        LocalFree(lf);
        return 0;
    }

    SetBkMode(hdc, 1);
    oldfont = SelectObject(hdc, font1);
    SetBkMode(hdc, 1);
    SetTextAlign(hdc, 6);
    TextOutA(hdc, destW / 2, pageH * 678 / ((BitmapInfoHeader*)pBmi)->biHeight, msg, (int)strlen(msg));

    lf->lfHeight = -MulDiv(8, GetDeviceCaps(hdc, 0x5a), 0x48);
    lf->lfWeight = 0x12c;
    lstrcpyA(lf->lfFaceName, g_font_face);
    font2 = CreateFontIndirectA(lf);
    if (font2 == 0) {
        GlobalFree(hDib);
        GlobalFree(hBits);
        DeleteObject(hbmp);
        DeleteDC(hdc);
        LocalFree(lf);
        return 0;
    }

    SetBkMode(hdc, 1);
    SelectObject(hdc, font2);
    SetBkMode(hdc, 1);
    SetTextAlign(hdc, 0);
    TextOutA(hdc, 0, 0, stamp, (int)strlen(stamp));

    LocalFree(lf);
    SelectObject(hdc, oldfont);
    DeleteObject(font2);
    DeleteDC(rpn.memdc);
    if (EndPage(hdc) <= 0) {
        GlobalFree(hDib);
        GlobalFree(hBits);
        DeleteObject(hbmp);
        EndDoc(hdc);
        DeleteDC(hdc);
        return 0;
    }
    EndDoc(hdc);
    DeleteDC(hdc);
    GlobalFree(hDib);
    GlobalFree(hBits);
    DeleteObject(hbmp);
    return 1;
}
