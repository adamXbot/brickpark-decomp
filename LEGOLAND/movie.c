/* LEGOLAND -- the AVI movie player and the front-end teardown around it.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd).
 * Field offsets and calling conventions are load-bearing; names are ours.
 *
 * ---------------------------------------------------------------------------
 * The movie player  (runtime spec)
 * ---------------------------------------------------------------------------
 * The game plays its FMV through Video for Windows' AVIFile API, not through
 * MCI or DirectShow.  OpenMovie builds a 0x28-byte Movie record:
 *
 *   AVIFileInit() the first time any movie is open (g_avi_open_count),
 *   AVIFileOpenA(&file, path, 0, 0), AVIFileInfoA -> dwStreams,
 *   then a linear walk of the streams: the FIRST 'vids' stream is kept (its
 *   rcFrame gives the pixel size, dwRate/dwScale the frame rate in Hz and
 *   dwLength the frame count), and the FIRST 'auds' stream is kept only to
 *   be held open; both are AddRef'd.  With no video stream the whole file is
 *   released and 0 returned.
 *
 * RunMovie decompresses through AVIStreamGetFrameOpen with a 16-bit
 * BITMAPINFOHEADER (g_movie_bmi) filled from the movie's own width/height, so
 * every frame arrives as RGB565/555 at the movie's native size, and blits one
 * frame per pass with the video surface LOCKED.  The frame actually wanted is
 * recomputed from the wall clock every pass --
 *     frame = (now - start) * fps / 1000
 * -- so the player DROPS frames rather than slowing down, and when it has
 * caught up entirely (the computed frame equals the one on screen) it spins
 * in a busy wait on the clock rather than sleeping.  A frame handle is
 * prefetched for frame+1 while the current one is displayed.
 *
 * Aborting: with `flags` non-zero (the in-game path) a lost message pump,
 * mouse button bit 0, any of the three mouse buttons (which also latches
 * g_movie_shown so the movie is not offered again) or DIK_SPACE stops it;
 * with `flags` zero (the attract path) only Ctrl+Q does.  Either way the
 * player then spins until the mouse is fully released so the aborting click
 * is not delivered to the screen underneath.
 *
 * ORIGINAL BUG, reproduced: CloseMovie releases the two streams and the
 * GETFRAME but never AVIFileRelease()s Movie.file -- every played movie leaks
 * one PAVIFILE.  It still decrements g_avi_open_count and AVIFileExit()s at
 * zero, so the leak is only reclaimed by AVIFile's own teardown.
 * ------------------------------------------------------------------------- */

#pragma intrinsic(memcpy, memset, strlen)
void* memcpy(void*, const void*, unsigned int);
void* memset(void*, int, unsigned int);
unsigned int strlen(const char*);

/* ---- local types -------------------------------------------------------- */

typedef struct Sprite Sprite;
typedef struct Elem Elem;

/* Win32 RECT, passed BY VALUE to the text printers (text.c's WinRect). */
typedef struct WinRect {
    long left;     /* +0x00 */
    long top;      /* +0x04 */
    long right;    /* +0x08 */
    long bottom;   /* +0x0c */
} WinRect;

/* iconui.c's 0x40-byte icon record; only the fields this file reads. */
typedef struct Icon {
    struct Icon*   next;      /* +0x00 */
    char           pad04[0x18 - 0x04];
    unsigned char  kind;      /* +0x18  1 = an object-class icon */
    char           pad19[0x1c - 0x19];
    char*          text;      /* +0x1c  the object class name for kind 1 */
    char           pad20[0x34 - 0x20];
    unsigned int   flags;     /* +0x34  0x400 = greyed out */
} Icon;

/* The edit-cursor block (0x1834 bytes @ 0x007febc0); only its ADDRESS is
 * used here, so the body is left opaque. */
typedef struct Cursor { unsigned char blk[0x1834]; } Cursor;

/* IDirectDrawSurface's vtable as text.c spells it: +0x44 GetDC, +0x68
 * ReleaseDC (PrintCursor borrows a GDI DC exactly like every text.c printer). */
typedef struct DDSurface DDSurface;
typedef struct DDSurfaceVtbl {
    char pad00[0x44];                                       /* +0x00 */
    long(__stdcall* GetDC)(DDSurface*, void** hdc);         /* +0x44 */
    char pad48[0x68 - 0x48];                                /* +0x48 */
    long(__stdcall* ReleaseDC)(DDSurface*, void* hdc);      /* +0x68 */
} DDSurfaceVtbl;
struct DDSurface { DDSurfaceVtbl* vtbl; };

/* IDirectSoundBuffer, as audio4.c/audio5.c spell it; the four slots the
 * narration ring buffer needs. */
