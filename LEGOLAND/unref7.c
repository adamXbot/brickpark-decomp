/* LEGOLAND -- unreferenced (dead) code kept by the linker, 0x00476450..0x0049c110.
 *
 * Scope LL15 (docs/SCOPE_LL15_unref_sim_profiles_screens.md).  Nothing live in
 * the image calls, tail-jumps to or takes the address of any of these bodies;
 * they survive only because the game was linked without /OPT:REF.  They are
 * ordinary C from the same translation units as their nearest matched
 * neighbours (movie.c, coaster.c, memdb.c, pathsq.c, workorder2.c, workers.c,
 * printlist.c, tri3d.c, screens2/3.c, profiles.c, music.c, text.c, levelkw.c,
 * util.c, savechunks.c), so the structs and globals below are those files'.
 *
 * Names are ours; only offsets and addresses are load-bearing.  Where a body
 * gives no clue at all what it was called the name is `Unref_<VA>` and it is
 * recorded in docs/lanes/scope-ll15.md.
 */
#include "legoland.h"

void* memset(void*, int, unsigned int);
char* strcpy(char*, const char*);
char* strcat(char*, const char*);
unsigned int strlen(const char*);
void* memcpy(void*, const void*, unsigned int);
#pragma intrinsic(memset, memcpy, strcpy, strcat, strlen)

/* ======================================================================== *
 *  types
 * ======================================================================== */

/* iconui.c's icon record, as screens3.c spells it. */
typedef struct Icon {
    struct Icon*   next;      /* +0x00 */
    void*          sprite;    /* +0x04 */
    void*          data;      /* +0x08 */
    short          x;         /* +0x0c */
    short          y;         /* +0x0e */
    short          w;         /* +0x10 */
    short          h;         /* +0x12 */
    unsigned short group;     /* +0x14 */
    short          pad16;     /* +0x16 */
    union {
        void*          owner;   /* +0x18 */
        unsigned char  level;   /* +0x18 */
        unsigned short control; /* +0x18  option screen: control number */
    } u18;
    unsigned char  slot;      /* +0x1c */
    char           pad1d[3];  /* +0x1d */
    unsigned char  flags20;   /* +0x20 */
    char           pad21[0x28 - 0x21];
    int          (*render)(struct Icon*);               /* +0x28 */
    char         (*input)(struct Icon*, int, int, int);  /* +0x2c */
    void*          widget;    /* +0x30 */
    unsigned int   flags;     /* +0x34 */
    char*          help;      /* +0x38 */
    int            help_id;   /* +0x3c */
} Icon;

/* profiles.c's on-disk profile record and its list node. */
#pragma pack(push, 1)
typedef struct ProfileStats {
    int            a;            /* +0x00 */
    int            b;            /* +0x04 */
    int            c;            /* +0x08 */
    unsigned short d;            /* +0x0c */
    unsigned char  e;            /* +0x0e */
} ProfileStats;                  /* 15 bytes */

typedef struct Profile {
    char           name[0x20];   /* +0x00 */
    unsigned int   age;          /* +0x20  1..99, the age spinner's value */
    unsigned char  f24;          /* +0x24 */
    unsigned char  pad25[3];     /* +0x25 */
    int            f28;          /* +0x28 */
    int            f2c;          /* +0x2c */
    int            f30;          /* +0x30 */
    ProfileStats   stats;        /* +0x34 */
    char           block[200];   /* +0x43 */
    int            tail;         /* +0x10b */
    unsigned char  f10f;         /* +0x10f */
} Profile;                       /* 0x110 */
#pragma pack(pop)

typedef struct ProfileNode {
    struct ProfileNode* next;    /* +0x000 */
    Profile             p;       /* +0x004 */
    int                 valid;   /* +0x114 */
    unsigned char       slot;    /* +0x118 */
} ProfileNode;                   /* 0x11c */

/* renderlist.c's ride seat slot list (head at the owner's +0xcc). */
typedef struct SeatSlot {
    struct SeatSlot* next;   /* +0x00 */
    int              pad4;   /* +0x04 */
    void*            bloke;  /* +0x08 */
    unsigned short   seat;   /* +0x0c */
    unsigned short   pade;   /* +0x0e */
} SeatSlot;

typedef struct SeatOwner {
    char       pad0[0xcc];   /* +0x00..0xcb */
    SeatSlot*  head;         /* +0xcc */
} SeatOwner;

/* pathmisc.c's path-to-point open list. */
typedef struct PTPNode {
    struct PTPNode* next;    /* +0x00 */
} PTPNode;

/* sprite2.c's sprite list node. */
typedef struct SpriteRec {
    struct SpriteRec* next;  /* +0x00 */
} SpriteRec;

/* blokelist.c's layered sprite object and its layer holder. */
typedef struct SprObj {
    unsigned char     pad00[8];  /* +0x00..0x07 */
    struct SprLayers* layers;    /* +0x08 */
    unsigned char     pad0c[4];  /* +0x0c..0x0f */
    unsigned int      flags;     /* +0x10  0x8000 = layered, 0x4000 = hidden */
    short             w;         /* +0x14 */
    short             h;         /* +0x16 */
} SprObj;

/* legoland.h's Layers with the two render-offset arrays named. */
typedef struct SprLayers {
    unsigned char pad00[4];  /* +0x00 */
    int           count;     /* +0x04 */
    SprObj**      sprites;   /* +0x08 */
    int*          render_ox; /* +0x0c */
    int*          render_oy; /* +0x10 */
} SprLayers;

/* tri3d.c's cached shading ramp and the palette entries a ramp table is
 * built from (4 bytes each -- the shade builder reads the first three). */
typedef struct Shade {
    int             levels;   /* +0x00 */
    unsigned short* table;    /* +0x04 */
} Shade;

typedef struct PalEntry {
    unsigned char r;   /* +0x00 */
    unsigned char g;   /* +0x01 */
    unsigned char b;   /* +0x02 */
    unsigned char a;   /* +0x03 */
} PalEntry;

/* The four-int bounding box GetLayeredSpriteBounds fills. */
typedef struct Rect4 {
    int x0;   /* +0x00 */
    int y0;   /* +0x04 */
    int x1;   /* +0x08 */
    int y1;   /* +0x0c */
} Rect4;

