/* LEGOLAND -- the movie player's second tier: the doubled DIB blit, the
 * RES text-file and keyword-file readers, the narration ring drain, the
 * build-menu availability flag and the per-level reset.  These are the
 * callees scope K's movie.c declared and did not define.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).  Struct
 * field OFFSETS, global addresses and callee argument counts are load-bearing;
 * names are ours.
 *
 * ---------------------------------------------------------------------------
 * THE MOVIE BLIT.  RunMovie (movie.c) decodes every frame to a 16-bit DIB of
 * the movie's native size and hands it to BlitDIBToScreen with the video
 * surface locked.  The blit DOUBLES the frame -- each source pixel is written
 * to two columns of two rows -- so a 320x240 movie fills a 640x480 surface,
 * walks the DIB bottom-up (the last DIB row is the top screen row), converts
 * RGB555 to RGB565 on the fly when the surface is 565 (g_screen_depth == 2:
 * the green and red fields shift up one bit, the blue field stays), and
 * clears the rows above and below the picture to black, splitting the
 * vertical slack as `gap / 2` above and the remainder below.  Every cleared
 * row is 640 pixels wide regardless of the surface width.
 *
 * THE TEXT FILES.  LoadTextFileLines reads a RES file whole into a heap
 * buffer and cuts it into lines at '\r', writing a NUL over each '\r' and
 * skipping the '\n' after it; the caller keeps the line table and the ONE
 * buffer (tinystubs.c's FreeHelpTextBuffer/FreeReportHintBuffer free entry
 * 0).  LoadHintTextFor is movie.c's LoadHelpTextFor for the hint table
 * ("Intervals\\<key>", cap 0x20) and the two Set*TextPrefix helpers build
 * the "<file>_" narration prefixes uimisc3.c's PlayReportPage/PlayReportHint
 * format clip numbers onto.  ParseKeywordFile opens "Scripts\\<name>",
 * remembers the name and hands the file to the section parser at
 * 0x00478280 with the caller's {keyword, handler} table.
 *
 * THE NARRATION RING.  The speech decoder fills a 0x20000-byte ring
 * (g_narr_ring, read cursor g_narr_d, write cursor g_narr_e).
 * ReadDecodedNarration drains up to `len` bytes: if fewer are ready it asks
 * the decoder for more and clamps to what arrived, copies in contiguous runs
 * (the run ends at the ring's end or the write cursor), advances the read
 * cursor modulo 0x20000, and tops the ring up again before returning the
 * count actually copied.
 *
 * THE BUILD MENU.  An element's +0x08 flags: bit 0 = the class exists in this
 * level's database, bit 1 = available in the build menu, bit 2 = its object
 * class is loaded, bit 16 = "taken away".  EnsureObjectClassLoaded loads the
 * class on first use, sets bit 2 and marks the element unavailable
 * (0x00469ab0 clears bits 1 and 16); MarkElemAvailable then grants it --
 * unless it is already granted and not taken away, which is only logged --
 * unlocks its free-play table entry and, when asked, adds a "new object"
 * marker icon for it.  Either way the menu is flagged dirty.
 *
 * ORIGINAL BUG, reproduced: MarkElemAvailable's "Already got it" trace
 * formats the element's NAME pointer with %d.
 * ------------------------------------------------------------------------- */
#include "legoland.h"

#pragma intrinsic(memcpy, memset)
void* memcpy(void*, const void*, unsigned int);
void* memset(void*, int, unsigned int);

/* ---- types -------------------------------------------------------------- */

/* One level-database element (screencb7.c's Elem; legoland.h's Elem is a different record). */
typedef struct DBElem {
    const char* name;    /* +0x00 */
    void*       image;   /* +0x04 */
    int         flags;   /* +0x08 see the header */
    void*       data;    /* +0x0c the loaded object class */
} DBElem;

/* The level configuration block lpConfig points at; only the fields this
 * file touches (legoland.h's Map is the same object). */
typedef struct LevelCfg {
    unsigned short width;         /* +0x00 */
    unsigned short height;        /* +0x02 screen height in pixels */
    unsigned char  pad04[0x1a - 0x04];
    unsigned short visitor_cap;   /* +0x1a reset to 200 */
    unsigned char  pad1c[0x30 - 0x1c];
    int            f30;           /* +0x30 reset to 0 */
    int            f34;           /* +0x34 reset to 1 */
    int            f38;           /* +0x38 reset to 1 */
} LevelCfg;