typedef struct IDSBuffer IDSBuffer;
typedef struct IDSBufferVtbl {
    char pad00[0x2c];                                                  /* +0x00 */
    long (__stdcall *Lock)(IDSBuffer*, unsigned long off, unsigned long len,
                           void** p1, unsigned long* n1,
                           void** p2, unsigned long* n2,
                           unsigned long flags);                       /* +0x2c */
    char pad30[0x34 - 0x30];                                           /* +0x30 */
    long (__stdcall *SetCurrentPosition)(IDSBuffer*, unsigned long);   /* +0x34 */
    char pad38[0x48 - 0x38];                                           /* +0x38 */
    long (__stdcall *Stop)(IDSBuffer*);                                /* +0x48 */
    long (__stdcall *Unlock)(IDSBuffer*, void* p1, unsigned long n1,
                             void* p2, unsigned long n2);              /* +0x4c */
} IDSBufferVtbl;
struct IDSBuffer { IDSBufferVtbl* lpVtbl; };

/* The BITMAPINFOHEADER RunMovie hands to AVIStreamGetFrameOpen (@0x004bb4e0).
 * biSize/biPlanes/biCompression are pre-initialised in .data and never
 * written here; only the four fields the movie's own size decides are. */
typedef struct BitmapInfoHeader {
    unsigned long  biSize;          /* +0x00 */
    long           biWidth;         /* +0x04 */
    long           biHeight;        /* +0x08 */
    unsigned short biPlanes;        /* +0x0c */
    unsigned short biBitCount;      /* +0x0e */
    unsigned long  biCompression;   /* +0x10 */
    unsigned long  biSizeImage;     /* +0x14 */
    long           biXPelsPerMeter; /* +0x18 */
    long           biYPelsPerMeter; /* +0x1c */
    unsigned long  biClrUsed;       /* +0x20 */
    unsigned long  biClrImportant;  /* +0x24 */
} BitmapInfoHeader;

/* Video for Windows' AVIFILEINFOA (0x6c) and AVISTREAMINFOA (0x8c).  Only
 * the members this file reads are named for what they are; the sizes are
 * load-bearing (they are passed to the API). */
typedef struct AVIFILEINFO {
    unsigned long dwMaxBytesPerSec;      /* +0x00 */
    unsigned long dwFlags;               /* +0x04 */
    unsigned long dwCaps;                /* +0x08 */
    long          dwStreams;             /* +0x0c */
    unsigned long dwSuggestedBufferSize; /* +0x10 */
    unsigned long dwWidth;               /* +0x14 */
    unsigned long dwHeight;              /* +0x18 */
    unsigned long dwScale;               /* +0x1c */
    unsigned long dwRate;                /* +0x20 */
    unsigned long dwLength;              /* +0x24 */
    unsigned long dwEditCount;           /* +0x28 */
    char          szFileType[64];        /* +0x2c */
} AVIFILEINFO;

typedef struct AVISTREAMINFO {
    unsigned long  fccType;              /* +0x00  'vids' / 'auds' */
    unsigned long  fccHandler;           /* +0x04 */
    unsigned long  dwFlags;              /* +0x08 */
    unsigned long  dwCaps;               /* +0x0c */
    unsigned short wPriority;            /* +0x10 */
    unsigned short wLanguage;            /* +0x12 */
    unsigned long  dwScale;              /* +0x14 */
    unsigned long  dwRate;               /* +0x18 */
    unsigned long  dwStart;              /* +0x1c */
    unsigned long  dwLength;             /* +0x20 */
    unsigned long  dwInitialFrames;      /* +0x24 */
    unsigned long  dwSuggestedBufferSize;/* +0x28 */
    unsigned long  dwQuality;            /* +0x2c */
    unsigned long  dwSampleSize;         /* +0x30 */
    WinRect        rcFrame;              /* +0x34 */
    unsigned long  dwEditCount;          /* +0x44 */
    unsigned long  dwFormatChangeCount;  /* +0x48 */
    char           szName[64];           /* +0x4c */
} AVISTREAMINFO;

/* The 0x28-byte movie handle OpenMovie returns.  +0x20 and +0x24 are
 * allocated but never written or read by any of the three entry points. */
typedef struct Movie {
    int   frames;     /* +0x00  the video stream's dwLength, in frames */
    int   fps;        /* +0x04  dwRate / dwScale, in Hz */
    int   width;      /* +0x08  rcFrame.right - rcFrame.left */
    int   height;     /* +0x0c  rcFrame.bottom - rcFrame.top */
    void* file;       /* +0x10  PAVIFILE  (never released -- see the header) */
    void* getframe;   /* +0x14  PGETFRAME, opened and closed by RunMovie */
    void* audio;      /* +0x18  PAVISTREAM, held open only */
    void* video;      /* +0x1c  PAVISTREAM */
    int   f20;        /* +0x20  never touched */
    int   f24;        /* +0x24  never touched */
} Movie;