/* workorder2.c's bloke, only the fields this file reads. */
typedef struct Bloke {
    struct Bloke*  next;        /* +0x00 */
    unsigned char  pad04[8];    /* +0x04..0x0b */
    unsigned short plan;        /* +0x0c  long-term action */
    unsigned short state;       /* +0x0e */
    unsigned char  pad10[0x58]; /* +0x10..0x67 */
    Pos            world;       /* +0x68  world position, 24.8 */
} Bloke;

/* music.c's DirectMusic interfaces -- only the slots called here. */
typedef struct GUID_ {
    unsigned long  Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char  Data4[8];
} GUID_;

typedef struct IDMStyle IDMStyle;
typedef struct IDMStyleVtbl {
    long          (__stdcall *QueryInterface)(IDMStyle*, const void*, void**);      /* +0x00 */
    unsigned long (__stdcall *AddRef)(IDMStyle*);                                   /* +0x04 */
    unsigned long (__stdcall *Release)(IDMStyle*);                                  /* +0x08 */
    long          (__stdcall *GetBand)(IDMStyle*, unsigned short*, void**);         /* +0x0c */
    long          (__stdcall *EnumBand)(IDMStyle*, long, unsigned short*);          /* +0x10 */
    long          (__stdcall *GetDefaultBand)(IDMStyle*, void**);                   /* +0x14 */
    long          (__stdcall *EnumMotif)(IDMStyle*, long, unsigned short*);         /* +0x18 */
    long          (__stdcall *GetMotif)(IDMStyle*, unsigned short*, void**);        /* +0x1c */
    long          (__stdcall *GetDefaultChordMap)(IDMStyle*, void**);               /* +0x20 */
    long          (__stdcall *EnumChordMap)(IDMStyle*, long, unsigned short*);      /* +0x24 */
    long          (__stdcall *GetChordMap)(IDMStyle*, unsigned short*, void**);     /* +0x28 */
} IDMStyleVtbl;
struct IDMStyle { IDMStyleVtbl* lpVtbl; };

typedef struct IDMLoader IDMLoader;
typedef struct IDMLoaderVtbl {
    long          (__stdcall *QueryInterface)(IDMLoader*, const void*, void**);     /* +0x00 */
    unsigned long (__stdcall *AddRef)(IDMLoader*);                                  /* +0x04 */
    unsigned long (__stdcall *Release)(IDMLoader*);                                 /* +0x08 */
    long          (__stdcall *GetObject)(IDMLoader*, void* desc, const void* iid,
                                         void** out);                               /* +0x0c */
} IDMLoaderVtbl;
struct IDMLoader { IDMLoaderVtbl* lpVtbl; };

/* DMUS_OBJECTDESC (0x350 bytes) */
typedef struct DMObjectDesc {
    unsigned long  dwSize;                /* +0x000 */
    unsigned long  dwValidData;           /* +0x004 */
    GUID_          guidObject;            /* +0x008 */
    GUID_          guidClass;             /* +0x018 */
    unsigned long  ftDate[2];             /* +0x028 */
    unsigned long  vVersion[2];           /* +0x030 */
    unsigned short wszName[64];           /* +0x038 */
    unsigned short wszCategory[64];       /* +0x0b8 */
    unsigned short wszFileName[260];      /* +0x138 */
    __int64        llMemLength;           /* +0x340 */
    unsigned char* pbMemData;             /* +0x348 */
    void*          pStream;               /* +0x34c */
} DMObjectDesc;

#define DMUS_OBJ_CLASS    0x02
#define DMUS_OBJ_NAME     0x04
#define DMUS_OBJ_FILENAME 0x10

/* objmap2.c's edit / destroy cursor block. */
typedef struct Cursor {
    unsigned short count;        /* +0x0000 */
    short          px[0x400];    /* +0x0002 */
    short          py[0x400];    /* +0x0802 */
    unsigned char  kind[0x400];  /* +0x1002 */
    unsigned char  pad1402[2];   /* +0x1402 */
    Pos            origin;       /* +0x1404 */
    int            status;       /* +0x140c */
    int            error;        /* +0x1410 */
    Rect           rect;         /* +0x1414 */
    unsigned char  style;        /* +0x1428 */
    unsigned char  pad1429[0x1828 - 0x1429];
    unsigned int   flags;        /* +0x1828 */
    int            f182c;        /* +0x182c */
    struct Cursor* next;         /* +0x1830 */
} Cursor;

/* pathsq.c's path square. */
typedef struct PathSquare {
    struct PathSquare* next;     /* +0x00 */
    int                pad4;     /* +0x04 */
    Rect               rect;     /* +0x08 */
    int                distance2;/* +0x1c */
    int                flags;    /* +0x20 */
} PathSquare;

/* ======================================================================== *
 *  globals
 * ======================================================================== */

/* movie.c's TU.  0x00668fa8 sits between g_movie_audio_scale (0x00668fa4) and
 * the movie clock-mode latch (0x00668fac); the store below is the ONLY
 * reference to it anywhere in the image, so what it meant is unrecoverable. */
extern int          g_movie_fa8;            /* 0x00668fa8 */

/* The "IconList.txt" debug dump.  Both globals and the literal are referenced
 * only from the one dead opener below. */
extern void*        g_iconlist_log;         /* 0x00798654 */
extern int          g_iconlist_log_open;    /* 0x00798658 */
extern const char   g_name_iconlist[];      /* 0x004beb60 "IconList.txt" */
extern const char   g_mode_wplus[];         /* 0x004beb70 "w+" */

extern PTPNode*     g_ptp_open_head;        /* 0x0066b450 */
extern SpriteRec*   g_sprites_head;         /* 0x0079a7c0 */