/* Win32 BITMAPINFOHEADER (0x28 bytes); the pixels follow it. */
typedef struct BitmapInfoHeader {
    int   size;            /* +0x00 */
    int   width;           /* +0x04 */
    int   height;          /* +0x08 */
    short planes;          /* +0x0c */
    short bit_count;       /* +0x0e */
    int   compression;     /* +0x10 */
    int   size_image;      /* +0x14 */
    int   xpels;           /* +0x18 */
    int   ypels;           /* +0x1c */
    int   clr_used;        /* +0x20 */
    int   clr_important;   /* +0x24 */
} BitmapInfoHeader;

/* ---- globals ------------------------------------------------------------ */
extern LevelCfg*   g_level_cfg;              /* 0x004bcbf4 (lpConfig) */
extern void*       g_ddsd_bits;              /* 0x006680c0 locked surface lpSurface */
extern long        g_ddsd_pitch;             /* 0x006680ac locked surface lPitch */
extern int         g_screen_depth;           /* 0x00668088 2 = RGB565 */
/* First named here: the surface the last movie frame went to, toggled to 0
 * when the same surface comes round again (flip tracking for the caller). */
extern void*       g_last_blit_bits;         /* 0x006681ec */

extern char        g_rep_page_narration[];   /* 0x007cae80 "<file>_" */
extern char        g_rep_hint_narration[];   /* 0x007cb1e0 "<file>_" */
extern char*       g_rep_hints[];            /* 0x007cb140 the hint string table */
extern int         g_rep_hint_count;         /* 0x00798880 */
extern const char  kFmtPrefix[];             /* 0x004bf674 "%s_" */
extern const char  kFmtIntervals[];          /* 0x004bf678 "Intervals\\%s" */
extern const char  kFmtTwoStrings[];         /* 0x004b7a90 "%s%s" */
extern const char  kScriptsDir[];            /* 0x004bc084 "Scripts\\" */
extern const char  kAlreadyGotFmt[];         /* 0x004ba6e4 "Not giving %d.. Already got it\n" */
extern const char  kEmptyString[];           /* 0x004d8bb0 "" */
/* First named here: the 0x80-byte copy of the keyword file's name. */
extern char        g_keyword_file_name[];    /* 0x00668fd0 */

extern int         g_menu_dirty;             /* 0x0066871c */

extern unsigned char g_narr_ring[0x20000];   /* 0x007aaca0 the decoded-PCM ring (first named here) */
extern int         g_narr_d;                 /* 0x0079a834 ring read cursor */

/* The per-level state ResetLevelGlobals clears; unnamed ones are first
 * named here from what the reset does to them. */
extern unsigned short g_oc_word;             /* 0x008119ac */
extern int         g_oc_flag_a;              /* 0x008119a8 */
extern int         g_oc_flag_b;              /* 0x008119a0 */
extern int         g_level_number;           /* 0x00669054 (0x004785d0 stores it beside the level name) */
extern int         g_level_db_flag_ac;       /* 0x004bb5ac */
extern unsigned char g_level_byte_669050;    /* 0x00669050 */
extern void*       g_script_cur;             /* 0x0066879c */
extern int         g_level_int_669098;       /* 0x00669098 */
extern int         g_visitor_cap_extra;      /* 0x00832924 */
extern int         g_visitor_cap;            /* 0x00832920 copy of lpConfig+0x1a */
extern void*       g_script_root;            /* 0x007fdca4 */
extern int         g_level_db_active;        /* 0x004bb5b0 */
extern int         g_bricks_full;            /* 0x00832974 */
extern int         g_show_capacity;          /* 0x00832994 */
extern int         g_ride_wear;              /* 0x00832980 */
extern int         g_832ba8;                 /* 0x00832ba8 */

/* ---- callees ------------------------------------------------------------ */
extern void  _splitpath(const char* path, char* drive, char* dir,
                        char* fname, char* ext);                    /* 0x0049ec85 (CRT) */
extern int   sprintf(char* dst, const char* fmt, ...);              /* 0x0049e573 (CRT) */
extern char* strncpy(char* dst, const char* src, unsigned int n);   /* 0x004a0110 (CRT) */
extern void* HeapAlloc_w(unsigned int n);                           /* 0x0049e4ff (CRT malloc) */
extern void  HeapFree_w(void* p);                                   /* 0x0049e4d0 */
extern int   DBPrintf(const char* fmt, ...);                        /* 0x00453a20 */

extern void* RES_OpenFile(const char* path);                        /* 0x00489b60 */
extern int   RES_GetFileSize(void* f);                              /* 0x00489ce0 */
extern int   RES_ReadFile(void* f, void* buf, int len);             /* 0x00489cf0 */
extern void  RES_CloseFile(void* f);                                /* 0x00489de0 */
/* 0x00478280 (not exported): read the opened keyword file section by section
 * and dispatch each `[keyword]` through the table; first named here. */