/* The three front-end state blocks the title screen pushes and pops.  Only
 * RestoreFrontEndState (the pop) is in this file; the push lives elsewhere. */
typedef struct SavedUiState {
    int icons2_mode;    /* +0x00 -> g_icons2_mode */
} SavedUiState;

typedef struct FrontEndState {
    int popup;          /* +0x00  0x0080ff80 */
    int screen;         /* +0x04  0x0080ff84 */
    int mode;           /* +0x08  0x0080ff88 */
} FrontEndState;

typedef struct EditState {
    int   changed;      /* +0x00  0x008119b0 */
    int   mode;         /* +0x04  0x008119b4 */
    void* object;       /* +0x08  0x008119b8 */
} EditState;

/* ---- globals ------------------------------------------------------------ */

extern Sprite* g_backdrop;           /* 0x00810148 full-screen background */
extern Sprite* g_cert_info_sprite;   /* 0x00798764 printinfo.lls */
extern int     g_info_active;        /* 0x007fdfa0 0 off, 1 panel, 2 mock panel */
/* g_edit and g_front are each ONE object: RestoreFrontEndState copies both
 * whole (a 12-byte struct assignment), which is what stops VC6 dead-storing
 * the g_front.screen write it then clobbers.  The individual fields are also
 * declared, because EnterParkPlayMode / SetInfoPanelText touch them singly and
 * the other files in the tree name them that way. */
extern EditState g_edit;             /* 0x008119b0 {EditMode, GameMode, EditObject} */
extern int     g_edit_changed;       /* 0x008119b0 (EditMode) */
extern int     g_game_mode;          /* 0x008119b4 3 = a running park */
extern void*   g_edit_object;        /* 0x008119b8 */
extern Cursor  g_edit_cursor;        /* 0x007febc0 (export EditCursor) */
extern unsigned int g_ui_flags;      /* 0x00813a40 */
extern void*   g_menu_help[4];       /* 0x004bb18c */
extern int     g_icons2_mode;        /* 0x00668e38 */
extern FrontEndState g_front;        /* 0x0080ff80 {PopUp, Screen, ScreenMode} */
extern int     g_screen_popup;       /* 0x0080ff80 pending front-end pop-up */
extern int     g_cur_screen;         /* 0x0080ff84 */
extern int     g_screen_mode;        /* 0x0080ff88 sub-mode of the screen */
extern Icon*   g_side_icons;         /* 0x006687c8 side-panel icon list head */
extern Icon*   g_brief_icon;         /* 0x00668e9c the info-panel briefing icon */
extern DDSurface* g_draw_surface;    /* 0x0066807c */
extern int     g_speech_state;       /* 0x0079a84c 2 = wound back, 3 = playing */
extern IDSBuffer* g_speech_buffer;   /* 0x0079a848 the 0xa000-byte narration ring */
extern int     g_speech_fill_block;  /* 0x0079a840 next 0x1000-byte block to fill */
extern int     g_speech_blocks_ready;/* 0x0079a844 blocks that carried real data */
extern WinRect g_clip_rect;          /* 0x004bdea0 */
extern int     g_avi_open_count;     /* 0x00668f98 open movies; AVIFileInit at 0 */
/* Latched to 0x16 whenever the opened file HAS an audio stream; the movie
 * audio path (0x004769e0 / 0x00476ac0) multiplies its per-frame byte count by
 * it.  What the 22 means is not established from these three sites. */
extern int     g_movie_audio_scale;  /* 0x00668fa4 */
extern BitmapInfoHeader g_movie_bmi; /* 0x004bb4e0 */
extern int     g_movie_shown;        /* 0x00668fb0 latched when a click skips one */
extern int     g_mouse_ev2;          /* 0x00813ac4 mouse button edge/hold bits */
extern int     g_mouse_btn_a;        /* 0x00813ad4 */
extern unsigned char g_key_state[256]; /* 0x007fdda0 DirectInput DIK_* key state */
extern int     g_rep_line_count;     /* 0x0079887c */
extern int     g_rep_page;           /* 0x004bf670 1-based first line */
extern char*   g_rep_lines[];        /* 0x007cafa0 the report line table */
extern const char kFmtIntervals[];   /* 0x004bf678 "Intervals\\%s" */
extern const void* g_level_db_sections; /* 0x004bb6f8 93 {keyword, handler} pairs */

/* ---- externs ------------------------------------------------------------ */