/* front end / profiles */
extern int          g_frontend_checkbox_closed; /* 0x004bef9c */
extern void*        g_snd_click;            /* 0x004b92c0 */
extern int          g_cur_screen;           /* 0x0080ff84 */
extern int          g_screen_mode;          /* 0x0080ff88 */
extern int          g_vol_speech;           /* 0x0080ffc4 */
extern int          g_vol_music;            /* 0x0080ffc8 */
extern int          g_vol_sfx;              /* 0x0080ffcc */
extern unsigned char g_profile_slot;        /* 0x0080ffe3  CurProfile+0x43 */
extern int          g_pending_state;        /* 0x00832ba0 */
extern int          g_newprof_popup_up;     /* 0x007986e8 */
extern ProfileNode* g_profile_list;         /* 0x00798890 */
extern Profile      g_temp_profile;         /* 0x007cad60 */
extern unsigned int g_temp_profile_age;     /* 0x007cad80  g_temp_profile.age */
extern int          g_exit_7cb310;          /* 0x007cb310 */
extern char         g_opt_speech;           /* 0x007cb314 */
extern char         g_opt_sfx;              /* 0x007cb315 */
extern char         g_opt_music;            /* 0x007cb31c */
extern int          g_exit_popup_up;        /* 0x007cb320 */
extern int          g_save_7cb324;          /* 0x007cb324 */
extern int          g_save_is_load;         /* 0x007cb328 */

extern const char   g_msg_no_profile_dir[]; /* 0x004bf748 "Cannot Find/Open Profile Directory" */
extern const char   g_fmt_profile[];        /* 0x004bf718 "profiles\\Profile%d.txt" */
extern const char   g_msg_cannot_open[];    /* 0x004bf6fc "\ncannot open output file" */
extern const char   g_fmt_save_default[];   /* 0x004bf2e8 "%s%d" */

extern Bloke*       g_mechanic_list;        /* 0x0079a8ac (export MechanicList) */
extern PathSquare*  g_path_squares;         /* 0x0066b44c */
extern Pos          g_suggest_target;       /* 0x004bcec0  (.y at 0x004bcec4) */
extern unsigned int g_llidb_count;          /* 0x006691a4 */
extern LLElem**     g_llidb_pages;          /* 0x006691a8  256 elements per page */
extern void*        g_music_sys;            /* 0x004bf774  music engine instance */
extern int          g_music_ready;          /* 0x0079a694 */
extern IDMLoader*   g_dm_loader;            /* 0x007cacd8  IDirectMusicLoader */
extern const GUID_  CLSID_DirectMusicSegment; /* 0x004ab9f0 */
extern const GUID_  IID_IDirectMusicSegment;  /* 0x004ab670 */
extern char         g_time_text[];          /* 0x0079a878  FormatMilliseconds' buffer */
extern const char   g_fmt_hmsms[];          /* 0x004bff0c "%02d:%02d:%02d.%03d" */

/* ======================================================================== *
 *  externs
 * ======================================================================== */

extern void  MemFree(void* p);                                  /* 0x0049e4d0 */
extern void* MemAlloc(unsigned int n);                          /* 0x0049e4ff */
extern int   tolower(int c);                                    /* 0x0049ef23 (CRT) */
extern Shade* MakeShadedColour(int levels, unsigned char* rgb); /* 0x00486280 */
unsigned short* __cdecl wcscpy(unsigned short* dst, const unsigned short* src); /* 0x004a0833 */
__declspec(dllimport) int __stdcall MultiByteToWideChar(unsigned int cp, unsigned long flags,
                                                        const char* src, int srclen,
                                                        unsigned short* dst, int dstlen); /* [0x4ab0dc] */
extern void* fopen(const char* path, const char* mode);         /* 0x0049f330 */
extern int   fclose(void* f);                                   /* 0x0049efee */
extern unsigned int fwrite(const void* p, unsigned int sz,
                           unsigned int n, void* f);            /* 0x004a069e */
extern int   sprintf(char* dst, const char* fmt, ...);          /* 0x0049e573 */
extern int   printf(const char* fmt, ...);                      /* 0x0049e5c5 */

extern void  UnInitialiseBlokes(void);                          /* 0x00482ec0 */
extern void  DefaultCursor(Cursor* c);                          /* 0x0045a390 */
extern void  BuildCursorPtr(Cursor* c, int a, int b);           /* 0x0045f5f0 */
extern void  RenderCursor(Cursor* c);                           /* 0x0045ff00 */
extern void  SetCursorError(Cursor* c, int error);              /* 0x0045f480 */
extern int   PlayInstanceOfSample(void* s, int a, int b, void* src); /* 0x00496d20 */
extern void  CloseFontEndCheckBox(void);                        /* 0x0048cc10 */
extern void  UpDateCurrentProfile(void);                        /* 0x00491680 */
extern char  ScanForProfiles(void);                             /* 0x004913f0 */
extern char  LoadProfilesFormDisk(void);                        /* 0x00491470 */
extern char  SaveProfileToDisk(void);                           /* 0x00491910 */
extern int   Goto_ProfileDir(void);                             /* 0x00491360 */
extern int   ReturnFrom_ProfileDir(void);                       /* 0x004913e0 */
extern void  RemoveIconGroup(int group);                        /* 0x0046d520 */
extern int   UpdateSoundVols(void);                             /* 0x00495a90 */
extern char* GetString(int id);                                 /* 0x00498f50 */
extern void  PrintCentColref(unsigned long colour, int x, int y, int w,
                             const char* text, int font);       /* 0x00455060 */

/* Defined below (both are in this file). */
extern ProfileNode* FindProfileNodeBySlot(unsigned char slot);  /* 0x004919a0 */
extern int   SaveProfileSlotToDisk(unsigned char slot);         /* 0x004917c0 */

/* ======================================================================== *
 *  movie.c
 * ======================================================================== */

// FUNCTION: LEGOLAND 0x00476450
void Unref_00476450(int v)
{
    g_movie_fa8 = v;
}

/* ======================================================================== *
 *  workers.c / pathobj2.c
 * ======================================================================== */

/* An empty stub the wrapper below still calls -- the linker kept both. */
// FUNCTION: LEGOLAND 0x004830e0
void UnInitialiseBlokeExtras(void)
{
}

/* Tail jump into UnInitialiseBlokes; audit.py bounds the jmp. */
// FUNCTION: LEGOLAND 0x00483100
void UnInitialiseAllBlokes(void)
{
    UnInitialiseBlokeExtras();
    UnInitialiseBlokes();
}

/* Which of a ride's seat slots holds this bloke? */
// FUNCTION: LEGOLAND 0x00483110
SeatSlot* FindBlokeSeatSlot(void* bloke, SeatOwner* owner)
{
    SeatSlot* slot = owner->head;

    while (slot) {
        if (slot->bloke == bloke)
            return slot;
        slot = slot->next;
    }
    return 0;
}