extern int   ParseKeywordSections(void* f, const void* table, int count, int arg); /* 0x00478280 */

extern void  FreeReportHintBuffer(void);                            /* 0x00490880 */
extern int   LoadTextFileLines(const char* path, char** lines, int max); /* 0x00490680 */

extern DBElem* ElemID(const char* name);                              /* 0x0047b3f0 */
extern int   LoadObjectClass(DBElem* e);                              /* 0x00480b40 */
/* 0x00469ab0 (not exported): clear flags 2 and 0x10000 of an element that
 * exists (flag 1) and flag the menu dirty; first named here. */
extern void  MarkElemUnavailable(DBElem* e);                          /* 0x00469ab0 */
/* 0x0048a6e0 (not exported): find the element's entry in fpui's g_fp_table
 * by name (_stricmp) and unlock it; first named here. */
extern void  UnlockFreePlayEntry(DBElem* e);                          /* 0x0048a6e0 */
/* 0x00471c10 (not exported): load "NewObjIcons\\<class>.bmp" and append it
 * to the new-object marker table (tinystubs.c's ClearNewObjectMarkers is the
 * clear side); first named here. */
extern void  AddNewObjectMarker(void* objclass);                    /* 0x00471c10 */

/* The narration decoder's three queries (first named here): bytes between
 * the cursors, the contiguous run readable from the read cursor, and the
 * refill. */
extern int   NarrationBytesReady(void);                             /* 0x00498230 */
extern int   NarrationContiguous(void);                             /* 0x00498210 */
extern void  RefillNarrationRing(void);                             /* 0x00498250 */

/* ResetLevelGlobals' callees; the unnamed ones are first named here from
 * their bodies. */
extern void  FreeScriptStrings(void);                               /* 0x004689a0 frees g_script_strings[0..count) */
extern void* NewScriptEvent(void* a, void* b, void* c);             /* 0x004689f0 */
extern void  ClearReportState(void);                                /* 0x004441f0 g_report_state[0] = 0 */
extern void  ClearSim832b9c(void);                                  /* 0x0044db20 */
extern void  ClearAppraisalState(void);                             /* 0x0044db80 zeroes 0x00832978 and g_instant_appraisal */
extern void  ClearScriptStateBytes(void);                           /* 0x00468840 */
extern void  ScriptState_NoOp(void);                                /* 0x004688e0 */
extern void  SetLevelGoalState(int a, int b);                       /* 0x0044dc70 */
extern void  ClearScriptTexts(void);                                /* 0x00468830 both script text buffers' first byte */
extern void  ResetMoodAdjustments(void);                            /* 0x00482d70 */
extern void  ResetSimTuning(void);                                  /* 0x00462e90 the 0x00832824 block */
extern void  ClearMenuHelp(void);                                   /* 0x00476000 */
extern void  ClearIconHelp(void);                                   /* 0x00476050 SetIconHelp(i, 0) for i < 9 */
extern void  SetReportMovie(const char* name);                      /* 0x00490610 */
extern void  ResetDamageClock(void);                                /* 0x00463560 */
extern void  ResetEntranceTileTime(void);                           /* 0x00482b10 */
extern void  ResetBuildTimer(void);                                 /* 0x00459960 */
extern void  StopScript(int stop);                                  /* 0x0046b240 */


/* =========================================================================
 *  SetHelpTextPrefix / SetHintTextPrefix -- "<file>_" narration prefixes
 * ========================================================================= */

/* All three cdecl cleanups (splitpath 0x14, sprintf 0xc, the 0x40 frame)
 * merge into one `add esp,0x60`. */
// FUNCTION: LEGOLAND 0x00490740
void SetHelpTextPrefix(const char* key)
{
    char fname[0x40];

    _splitpath(key, 0, 0, fname, 0);
    sprintf(g_rep_page_narration, kFmtPrefix, fname);
}

/* The hint-narration twin (0x00490770); nothing in the tree declares it
 * because only LoadHintTextFor below calls it. */
// FUNCTION: LEGOLAND 0x00490770
void SetHintTextPrefix(const char* key)
{
    char fname[0x40];

    _splitpath(key, 0, 0, fname, 0);
    sprintf(g_rep_hint_narration, kFmtPrefix, fname);
}


/* =========================================================================
 *  LoadHintTextFor -- movie.c's LoadHelpTextFor for the hint table
 * ========================================================================= */