extern int   KillSprite(Sprite*);                        /* 0x00497bd0 */
extern void  RemoveIconGroup(int group);                 /* 0x0046d520 */
extern void  ResetInfoStruct(void);                      /* 0x00471510 */
extern char  PU_CloseInput(Icon*, int, short, short);    /* 0x004730f0 */
extern char  PU_NextInput(Icon*, int, short, short);     /* 0x00473360 */
extern void  SetInGameIconHandlers(void);                /* 0x00474880 */
extern void  DefaultCursor(Cursor*);                     /* 0x0045a390 */
extern void  BuildCursorPtr(Cursor*, int, int);          /* 0x0045f5f0 */
extern void  SetSfxPaused(void);                         /* 0x00492980 g_sfx_paused = 1 */
extern void  ClearSfxPaused(void);                       /* 0x00492990 g_sfx_paused = 0 */
extern void  LLIDB_ClearOnLevel(void);                   /* 0x0047b4c0 */
extern void  ResetLevelGlobals(void);                    /* 0x004784c0 */
extern int   ParseKeywordFile(const char*, const void*, int, int); /* 0x004781f0 */
extern void  progress_tick(void);                        /* 0x004663f0 */
extern int   EnsureObjectClassLoaded(const char* name);  /* 0x00478b20 */
extern Elem* ElemID(const char* name);                   /* 0x0047b3f0 */
extern void  MarkElemAvailable(Elem*, int, int);         /* 0x00469900 */
extern void  SetHelpTextPrefix(const char* key);         /* 0x00490740 */
extern void  FreeHelpTextBuffer(void);                   /* 0x00490850 */
extern int   LoadTextFileLines(const char* path, char** lines, int max); /* 0x00490680 */
extern int   LoadHintTextFor(const char* key);           /* 0x00490800 */
extern int   ReadDecodedNarration(void* dst, int len);   /* 0x004983a0 */
extern int   ProcessSystemEvents(void);                  /* 0x00480050 */
extern void  ReadGameButtons(void);                      /* 0x00452460 */
extern void  PushRenderingStatusAndLockVideoSurface(void); /* 0x00463fc0 */
extern int   RenderingComplete(void);                    /* 0x00466500 */
extern void  BlitDIBToScreen(void* dib);                 /* 0x00465850 */
/* The movie audio path (names ours; nothing else in the tree calls them).
 * 0x00476910 opens a KLIBAUDIO buffer for mv->audio and returns non-zero when
 * there is one; 0x00476bf0 primes it at the first displayed frame;
 * 0x00476d20 tops it up per frame step; 0x00476c90 tears it down. */
extern int   StartMovieAudio(Movie* mv);                 /* 0x00476910 */
#ifndef LEGOLAND_PORTABLE
extern void  PrimeMovieAudio(Movie* mv);                 /* 0x00476bf0 */
#else
extern int PrimeMovieAudio(Movie* mv);                 /* 0x00476bf0 */
#endif
#ifndef LEGOLAND_PORTABLE
extern void  UpdateMovieAudio(int frame, int prev);      /* 0x00476d20 */
#else
extern int UpdateMovieAudio(int frame, int prev);      /* 0x00476d20 */
#endif
#ifndef LEGOLAND_PORTABLE
extern void  StopMovieAudio(void);                       /* 0x00476c90 */
#else
extern int StopMovieAudio(void);                       /* 0x00476c90 */
#endif
/* 0x00476680: a millisecond clock -- QueryPerformanceCounter scaled by its
 * frequency where one exists, GetTickCount otherwise (the mode is latched in
 * 0x00668fac on the first call). */
extern unsigned int MovieTicks(void);                    /* 0x00476680 */
extern int   sprintf(char*, const char*, ...);           /* 0x0049e573 (CRT) */
extern void  HeapFree_w(void*);                          /* 0x0049e4d0 */
extern void* HeapAlloc_w(unsigned int);                  /* 0x0049e4ff (CRT malloc) */
extern void* SelectFont(void* dc, int font);             /* 0x00454b40 */
extern void  PushRenderingStatusAndUnlockVideoSurface(void); /* 0x00464080 */
extern void  PopRenderingStatus(void);                   /* 0x004641f0 */

/* ---- imports ------------------------------------------------------------ */

__declspec(dllimport) void*         __stdcall CreateRectRgn(int l, int t, int r, int b); /* [0x4ab0b8] */
__declspec(dllimport) void*         __stdcall SelectObject(void* dc, void* obj);         /* [0x4ab080] */
__declspec(dllimport) int           __stdcall SetBkMode(void* dc, int mode);             /* [0x4ab074] */
__declspec(dllimport) unsigned long __stdcall SetTextColor(void* dc, unsigned long c);   /* [0x4ab0b0] */
__declspec(dllimport) int           __stdcall DrawTextA(void* dc, const char* s, int n,
                                                        WinRect* rc, unsigned int fmt);  /* [0x4ab2ac] */
__declspec(dllimport) int           __stdcall DeleteObject(void* obj);                   /* [0x4ab09c] */

/* AVIFile (AVIFIL32.DLL).  These are called through the linker's own jump
 * thunks in the original, not through __imp__ slots, so they are declared
 * WITHOUT __declspec(dllimport) -- the same spelling audio4.c uses for the
 * ACM entry points. */