/* ======================================================================== *
 *  pathmisc.c / workorder2.c
 * ======================================================================== */

/* FreePTPOpenList (0x004821e0) WITHOUT the trailing `g_ptp_open_head = 0`,
 * so it leaves the head dangling.  Reproduced. */
// FUNCTION: LEGOLAND 0x004826f0
void FreePTPOpenNodes(void)
{
    PTPNode* node = g_ptp_open_head;

    while (node) {
        PTPNode* next = node->next;
        MemFree(node);
        node = next;
    }
}

/* ======================================================================== *
 *  bnvpath.c
 * ======================================================================== */

// FUNCTION: LEGOLAND 0x00484940
int Unref_00484940(void)
{
    return 0;
}

/* ======================================================================== *
 *  tri3d.c
 * ======================================================================== */

// FUNCTION: LEGOLAND 0x00486180
void Unref_00486180(void)
{
}

/* ======================================================================== *
 *  tinystubs.c
 * ======================================================================== */

/* Opens the "IconList.txt" dump.  The flag it tests is never set anywhere in
 * the image and the FILE* it stores is never read; both globals and the
 * literal have this one reference. */
// FUNCTION: LEGOLAND 0x0048b690
void OpenIconListLog(void)
{
    if (!g_iconlist_log_open)
        g_iconlist_log = fopen(g_name_iconlist, g_mode_wplus);
}

/* ======================================================================== *
 *  narration2.c / sprite2.c
 * ======================================================================== */

/* Walks the whole sprite list and does nothing with it. */
// FUNCTION: LEGOLAND 0x004975a0
void Unref_004975a0(void)
{
    SpriteRec* s = g_sprites_head;

    while (s)
        s = s->next;
}

/* ======================================================================== *
 *  blokelist.c
 * ======================================================================== */

/* ShowLayer across every layer of a layered sprite. */
// FUNCTION: LEGOLAND 0x00497e40
void ShowAllLayers(SprObj* obj)
{
    int     i;
    SprObj* s;

    if (obj->flags & 0x8000) {
        for (i = 0; i < obj->layers->count; i++) {
            s = obj->layers->sprites[i];
            if (s)
                s->flags &= ~0x4000;
        }
    }
}

/* ======================================================================== *
 *  savechunks.c
 * ======================================================================== */

/* Links traversed from `p` to the end of the list -- one LESS than the node
 * count, and -1 for an empty list. */
// FUNCTION: LEGOLAND 0x0049c0f0
int CountListLinks(PTPNode* p)
{
    int n = -1;

    while (p) {
        p = p->next;
        n++;
    }
    return n;
}

/* ======================================================================== *
 *  profiles.c
 * ======================================================================== */

// FUNCTION: LEGOLAND 0x004919a0
ProfileNode* FindProfileNodeBySlot(unsigned char slot)
{
    ProfileNode* n = g_profile_list;

    while (n) {
        if (n->slot == slot)
            return n;
        n = n->next;
    }
    return 0;
}

/* ======================================================================== *
 *  screens2.c / screens3.c -- front-end button handlers
 * ======================================================================== */

/* The three age buttons of the register screen: stamp 7, 9 or 11 into the
 * named profile's age and write the profile back out. */
// FUNCTION: LEGOLAND 0x0048cc90
char ProfileAge7Input(Icon* p, int buttons, int a3, int a4)
{
    ProfileNode* n;

    if (g_frontend_checkbox_closed && (buttons & 2)) {
        n = FindProfileNodeBySlot(g_profile_slot);
        if (n) {
            n->p.age = 7;
            SaveProfileSlotToDisk(g_profile_slot);
        }
    }
    return 1;
}

// FUNCTION: LEGOLAND 0x0048ccd0
char ProfileAge9Input(Icon* p, int buttons, int a3, int a4)
{
    ProfileNode* n;

    if (g_frontend_checkbox_closed && (buttons & 2)) {
        n = FindProfileNodeBySlot(g_profile_slot);
        if (n) {
            n->p.age = 9;
            SaveProfileSlotToDisk(g_profile_slot);
        }
    }
    return 1;
}

// FUNCTION: LEGOLAND 0x0048cd10
char ProfileAge11Input(Icon* p, int buttons, int a3, int a4)
{
    ProfileNode* n;

    if (g_frontend_checkbox_closed && (buttons & 2)) {
        n = FindProfileNodeBySlot(g_profile_slot);
        if (n) {
            n->p.age = 11;
            SaveProfileSlotToDisk(g_profile_slot);
        }
    }
    return 1;
}

/* Just the click. */
// FUNCTION: LEGOLAND 0x0048f5d0
char ClickOnlyInput(Icon* p, int buttons, int a3, int a4)
{
    if (buttons & 2)
        PlayInstanceOfSample(g_snd_click, 0, 1, 0);
    return 1;
}

/* Click, then the options sub-screen. */
// FUNCTION: LEGOLAND 0x0048fbd0
char GotoOptionsInput(Icon* p, int buttons, int a3, int a4)
{
    if (buttons & 2) {
        PlayInstanceOfSample(g_snd_click, 0, 1, 0);
        g_screen_mode = 5;
    }
    return 1;
}

/* Options OK: keep the sliders (they are already live) and write the profile. */
// FUNCTION: LEGOLAND 0x0048ef10
char OptionsAcceptInput(Icon* p, int buttons, int a3, int a4)
{
    if (buttons & 2) {
        PlayInstanceOfSample(g_snd_click, 0, 1, 0);
        UpDateCurrentProfile();
        g_screen_mode = 1;
    }
    return 1;
}

/* Options cancel: put the three sliders back from the entry snapshot. */
// FUNCTION: LEGOLAND 0x0048ef40
char OptionsCancelInput(Icon* p, int buttons, int a3, int a4)
{
    if (buttons & 2) {
        PlayInstanceOfSample(g_snd_click, 0, 1, 0);
        g_vol_speech = g_opt_speech;
        g_vol_music  = g_opt_music;
        g_vol_sfx    = g_opt_sfx;
        g_screen_mode = 1;
    }
    return 1;
}