/* Unlike its help twin it keeps the raw line count (no header line to drop)
 * and does not touch the page. */
// FUNCTION: LEGOLAND 0x00490800
int LoadHintTextFor(const char* key)
{
    char path[0x80];

    SetHintTextPrefix(key);
    FreeReportHintBuffer();
    sprintf(path, kFmtIntervals, key);
    g_rep_hint_count = LoadTextFileLines(path, g_rep_hints, 0x20);
    return g_rep_hint_count;
}


/* =========================================================================
 *  EnsureObjectClassLoaded -- load an element's object class on first use
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x00478b20
int EnsureObjectClassLoaded(const char* name)
{
    DBElem* e = ElemID(name);

    if (e) {
        if (!(e->flags & 4)) {
            if (!LoadObjectClass(e))
                return 0;
            e->flags |= 4;
            MarkElemUnavailable(e);
        }
        return 1;
    }
    return 0;
}


/* =========================================================================
 *  MarkElemAvailable -- grant an element to the build menu
 * ========================================================================= */

/* `marker` asks for the "new object" icon; the third argument is dead. */
// FUNCTION: LEGOLAND 0x00469900
void MarkElemAvailable(DBElem* e, int marker, int unused)
{
    if (e) {
        if (e->flags & 1) {
            if ((e->flags & 0x10002) != 2) {
                e->flags = (e->flags & ~0x10000) | 2;
                UnlockFreePlayEntry(e);
                if (marker)
                    AddNewObjectMarker(e->data);
            } else {
                DBPrintf(kAlreadyGotFmt, e->name);   /* BUG: %d of a pointer */
            }
        }
    }
    g_menu_dirty = 1;
}


/* =========================================================================
 *  ParseKeywordFile -- open "Scripts\\<name>" and run the section parser
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x004781f0
int ParseKeywordFile(const char* name, const void* table, int count, int arg)
{
    char  path[0x100];
    void* f;
    int   rc;

    sprintf(path, kFmtTwoStrings, kScriptsDir, name);
    f = RES_OpenFile(path);
    if (f) {
        strncpy(g_keyword_file_name, name, 0x80);
        rc = ParseKeywordSections(f, table, count, arg);
        RES_CloseFile(f);
        return rc;
    }
    return 0;
}


/* =========================================================================
 *  ReadDecodedNarration -- drain up to `len` bytes of decoded speech
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x004983a0
int ReadDecodedNarration(void* dst, int len)
{
    int avail;
    int total;
    int chunk;

    if (len > NarrationBytesReady()) {
        RefillNarrationRing();
        avail = NarrationBytesReady();
        if (len > avail)
            len = avail;
    }
    total = len;
    if (len) {
        do {
            chunk = NarrationContiguous();
            if (chunk > len)
                chunk = len;
            memcpy(dst, g_narr_ring + g_narr_d, chunk);
            len -= chunk;
            dst = (char*)dst + chunk;
            g_narr_d = (g_narr_d + chunk) & 0x1ffff;
        } while (len);
    }
    RefillNarrationRing();
    return total;
}


/* =========================================================================
 *  ResetLevelGlobals -- the per-level state, before a database loads
 * ========================================================================= */

/* movie.c's LoadLevelDatabase calls this between LLIDB_ClearOnLevel and the
 * keyword parse.  lpConfig is re-read at every use (the stores through it
 * could alias the pointer itself); the four cdecl cleanups at the end merge
 * into one `add esp,0x1c`. */
// FUNCTION: LEGOLAND 0x004784c0
void ResetLevelGlobals(void)
{
    g_oc_word = 0;
    g_oc_flag_a = 1;
    g_oc_flag_b = 1;
    g_level_number = 0;
    g_level_db_flag_ac = 1;
    g_level_byte_669050 = 0;
    g_script_cur = 0;
    g_level_int_669098 = 0;
    g_level_cfg->f30 = 0;
    g_level_cfg->f38 = 1;
    g_level_cfg->f34 = 1;
    g_level_cfg->visitor_cap = 200;
    g_visitor_cap = g_level_cfg->visitor_cap;
    g_visitor_cap_extra = 0;
    FreeScriptStrings();
    g_script_root = NewScriptEvent(0, 0, 0);
    g_level_db_active = 1;
    ClearReportState();
    ClearSim832b9c();
    ClearAppraisalState();
    ClearScriptStateBytes();
    ScriptState_NoOp();
    SetLevelGoalState(0, 0);
    ClearScriptTexts();
    ResetMoodAdjustments();
    ResetSimTuning();
    ClearMenuHelp();
    ClearIconHelp();
    SetReportMovie(kEmptyString);
    ResetDamageClock();
    ResetEntranceTileTime();
    ResetBuildTimer();
    g_bricks_full = 1000;
    g_show_capacity = 0;
    g_ride_wear = 0;
    g_832ba8 = 1;
    StopScript(0);
}