void          __stdcall AVIFileInit(void);                                        /* 0x0049e406 */
void          __stdcall AVIFileExit(void);                                        /* 0x0049e3d6 */
long          __stdcall AVIFileOpenA(void**, const char*, unsigned int, const void*); /* 0x0049e400 */
long          __stdcall AVIFileInfoA(void* pfile, void* pfi, long size);          /* 0x0049e3fa */
long          __stdcall AVIFileGetStream(void*, void**, unsigned long, long);         /* 0x0049e3f4 */
unsigned long __stdcall AVIFileRelease(void* pfile);                              /* 0x0049e3dc */
long          __stdcall AVIStreamInfoA(void* pavi, void* psi, long size);         /* 0x0049e3ee */
unsigned long __stdcall AVIStreamAddRef(void* pavi);                              /* 0x0049e3e8 */
unsigned long __stdcall AVIStreamRelease(void* pavi);                             /* 0x0049e3e2 */
void*         __stdcall AVIStreamGetFrameOpen(void* pavi, const void* wanted);    /* 0x0049e412 */
void*         __stdcall AVIStreamGetFrame(void* pg, long pos);                    /* 0x0049e418 */
long          __stdcall AVIStreamGetFrameClose(void* pg);                         /* 0x0049e40c */

/* =========================================================================
 *  Front-end teardown
 * ========================================================================= */

/* Bind one of the four menu help slots.  Out-of-range slots are silently
 * dropped, which is what lets tinystubs.c's ClearMenuHelp loop 0..3 blind. */
// FUNCTION: LEGOLAND 0x00475fe0
void SetMenuHelp(int slot, void* text)
{
    if (slot >= 0 && slot < 4)
        g_menu_help[slot] = text;
}

/* Drop the advert (title) screen: its backdrop sprite and icon group 7. */
// FUNCTION: LEGOLAND 0x0048ffb0
void KillAdvertScreenSprites(void)
{
    if (g_backdrop) {
        KillSprite(g_backdrop);
        g_backdrop = 0;
    }
    RemoveIconGroup(7);
}

/* Close the info pop-up if one is up, by handing its own close handler a
 * synthetic click (event 2) on a null icon.  Non-zero when it did. */
// FUNCTION: LEGOLAND 0x00473130
int CloseInfoPopUpIfOpen(void)
{
    if (g_info_active) {
        PU_CloseInput(0, 2, 0, 0);
        return 1;
    }
    return 0;
}

/* Enter the in-game (running park) mode with the default edit cursor.
 * Named from what it does; StartFreePlayPark and the level start-up path
 * (0x00458acf) are the two callers, which spell it sub_458940. */
// FUNCTION: LEGOLAND 0x00458940
void EnterParkPlayMode(void)
{
    g_edit_changed = 0;
    g_game_mode = 3;
    SetInGameIconHandlers();
    g_edit_object = 0;
    DefaultCursor(&g_edit_cursor);
    BuildCursorPtr(&g_edit_cursor, 0x8f8, 0);
    g_ui_flags = (g_ui_flags & ~0x1400u) | 0x20;
}

/* Close whichever primary pop-up is up.  State 1 is the real info panel (it
 * just resets the info block); state 2 is the mock panel, closed by handing
 * its NEXT handler a synthetic click -- and the state value 2 is what is
 * pushed as the event, which is why the original re-uses the register. */
// FUNCTION: LEGOLAND 0x00473160
int ClosePrimaryPopUp(void)
{
    int state = g_info_active;

    if (state == 1) {
        ResetInfoStruct();
        return 1;
    }
    if (state == 2) {
        PU_NextInput(0, state, 0, 0);
        return 1;
    }
    return 0;
}

/* Drop the certificate screen: the backdrop, the printed-info sprite and
 * icon group 7. */
// FUNCTION: LEGOLAND 0x00490270
void KillCertScreenSprites(void)
{
    if (g_backdrop) {
        KillSprite(g_backdrop);
        g_backdrop = 0;
    }
    if (g_cert_info_sprite) {
        KillSprite(g_cert_info_sprite);
        g_cert_info_sprite = 0;
    }
    RemoveIconGroup(7);
}

/* Load a level database (a keyword-sectioned text file) with new sample
 * instances forced to start paused for the duration.  Returns 2 when the
 * parser failed and 0 when it did not -- uimisc2.c stores that straight into
 * g_freeplay_db and declares the function `void*`, an extern-type divergence
 * left alone. */
// FUNCTION: LEGOLAND 0x0047afb0
int LoadLevelDatabase(const char* name)
{
    int rc;

    SetSfxPaused();
    LLIDB_ClearOnLevel();
    ResetLevelGlobals();
    rc = ParseKeywordFile(name, &g_level_db_sections, 0x5d, 0);
    ClearSfxPaused();
    return (rc < 0) ? 2 : 0;
}