/* The exit pop-up's close button. */
// FUNCTION: LEGOLAND 0x0048f4f0
char ExitPopupCloseInput(Icon* p, int buttons, int a3, int a4)
{
    if (buttons & 2) {
        PlayInstanceOfSample(g_snd_click, 0, 1, 0);
        g_exit_popup_up = 0;
        CloseFontEndCheckBox();
        g_exit_7cb310 = 1;
        g_save_is_load = 0;
        g_save_7cb324 = 0;
        g_screen_mode = 4;
    }
    return 1;
}

/* Title screen "new game": clear the pending state and go to sub-screen 6. */
// FUNCTION: LEGOLAND 0x0048fe70
char TitleNewGameInput(Icon* p, int buttons, int a3, int a4)
{
    if (g_frontend_checkbox_closed && (buttons & 2)) {
        PlayInstanceOfSample(g_snd_click, 0, 1, 0);
        g_pending_state = 0;
        g_screen_mode = 6;
    }
    return 1;
}

/* ======================================================================== *
 *  listdel.c
 * ======================================================================== */

/* Rebuild the profile list from disk, complaining on screen when the profile
 * directory is not usable. */
// FUNCTION: LEGOLAND 0x00491b80
void RescanProfiles(void)
{
    int rc = ScanForProfiles();

    /* Two INDEPENDENT guards, not if/else: that is what keeps `movsx eax,al`
     * alive (one guard narrows to `cmp al,-1`).  VC6 threads the second test
     * away but the widening survives. */
    if (rc == -1)
        PrintCentColref(0xffffff, 0x140, 0x190, 0x280, g_msg_no_profile_dir, 0);
    if (rc != -1) {
        LoadProfilesFormDisk();
        g_screen_mode = 0;
    }
}

/* Copy one list node's profile record into a fresh Profile and write it to
 * that slot's "profiles\\Profile%d.txt".  Note the record is rebuilt field by
 * field over a zeroed local rather than assigned whole: Profile.f24 (+0x24)
 * and the three pad bytes after it are therefore always written as zero.
 *
 * Returns 0 when the slot has no node or the file will not open, -1 when the
 * profile directory is unusable, and 1/0 from the directory restore. */
// FUNCTION: LEGOLAND 0x004917c0
int SaveProfileSlotToDisk(unsigned char slot)
{
    Profile      prof;
    char         path[0x78];
    ProfileNode* n = FindProfileNodeBySlot(slot);
    void*        f;

    if (!n)
        return 0;

    memset(&prof, 0, sizeof(prof));
    strcpy(prof.name, n->p.name);
    prof.age   = n->p.age;
    prof.f28   = n->p.f28;
    prof.f2c   = n->p.f2c;
    prof.f30   = n->p.f30;
    prof.stats = n->p.stats;
    memcpy(prof.block, n->p.block, sizeof(prof.block));
    prof.tail  = n->p.tail;

    if (!Goto_ProfileDir())
        return -1;
    sprintf(path, g_fmt_profile, slot);
    f = fopen(path, g_mode_wplus);
    if (!f) {
        printf(g_msg_cannot_open);
        return 0;
    }
    fwrite(&prof, 0x110, 1, f);
    fclose(f);
    return ReturnFrom_ProfileDir() != 0;
}

/* ======================================================================== *
 *  tri3d.c
 * ======================================================================== */

/* The 16.16 twin of ShadeLookup (0x004864f0), which takes a float. */
// FUNCTION: LEGOLAND 0x00486520
unsigned short ShadeLookupFixed(Shade* s, int t)
{
    return s->table[t >> 16];
}

/* One shading ramp per palette entry.  The block is FOUR BYTES LARGER than
 * the 256 pointers it holds (0x404, not 0x400) -- as shipped. */
// FUNCTION: LEGOLAND 0x00486490
Shade** MakeShadedPalette(int levels, PalEntry* pal)
{
    Shade** tab = (Shade**)MemAlloc(0x404);
    int     i;

    for (i = 0; i < 256; i++)
        tab[i] = MakeShadedColour(levels, (unsigned char*)&pal[i]);
    return tab;
}

/* ======================================================================== *
 *  printlist.c
 * ======================================================================== */

/* The bounding box of a sprite object, in its own local coordinates.
 *
 * ORIGINAL BUG, reproduced: `obj->flags` is read BEFORE the `obj != 0` test,
 * so a null object faults on the flag load and never reaches the guard.
 *
 * Each layer's render offset is HALVED toward zero (the explicit
 * `-((-v) >> 1)` for negatives, not an arithmetic shift), which is what the
 * branchy neg/sar/neg pair in the original is. */
// FUNCTION: LEGOLAND 0x004855d0
void GetLayeredSpriteBounds(SprObj* obj, Rect4* r)
{
    int i;

    if (!(obj->flags & 0x8000)) {
        r->x0 = 0;
        r->y0 = 0;
        if (obj) {
            r->x1 = obj->w - 1;
            r->y1 = obj->h - 1;
        } else {
            r->x1 = 0;
            r->y1 = 0;
        }
        return;
    }
    {
        SprLayers* ls;

        r->x0 = 0x7fffffff;
        r->y0 = 0x7fffffff;
        r->x1 = (int)0x80000000;
        r->y1 = (int)0x80000000;
        i = 0;
        ls = obj->layers;
        if (ls->count > 0) {
            do {
                int     ox = ls->render_ox[i];
                int     oy = ls->render_oy[i];
                SprObj* s;
                int     x1;
                int     y1;

                if (ox < 0)
                    ox = -((-ox) >> 1);
                else
                    ox >>= 1;
                if (oy < 0)
                    oy = -((-oy) >> 1);
                else
                    oy >>= 1;
                s  = ls->sprites[i];
                x1 = s->w + ox;
                y1 = s->h + oy;
                if (ox < r->x0)
                    r->x0 = ox;
                if (oy < r->y0)
                    r->y0 = oy;
                if (x1 > r->x1)
                    r->x1 = x1;
                if (y1 > r->y1)
                    r->y1 = y1;
                ls = obj->layers;
                i++;
            } while (i < ls->count);
        }
    }
}

/* ======================================================================== *
 *  workorder2.c
 * ======================================================================== */

/* The idle mechanic (plan 0x11) nearest to the map cell `pos` -- the twin of
 * FindFreeGardener (0x00499c40). */