/* =========================================================================
 *  LoadTextFileLines -- read a RES text file into a line table
 * ========================================================================= */

/* Returns the number of lines cut, 0 for a missing or empty file or a failed
 * allocation.  The buffer is size + 2 so the NUL written after a final
 * unterminated line stays inside it. */
// FUNCTION: LEGOLAND 0x00490680
int LoadTextFileLines(const char* path, char** lines, int max)
{
    void* f;
    int   size;
    char* buf;
    int   i;
    int   n;

    f = RES_OpenFile(path);
    if (!f)
        return 0;
    size = RES_GetFileSize(f);
    if (size == 0) {
        RES_CloseFile(f);
        return 0;
    }
    buf = (char*)HeapAlloc_w(size + 2);
    if (!buf) {
        RES_CloseFile(f);
        return 0;
    }
    RES_ReadFile(f, buf, size);
    i = 0;
    n = 0;
    while (n < max && i < size) {
        lines[n] = buf + i;
        do
            ;
        while (buf[i++] != '\r' && i < size);
        if (i >= size)
            i++;
        buf[i - 1] = 0;
        i++;
        n++;
    }
    RES_CloseFile(f);
    return n;
}


/* =========================================================================
 *  BlitDIBToScreen -- double a 16-bit DIB onto the locked video surface
 * ========================================================================= */

/* WIP.  113 instructions in the original; this spelling is 66 of 111 aligned.
 * Structure is right (toggle, gap split, top fill, bottom-up rows doubled
 * two pixels by two rows with the 555->565 shift, bottom fill) and the
 * prologue's esi=row / edi=dib roles landed once the surface toggle was
 * written before the DIB reads; what remains is allocation: the original
 * gives w ebx and h ebp (ours the reverse), keeps the source cursor in edi
 * with the column countdown in memory (ours spills the cursor), and anchors
 * the second-row cursor at +2 while walking the first row at +0 (ours the
 * reverse).  Measured inert: the row loop as for(h), for(h-1..-1),
 * guarded do/while and for(0..h) (the guarded do/while with y = h - 1
 * hoisted above the top fill is what puts inc-from-h-1 in the right place);
 * the pixel temp as int/unsigned (worse, 46) versus short/unsigned short;
 * store orders d0/d1 interleaved either way and the *d++ forms. */
// WIP-FUNCTION: LEGOLAND 0x00465850  (59.5%, 66/111 vs 113 insns; allocation -- see note)
void BlitDIBToScreen(BitmapInfoHeader* dib)
{
    int             w;
    int             h;
    int             gap;
    int             top;
    int             bottom;
    unsigned short* src;
    unsigned short* p;
    unsigned char*  row;
    unsigned char*  row1;
    unsigned short* d0;
    unsigned short* d1;
    int             x;
    int             y;
    short           pix;

    g_last_blit_bits = (g_last_blit_bits != g_ddsd_bits) ? g_ddsd_bits : 0;
    h = dib->height;
    w = dib->width;
    gap = g_level_cfg->height - 2 * h;
    top = gap / 2;
    bottom = gap - top;
    y = h - 1;
    src = (unsigned short*)(dib + 1) + y * w;
    row = (unsigned char*)g_ddsd_bits;
    for (x = top; x > 0; x--) {
        memset(row, 0, 0x500);
        row += g_ddsd_pitch;
    }
    if (h != 0) {
        do {
            row1 = row + g_ddsd_pitch;
            if (w > 0) {
                p = src;
                d0 = (unsigned short*)row;
                d1 = (unsigned short*)row1;
                for (x = 0; x < w; x++) {
                    pix = *p++;
                    if (g_screen_depth == 2)
                        pix = (pix & 0x1f) | ((pix & ~0x1f) << 1);
                    d0[0] = pix;
                    d1[0] = pix;
                    d0[1] = pix;
                    d1[1] = pix;
                    d0 += 2;
                    d1 += 2;
                }
            }
            row += 2 * g_ddsd_pitch;
            src -= w;
        } while (y-- != 0);
    }
    row = row1 + g_ddsd_pitch;
    for (x = bottom; x > 0; x--) {
        memset(row, 0, 0x500);
        row += g_ddsd_pitch;
    }
}