/* Pop the front-end state the title screen pushed.
 *
 * ORIGINAL BUG, reproduced: the current g_screen_mode is latched at entry and
 * written back over g_cur_screen, so the SAVED screen index (screen->screen,
 * stored one instruction earlier) is discarded and the screen index becomes
 * whatever sub-mode happened to be current.  The saved sub-mode is then
 * restored correctly, so the pair ends up mismatched. */
// FUNCTION: LEGOLAND 0x0048fa40
void RestoreFrontEndState(SavedUiState* ui, FrontEndState* screen,
                          EditState* game)
{
    int mode = g_front.mode;

    g_icons2_mode = ui->icons2_mode;
    g_edit = *game;
    g_front = *screen;
    g_front.screen = mode;            /* BUG: discards screen->screen */
}

/* Load the report/help text for `key` into the report line table, set the
 * string-id prefix from the key's file name and reset the page.  The stored
 * line count is one LESS than the number of lines read whenever any were
 * read (the first line of the file is a header), and that is also the
 * result. */
// FUNCTION: LEGOLAND 0x004907a0
int LoadHelpTextFor(const char* key)
{
    char path[0x80];

    SetHelpTextPrefix(key);
    FreeHelpTextBuffer();
    sprintf(path, kFmtIntervals, key);
    g_rep_line_count = LoadTextFileLines(path, g_rep_lines, 0x64);
    if (g_rep_line_count) {
        g_rep_page = 1;
        g_rep_line_count--;
    }
    return g_rep_line_count;
}

/* Bring up the info panel for a script step: the briefing text `a` always,
 * the hint text `b` when there is one.  With no briefing text the briefing
 * icon is greyed (flag 0x400) and nothing else changes. */
// FUNCTION: LEGOLAND 0x004911c0
int SetInfoPanelText(const char* a, const char* b)
{
    Icon* icon;

    if (LoadHelpTextFor(a)) {
        if (b)
            LoadHintTextFor(b);
        icon = g_brief_icon;
        g_icons2_mode = 1;
        g_game_mode = 2;
        g_cur_screen = -1;
        g_screen_mode = 7;
        if (icon)
            icon->flags &= ~0x400u;
        return 1;
    }
    icon = g_brief_icon;
    if (icon)
        icon->flags |= 0x400u;
    return 0;
}

/* Walk the side-panel icons and make every object class one of them names
 * available in the build menu.  Named from what it does; uimisc2.c spells it
 * sub_48ab60.  progress_tick() drives the loading bar per icon. */
// FUNCTION: LEGOLAND 0x0048ab60
void UnlockSidePanelObjects(void)
{
    Icon* p = g_side_icons;

    while (p) {
        progress_tick();
        if (p->kind == 1 && EnsureObjectClassLoaded(p->text))
            MarkElemAvailable(ElemID(p->text), 0, 1);
        p = p->next;
    }
}

/* =========================================================================
 *  Text — the small-font, left-aligned printer
 * ========================================================================= */

/* text.c's NewPrintCent (0x00491d60) with DT_CENTER dropped: single-line text
 * vertically centred in a caller-supplied box (BY VALUE) but LEFT aligned,
 * which is what lets screens2.c park a blinking cursor at the end of a name
 * being typed, and what uimisc2.c's PrintReportLine uses for the small font.
 * Index for index the same body; only the DrawText flags differ. */
// FUNCTION: LEGOLAND 0x00490fa0
void PrintCursor(const char* text, int font, WinRect rc, char white)
{
    void* hdc;
    void* rgn;
    void* oldrgn;
    void* oldfont;

    rgn = CreateRectRgn(g_clip_rect.left, g_clip_rect.top,
                        g_clip_rect.right, g_clip_rect.bottom);
    PushRenderingStatusAndUnlockVideoSurface();
    g_draw_surface->vtbl->GetDC(g_draw_surface, &hdc);
    SetBkMode(hdc, 1);
    if (white == 1)
        SetTextColor(hdc, 0xffffff);
    oldrgn = SelectObject(hdc, rgn);
    oldfont = SelectFont(hdc, font);
    DrawTextA(hdc, text, strlen(text), &rc, 0x24);   /* DT_VCENTER|DT_SINGLELINE */
    SelectObject(hdc, oldfont);
    SelectObject(hdc, oldrgn);
    g_draw_surface->vtbl->ReleaseDC(g_draw_surface, hdc);
    PopRenderingStatus();
    DeleteObject(rgn);
}

/* =========================================================================
 *  The movie player
 * ========================================================================= */

/* Tear a movie handle down.
 *
 * ORIGINAL BUG, reproduced: mv->file (the PAVIFILE) is never released, so a
 * PAVIFILE leaks per movie.  The AVIFile library itself is only shut down
 * when the last movie closes. */