// FUNCTION: LEGOLAND 0x00499ca0
Bloke* FindFreeMechanic(Pos* pos)
{
    Bloke* best = 0;
    Bloke* m = g_mechanic_list;
    int    bestd = 0x7fffffff;

    while (m) {
        if (m->plan == 0x11) {
            int dx = (m->world.x >> 8) - pos->x;
            int dy = (m->world.y >> 8) - pos->y;
            int d = dy * dy + dx * dx;
            if (d < bestd) {
                bestd = d;
                best = m;
            }
        }
        m = m->next;
    }
    return best;
}

/* ======================================================================== *
 *  levelkw.c
 * ======================================================================== */

/* Lower-case a word in place; returns its length.  Instruction-for-
 * instruction the twin of UpcaseString (0x00499300) with tolower. */
// FUNCTION: LEGOLAND 0x00499340
int DowncaseString(char* s)
{
    int i = 0;

    while ((s[i] = (char)tolower(s[i])) != 0)
        i++;
    return i;
}

/* ======================================================================== *
 *  util.c
 * ======================================================================== */

/* Format a millisecond count as "hh:mm:ss.mmm" in a shared static buffer. */
// FUNCTION: LEGOLAND 0x00499490
char* FormatMilliseconds(int ms)
{
    sprintf(g_time_text, g_fmt_hmsms,
            ms / 3600000, (ms / 60000) % 60, (ms / 1000) % 60, ms % 1000);
    return g_time_text;
}

/* ======================================================================== *
 *  savechunks.c
 * ======================================================================== */

/* Step `n` links along a list.  -1 answers 0; 0 answers the node itself. */
// FUNCTION: LEGOLAND 0x0049c110
PTPNode* StepListNodes(PTPNode* p, unsigned int n)
{
    if (n == 0xffffffff)
        return 0;
    while (n-- != 0)
        p = p->next;
    return p;
}

/* ======================================================================== *
 *  text.c -- path string helpers
 *
 *  All five share one backwards scan from the terminator: walk down until a
 *  separator is met, remembering (or acting on) the last '.'.  The scan tests
 *  the character FIRST and the index second, so index 0 is always examined.
 * ======================================================================== */

/* Split "a\b\name.ext" into `dir` = "a\b" and `base` = "name".  With no
 * backslash `dir` comes back empty and `base` is the whole stem. */
// FUNCTION: LEGOLAND 0x00499040
void SplitPathAndBase(const char* path, char* dir, char* base)
{
    int dot = strlen(path);
    int i = dot;
    int cut;

    while (path[i] != '\\') {
        if (i <= 0)
            break;
        if (path[i] == '.')
            dot = i;
        i--;
    }
    cut = i;
    for (i = cut + 1; i < dot; i++)
        *base++ = path[i];
    *base = 0;
    for (i = 0; i < cut; i++)
        *dir++ = path[i];
    *dir = 0;
}

/* Give `path` an extension only if it has none. */
// FUNCTION: LEGOLAND 0x004990c0
void AppendExtensionIfNone(char* path, const char* ext)
{
    int i = strlen(path);

    while (path[i] != '.') {
        if (path[i] == '\\')
            break;
        if (i <= 0)
            break;
        i--;
    }
    if (path[i] != '.')
        strcat(path, ext);
}

/* Replace `path`'s extension (or append one when it has none). */
// FUNCTION: LEGOLAND 0x00499120
void ReplaceExtension(char* path, const char* ext)
{
    int i = strlen(path);

    while (path[i] != '.') {
        if (path[i] == '\\')
            break;
        if (i <= 0)
            break;
        i--;
    }
    if (path[i] == '.')
        path[i] = 0;
    strcat(path, ext);
}

/* Prefix `path` with `prefix` only when it carries no directory or drive at
 * all -- the scan must reach index 0 without meeting '\' or ':'. */
// FUNCTION: LEGOLAND 0x00499190
void PrefixPathIfBare(char* path, const char* prefix)
{
    char tmp[200];
    int  i = strlen(path);

    while (path[i] != '\\') {
        if (path[i] == ':')
            break;
        if (i <= 0)
            break;
        i--;
    }
    if (i == 0) {
        strcpy(tmp, path);
        strcpy(path, prefix);
        strcat(path, tmp);
    }
}

/* Replace `path`'s directory with `prefix`, keeping only the part after the
 * last '\' or ':'. */
// FUNCTION: LEGOLAND 0x00499240
void PrependPathPrefix(char* path, const char* prefix)
{
    char tmp[200];
    int  i = strlen(path);

    while (path[i] != '\\') {
        if (path[i] == ':')
            break;
        if (i <= 0)
            break;
        i--;
    }
    if (i != 0)
        i++;
    strcpy(tmp, prefix);
    strcat(tmp, path + i);
    strcpy(path, tmp);
}

/* ======================================================================== *
 *  music.c -- two more DirectMusic wrappers
 * ======================================================================== */

/* LoadMusicSegment (0x00495e30) with a NAME as well as a filename: both
 * strings are widened and dwValidData carries DMUS_OBJ_NAME on top. */
// FUNCTION: LEGOLAND 0x00495f00
int LoadNamedMusicSegment(const char* file, const char* name, void** out)
{
    DMObjectDesc   desc;
    unsigned short wfile[0x200];
    unsigned short wname[0x200];
    long           hr;

    if (!g_music_sys)
        return 0;
    if (!g_music_ready)
        return 0;
    *out = 0;
    MultiByteToWideChar(0, 0, file, -1, wfile, 0x200);
    MultiByteToWideChar(0, 0, name, -1, wname, 0x200);
    desc.dwSize = sizeof(DMObjectDesc);
    desc.guidClass = CLSID_DirectMusicSegment;
    wcscpy(desc.wszFileName, wfile);
    wcscpy(desc.wszName, wname);
    desc.dwValidData = DMUS_OBJ_CLASS | DMUS_OBJ_NAME | DMUS_OBJ_FILENAME;
    hr = g_dm_loader->lpVtbl->GetObject(g_dm_loader, &desc, &IID_IDirectMusicSegment, out);
    return hr >= 0;
}

/* The chord-map twin of GetMusicBand (0x00496090): IDirectMusicStyle's
 * GetChordMap is the vtable slot at +0x28. */
// FUNCTION: LEGOLAND 0x00496010
int GetMusicChordMap(const char* name, IDMStyle* style, void** chordmap)
{
    unsigned short wname[0x200];
    long           hr;

    if (!g_music_sys)
        return 0;
    if (!g_music_ready)
        return 0;
    *chordmap = 0;
    MultiByteToWideChar(0, 0, name, -1, wname, 0x200);
    hr = style->lpVtbl->GetChordMap(style, wname, chordmap);
    return hr >= 0;
}

/* ======================================================================== *
 *  screens3.c / profiles.c -- the new-profile popup
 * ======================================================================== */

/* The register screen's age spinner: control 2 counts DOWN (floor 1), any
 * other control counts UP (ceiling 100).  Like the volume rows it answers 2
 * rather than 1 while the button is down or held. */
// FUNCTION: LEGOLAND 0x00491f90
char ProfileAgeSpinInput(Icon* p, int buttons, int a3, int a4)
{
    if (buttons & 2) {
        if (p->u18.level == 2) {
            if (g_temp_profile.age >= 2)
                g_temp_profile.age--;
        } else if (g_temp_profile.age <= 0x63) {
            g_temp_profile.age++;
        }
        return 1;
    }
    if (buttons & 1)
        return 2;
    /* Two plain `return 2` guards: VC6 folds the pair into
     * `test al,4 / setne al / inc eax`.  Any arithmetic spelling of the same
     * value ((x&4)!=0)+1 lowers to shr/and/inc instead. */
    if (buttons & 4)
        return 2;
    return 1;
}

/* The new-profile popup's OK button.  With no name typed yet it fills the
 * temp profile in with the default name plus the slot number ("%s%d" over
 * string 0x8d) and stays put; with a name it commits the profile and drops
 * back to the top of the front end.
 *
 * g_temp_profile.name[0x1e] carries the name's LENGTH -- the last usable byte
 * of the 0x20-byte field doubles as the caret position for the on-screen
 * keyboard. */
// FUNCTION: LEGOLAND 0x00491fe0
char NewProfileOkInput(Icon* p, int buttons, int a3, int a4)
{
    char buf[16];

    if (buttons & 2) {
        if (g_temp_profile.name[0] == 0) {
            sprintf(buf, g_fmt_save_default, GetString(0x8d), g_profile_slot);
            g_temp_profile.name[0x1e] = (char)strlen(buf);
            strcpy(g_temp_profile.name, buf);
        } else {
            SaveProfileToDisk();
            RemoveIconGroup(0x15);
            CloseFontEndCheckBox();
            g_newprof_popup_up = 0;
            g_cur_screen = -1;
            UpdateSoundVols();
        }
    }
    return 1;
}

/* ======================================================================== *
 *  coaster.c -- two more of the memory module's UNOPTIMISED wrappers
 *
 *  Like FreeMemScratch / MemScratch_Noop / MemScratchInit (coaster.c,
 *  schoolcar.c) these two keep a full ebp frame with every local in memory,
 *  which /O2 never emits: the memory module was built without optimisation.
 * ======================================================================== */

/* The 0x00668fb8 block: a header followed by 0x8c-byte nodes, with a free
 * list at +0x04 and a live list at +0x08 threaded through each node's +0x00. */
typedef struct MemScratchPool {
    int count;      /* +0x00  nodes the block already has room for */
    int freelist;   /* +0x04  head of the free chain */
    int used;       /* +0x08  head of the live chain */
} MemScratchPool;

extern MemScratchPool* g_mem_scratch;              /* 0x00668fb8 */
extern void* realloc(void* p, unsigned int size);  /* 0x0049fca2 */

#pragma optimize("", off)

/* Take one node off the free list and push it on the live list, growing the
 * block by 0x400 nodes (and relocating the live chain through the pointer
 * delta realloc left behind) when the free list has run dry. */
/* NOTE ON THE LOCAL NAMES.  At /Od VC6 assigns ebp slots by a hash of the
 * local's NAME -- neither declaration order nor first-use order moves them
 * (all six permutations of the declarations give identical offsets).  These
 * six names are the set that reproduces the original's frame:
 *   idx -4, front -8, was -0xc, larger -0x10, march -0x14, skew -0x18. */
// FUNCTION: LEGOLAND 0x00477440
int MemScratch_TakeNode(void)
{
    int             idx;      /* node index while threading the free list */
    int             front;    /* base of the freshly added node array */
    int             was;      /* the old live-chain head */
    MemScratchPool* larger;   /* the reallocated block */
    int*            march;    /* walker over the live chain */
    int             skew;     /* the byte delta realloc moved the block by */

    if (g_mem_scratch == 0)
        return 0;
    if (g_mem_scratch->freelist == 0) {
        larger = (MemScratchPool*)realloc(g_mem_scratch,
                                         (g_mem_scratch->count + 0x400) * 0x8c + 0xc);
        if (larger == 0)
            return 0;
        skew = (char*)larger - (char*)g_mem_scratch;
        larger->used = larger->used + skew;
        march = (int*)larger->used;
        while (*march != 0) {
            *march = *march + skew;
            march = (int*)*march;
        }
        g_mem_scratch = larger;
        front = (int)g_mem_scratch + g_mem_scratch->count * 0x8c + 0xc;
        memset((void*)front, 0, 0x400 * 0x8c);
        g_mem_scratch->freelist = front;
        for (idx = 0; idx < 0x3ff; idx++)
            *(int*)(g_mem_scratch->freelist + idx * 0x8c) =
                g_mem_scratch->freelist + (idx + 1) * 0x8c;
    }
    was = g_mem_scratch->used;
    g_mem_scratch->used = g_mem_scratch->freelist;
    g_mem_scratch->freelist = *(int*)g_mem_scratch->freelist;
    *(int*)g_mem_scratch->used = was;
    return g_mem_scratch->used;
}

/* Total the live chain: `*bytes` gets the sum of every node's +0x08 word and
 * `*count` the node count.  Answers 0 for a null out-pointer. */
// FUNCTION: LEGOLAND 0x00477600
int MemScratch_Totals(int* bytes, int* count)
{
    int  amount;
    int* node;
    int  nodes;

    amount = 0;
    nodes = 0;
    if (bytes == 0 || count == 0)
        return 0;
    if (g_mem_scratch != 0) {
        node = (int*)g_mem_scratch->used;
        while (node != 0) {
            amount = amount + node[2];
            nodes = nodes + 1;
            node = (int*)*node;
        }
    }
    *bytes = amount;
    *count = nodes;
    return 1;
}