// FUNCTION: LEGOLAND 0x00476630
void CloseMovie(Movie* mv)
{
    if (mv->getframe)
        AVIStreamGetFrameClose(mv->getframe);
    if (mv->video)
        AVIStreamRelease(mv->video);
    if (mv->audio)
        AVIStreamRelease(mv->audio);
    HeapFree_w(mv);
    if (--g_avi_open_count == 0)
        AVIFileExit();
}

/* =========================================================================
 *  Narration
 * ========================================================================= */

/* Rewind the narration ring and refill it from the decoder.
 *
 * MECHANICS.  The DirectSound buffer is 0xa000 bytes = TEN 0x1000-byte
 * blocks; the whole thing is locked as one region and each block is decoded
 * into a stack staging buffer and copied in, the short final block being
 * zero-padded to silence.  g_speech_blocks_ready counts the blocks that
 * carried any data at all, and g_speech_fill_block -- the global cursor the
 * streaming thread also uses -- is left at 0 for playback.  State 2 means
 * "wound back and ready"; the function is a no-op in that state and while
 * nothing is loaded at all. */
// FUNCTION: LEGOLAND 0x004989b0
int RewindNarrationBuffer(void)
{
    void*         p1;
    unsigned long n1;
    char          blk[0x1000];
    int           n;

    if (g_speech_state == 2 || g_speech_state == 0) return 0;

    g_speech_buffer->lpVtbl->Stop(g_speech_buffer);
    g_speech_buffer->lpVtbl->SetCurrentPosition(g_speech_buffer, 0);
    g_speech_fill_block = 0;
    if (g_speech_buffer->lpVtbl->Lock(g_speech_buffer, 0, 0xa000,
                                      &p1, &n1, 0, 0, 0) == 0) {
        for (; g_speech_fill_block < 10; g_speech_fill_block++) {
            n = ReadDecodedNarration(blk, 0x1000);
            memcpy((char*)p1 + (g_speech_fill_block << 12), blk, n);
            if (n < 0x1000)
                memset((char*)p1 + (g_speech_fill_block << 12) + n, 0, 0x1000 - n);
            if (n)
                g_speech_blocks_ready++;
        }
        g_speech_fill_block = 0;
        g_speech_buffer->lpVtbl->Unlock(g_speech_buffer, p1, n1, 0, 0);
    }
    g_speech_state = 2;
    return 1;
}

/* Open one .AVI and build a movie handle.
 *
 * MECHANICS.  The stream walk keeps the LAST 'vids' and the LAST 'auds'
 * stream it sees (each assignment overwrites the previous without releasing
 * it), and both are AddRef'd on top of the reference AVIFileGetStream already
 * returned, which is what lets CloseMovie release them once each after the
 * file itself has gone.  The frame rate is the integer quotient dwRate /
 * dwScale, so a 29.97 Hz file plays at 29.  Returns 0 with everything
 * released when the file will not open, has no video stream, or the handle
 * cannot be allocated.
 *
 * Note `fi.dwStreams = 0;` before AVIFileInfoA: the count is pre-cleared so a
 * failing call cannot leave the walk reading a garbage stack value -- the
 * result of AVIFileInfoA itself is never checked. */
// FUNCTION: LEGOLAND 0x00476460
Movie* OpenMovie(const char* path)
{
    void*  pfile;
    void*  audio = 0;
    int    fps;
    int    i;
    void*  stream;
    AVISTREAMINFO si;
    AVIFILEINFO   fi;
    void*  video = 0;
    int    frames;
    int    w;
    int    h;
    Movie* mv;

    if (g_avi_open_count == 0)
        AVIFileInit();
    if (AVIFileOpenA(&pfile, path, 0, 0) != 0) {
        if (g_avi_open_count == 0)
            AVIFileExit();
        return 0;
    }
    fi.dwStreams = 0;
    AVIFileInfoA(pfile, &fi, 0x6c);
    for (i = 0; i < fi.dwStreams; i++) {
        if (AVIFileGetStream(pfile, &stream, 0, i) != 0)
            break;
        if (AVIStreamInfoA(stream, &si, 0x8c) != 0)
            continue;
        if (si.fccType == 0x73646976) {
            video = stream;
            AVIStreamAddRef(stream);
            frames = si.dwLength;
            fps = si.dwRate / si.dwScale;
            w = si.rcFrame.right - si.rcFrame.left;
            h = si.rcFrame.bottom - si.rcFrame.top;
        } else if (si.fccType == 0x73647561) {
            g_movie_audio_scale = 0x16;
            audio = stream;
            AVIStreamAddRef(stream);
        }
    }
    if (!video) {
        if (audio)
            AVIStreamRelease(audio);
        AVIFileRelease(pfile);
        if (g_avi_open_count == 0)
            AVIFileExit();
        return 0;
    }
    mv = (Movie*)HeapAlloc_w(0x28);
    if (!mv) {
        AVIStreamRelease(video);
        if (audio)
            AVIStreamRelease(audio);
        AVIFileRelease(pfile);
        if (g_avi_open_count == 0)
            AVIFileExit();
        return 0;
    }
    mv->frames = frames;
    mv->fps = fps;
    mv->width = w;
    mv->height = h;
    mv->file = pfile;
    mv->getframe = 0;
    mv->audio = audio;
    mv->video = video;
    g_avi_open_count++;
    return mv;
}

/* Play an open movie to the end or to an abort.
 *
 * MECHANICS.  `dst` is never read -- the frame always lands wherever
 * BlitDIBToScreen puts it, at the movie's own size, so uimisc2.c's fixed
 * 320x240 rectangle is decoration.  Returns 0 when there is no movie or the
 * decompressor hands back a null frame, 1 otherwise (including every abort).
 *
 * Frame selection is CLOCK-driven, not counter-driven: the wanted frame is
 * (now - start) * fps / 1000, recomputed every pass, so a slow machine drops
 * frames rather than running slow.  `next` holds the decompressed frame to
 * show and `frame` is the state that says where it came from: -1 means
 * "fetch the frame for `cur` now", anything else means "`next` already holds
 * it".  When the clock has NOT moved on, the frame AFTER the current one is
 * decompressed ahead of time into `next` and `frame` is set to that frame's
 * INDEX -- the same variable carrying a pointer on one pass and an index on
 * the next, purely as a not-(-1) sentinel.  That overloading is what makes
 * `frame` VC6's fourth callee-saved value (ebx) and leaves `shown` in the
 * frame; it is also why the last pass of a movie leaves a stale `next`
 * behind (the prefetch is skipped at the end but `frame` is still set),
 * harmlessly, because the loop exits on the same pass.
 *
 * The catch-up wait is a SPIN on the clock, not a sleep. */
// FUNCTION: LEGOLAND 0x004766f0
int RunMovie(Movie* mv, WinRect* dst, int flags)
{
    /* DECLARATION ORDER IS LOAD-BEARING HERE: it decides the callee-saved
     * ranking (frame->ebx, cur->esi, start->ebp) and therefore which of the
     * four zero-valued initialisers becomes the shared zero register the two
     * entry guards compare against.  Six of the twenty-four orders of these
     * four names give the original; the rest cost 2 to 6 instructions. */
    int          frame = -1;
    int          cur = 0;
    unsigned int start = 0;
    int          shown = 0;
    void*        next;
    int          audio;
    int          w;
    int          h;

    (void)dst;
    if (mv == 0)
        return 0;
    if (mv->video == 0)
        return 0;
    audio = StartMovieAudio(mv);
    g_movie_bmi.biBitCount = 0x10;
    w = mv->width;
    g_movie_bmi.biWidth = w;
    h = mv->height;
    g_movie_bmi.biHeight = h;
    g_movie_bmi.biSizeImage = h * w * 2;
    mv->getframe = AVIStreamGetFrameOpen(mv->video, &g_movie_bmi);
    while (cur < mv->frames) {
        if (flags) {
            if (!ProcessSystemEvents())
                break;
            ReadGameButtons();
            if (g_mouse_ev2 & 1)
                break;
            if (g_mouse_btn_a & 7) {
                g_movie_shown = 1;
                break;
            }
            if (g_key_state[0x39] & 0x80)          /* DIK_SPACE */
                break;
        } else {
            ProcessSystemEvents();
            if ((g_key_state[0x1d] | g_key_state[0x9d]) & 0x80) {   /* either Ctrl: the original loads 0x9d first (right operand) */
                if (g_key_state[0x10] & 0x80)                       /* DIK_Q */
                    break;
            }
        }
        if (frame == -1)
            frame = (int)(next = AVIStreamGetFrame(mv->getframe, cur));
        else
            frame = (int)next;
        if (frame == 0) {
            mv->getframe = 0;
            return 0;
        }
        PushRenderingStatusAndLockVideoSurface();
        BlitDIBToScreen((void*)frame);
        PopRenderingStatus();
        if (start == 0) {
            if (audio)
                PrimeMovieAudio(mv);
            start = MovieTicks();
        }
        RenderingComplete();
        if ((MovieTicks() - start) * mv->fps / 1000 == (unsigned int)cur) {
            frame = cur + 1;
            if (frame < mv->frames)
                next = AVIStreamGetFrame(mv->getframe, frame);
        } else {
            frame = -1;
        }
        while (cur == shown)
            cur = (MovieTicks() - start) * mv->fps / 1000;
        if (audio)
            UpdateMovieAudio(cur, shown);
        shown = cur;
    }
    StopMovieAudio();
    do {
        ProcessSystemEvents();
        ReadGameButtons();
    } while (g_mouse_ev2 & 6);
    AVIStreamGetFrameClose(mv->getframe);
    mv->getframe = 0;
    return 1;
}