#pragma optimize("", on)

/* ======================================================================== *
 *  memdb.c -- LLIDB element table
 * ======================================================================== */

/* Delete element `idx` from the paged LLIDB table, closing the gap.  Within a
 * page the tail shifts down one slot; across a page boundary the next page's
 * first element is carried up into the previous page's LAST slot (255) and
 * that page then shifts from 0.  -3 for an index past the end.
 *
 * `slot == -1` is the "shift this whole page from the start" marker the outer
 * loop leaves behind, and `slot >= 0xfe` (the last slot of a page) means
 * there is nothing left to move inside it. */
// FUNCTION: LEGOLAND 0x0047b500
int LLIDB_RemoveElement(unsigned int idx)
{
    unsigned int page;
    int          slot;
    int          j;

    if (idx < g_llidb_count) {
        page = idx >> 8;
        slot = idx & 0xff;
        for (; page <= g_llidb_count >> 8; page++) {
            if (slot == -1) {
                g_llidb_pages[page - 1][255] = g_llidb_pages[page][0];
                slot = 0;
            }
            if (slot < 0xfe) {
                for (j = slot; j < 0xfe; j++)
                    g_llidb_pages[page][j] = g_llidb_pages[page][j + 1];
            }
            slot = -1;
        }
        g_llidb_count--;
        return 0;
    }
    return -3;
}

/* ======================================================================== *
 *  pathsq.c
 * ======================================================================== */

/* Draw a build cursor over every path square, then one more over the
 * suggested target cell.  The per-square cursors use style flag 4; the final
 * one uses 8 and an EMPTY footprint rect, so it is the bare cell marker. */
// FUNCTION: LEGOLAND 0x00481d70
void RenderPathSquareCursors(void)
{
    Cursor      cur;
    PathSquare* sq;

    sq = g_path_squares;
    DefaultCursor(&cur);
    for (; sq; sq = sq->next) {
        cur.rect = sq->rect;
        cur.flags = 4;
        cur.origin.x = 0;
        cur.origin.y = 0;
        BuildCursorPtr(&cur, 0, 0);
        RenderCursor(&cur);
    }
    /* A sub-object memset, not five field stores: it is what gives the
     * function its SECOND zero register (eax beside ebp). */
    memset(&cur.rect, 0, sizeof(cur.rect));
    cur.flags = 8;
    SetCursorError(&cur, 0);
    cur.origin.x = g_suggest_target.x;
    cur.origin.y = g_suggest_target.y;
    BuildCursorPtr(&cur, 0, 0);
    RenderCursor(&cur);
}

/* ======================================================================== *
 *  tri3d.c -- a standalone texture sampler
 *
 *  HAND-WRITTEN ASSEMBLY, not compiled C.  The tells: an EBP frame in an /O2
 *  file, a 32x32->64 `mul` with `shrd eax,edx,10h` (VC6's C 64-bit shift goes
 *  through __aullshr -- it never emits shrd), bit 16 tested straight into the
 *  carry flag (`shr ecx,11h / sbb eax,0`, the only such site in the image
 *  outside the CRT), and a result parked in a dword stack slot that the C
 *  epilogue then reads back as a WORD.
 * ======================================================================== */

/* The texture descriptor as THIS routine reads it: +0x00/+0x04 are used as
 * multiplicands (extent-1), not as the log2 shifts the rasterisers take them
 * for, and +0x08 is the row shift. */
typedef struct TexDesc {
    unsigned int    w;        /* +0x00 */
    unsigned int    h;        /* +0x04 */
    int             shift;    /* +0x08 */
    unsigned char*  texels;   /* +0x0c */
    Shade**         ramps;    /* +0x10 */
} TexDesc;

extern TexDesc* g_texture;   /* 0x0066b630 */

/* Sample the current texture at 16.16 (u,v) with a 16.16 shade term, in the
 * surface's own pixel format.  u and v are taken modulo 1.0 by the & 0ffffh
 * and scaled by extent-1; shade indexes the 64-level ramp as shade >> 10,
 * with 1.0 exactly (bit 16 set) folded back onto level 63 by the borrow. */
// FUNCTION: LEGOLAND 0x00488730
unsigned short SampleTexturePixel(unsigned int u, unsigned int v, unsigned int shade)
{
    unsigned int px;

#ifndef LEGOLAND_PORTABLE
    __asm {
        mov   ebx, g_texture
        mov   eax, [ebx]
        dec   eax
        mov   ecx, u
        and   ecx, 0ffffh
        mul   ecx
        shrd  eax, edx, 10h
        mov   ecx, eax
        mov   eax, [ebx+4]
        dec   eax
        mov   edx, v
        and   edx, 0ffffh
        mul   edx
        shrd  eax, edx, 10h
        mov   edx, ecx
        mov   ecx, [ebx+8]
        shl   eax, cl
        add   eax, edx
        mov   edx, [ebx+0ch]
        mov   ebx, [ebx+10h]
        movzx edx, byte ptr [edx+eax]
        mov   ebx, [ebx+edx*4]
        mov   ebx, [ebx+4]
        mov   eax, shade
        mov   ecx, eax
        shr   ecx, 11h
        sbb   eax, 0
        shr   eax, 0ah
        movzx eax, word ptr [ebx+eax*2]
        mov   px, eax
    }
#else
    {
        TexDesc* ll_t  = g_texture;
        unsigned ll_cu = (unsigned)(((unsigned long long)(ll_t->w - 1) * (u & 0xffffu)) >> 16);
        unsigned ll_cv = (unsigned)(((unsigned long long)(ll_t->h - 1) * (v & 0xffffu)) >> 16);
        unsigned ll_ix = (ll_cv << ll_t->shift) + ll_cu;
        Shade*   ll_rp = ll_t->ramps[ll_t->texels[ll_ix]];
        /* shr ecx,11h / sbb eax,0: 1.0 exactly (bit 16) folds back onto level 63 */
        px = ll_rp->table[(shade - ((shade >> 17) & 1)) >> 10];
    }
#endif
    return (unsigned short)px;
}
