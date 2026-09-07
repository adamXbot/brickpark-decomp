/* LEGOLAND — front-end helpers: profile / saved-game / option screen glue,
 * the free-play unlock table, the marked-tile counters and the text printer
 * that returns where its text ends (scope AJ).
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Only
 * struct field offsets, callee argument counts and global addresses are
 * load-bearing; names are ours. Types are defined LOCALLY on purpose
 * (legoland.h is owned elsewhere). Extern prototype TYPES are caller-side
 * codegen levers and are declared the way THIS file's bodies need them.
 */
#include <string.h>

#pragma intrinsic(strlen, strcpy, memset)

/* ---- CRT / allocator ------------------------------------------------------ */
extern void* MemAlloc(unsigned int size);                    /* 0x0049e4ff */
extern void  MemFree(void* p);                               /* 0x0049e4d0 */
extern int   _stricmp(const char*, const char*);             /* 0x004aab90 */

/* ---- local types ---------------------------------------------------------- */

typedef struct Pos { int x, y; } Pos;

/* The marked-tile counter table @ 0x007cb3e0 (pathmisc.c): 128 entries of
 * {key = (x << 8) + y, count}; an unused entry has key 0xffff. */
typedef struct MarkedTile { unsigned short key, count; } MarkedTile;
extern MarkedTile g_marked_tiles[128];                       /* 0x007cb3e0 */

/* A loaded LLIDB element (movie3.c's DBElem). */
typedef struct DBElem {
    const char* name;    /* +0x00 */
    void*       image;   /* +0x04 */
    int         flags;   /* +0x08 */
    void*       data;    /* +0x0c */
} DBElem;

/* An object class on the class chain @ 0x00669240 (link at +0x00, the
 * instance list at +0x04, the LLIDB element at +0xc4). */
typedef struct InstNode { struct InstNode* next; } InstNode;
typedef struct ObjClass {
    struct ObjClass* next;          /* +0x00 */
    InstNode*        instances;     /* +0x04 */
    char             pad08[0xc4 - 0x08];
    DBElem*          elem;          /* +0xc4 */
} ObjClass;
extern ObjClass* g_objcls_head;                              /* 0x00669240 */

/* The free-play object table @ 0x004bdeb8 (fpui2.c's FPTableEntry): 16-byte
 * rows {id byte, LLIDB class name, ..}, terminated by an empty name. The id
 * indexes the current profile's 200-byte "unlocked" block @ 0x0080ffe6. */
typedef struct FPTableEntry {
    unsigned char id;      /* +0x00 */
    char          pad01[3];
    char*         name;    /* +0x04 */
    int           f08;     /* +0x08 */
    int           f0c;     /* +0x0c */
} FPTableEntry;
extern FPTableEntry  g_fp_table[0x86];                       /* 0x004bdeb8 */
extern unsigned char g_profile_unlocked[200];                /* 0x0080ffe6 */

/* A front-end icon (screens2.c's Icon); only the fields touched here. */
typedef struct Icon {
    char           pad00[0xc];
    short          x;         /* +0x0c */
    short          y;         /* +0x0e */
    char           pad10[0x2c - 0x10];
    char         (*input)(struct Icon*, int);      /* +0x2c */
    int            f30;       /* +0x30 */
    unsigned int   flags;     /* +0x34 */
    char*          help;      /* +0x38  GetString(help_id) */
    int            help_id;   /* +0x3c */
} Icon;
typedef char (*IconInputFn)(Icon*, int);
typedef void Sprite;

/* Win32 RECT, passed BY VALUE to the text printers (text.c's WinRect). */
typedef struct WinRect {
    long left;     /* +0x00 */
    long top;      /* +0x04 */
    long right;    /* +0x08 */
    long bottom;   /* +0x0c */
} WinRect;

typedef struct DDSurface DDSurface;
typedef struct DDSurfaceVtbl {
    char pad00[0x44];                                       /* +0x00 */
    long(__stdcall* GetDC)(DDSurface*, void** hdc);         /* +0x44 */
    char pad48[0x68 - 0x48];                                /* +0x48 */
    long(__stdcall* ReleaseDC)(DDSurface*, void* hdc);      /* +0x68 */
} DDSurfaceVtbl;
struct DDSurface { DDSurfaceVtbl* vtbl; };

/* ---- the profile record (profiles.c, 0x110 bytes) ------------------------- */
#pragma pack(push, 1)
typedef struct Profile {
    char           name[0x1e];   /* +0x00 */
    unsigned char  len;          /* +0x1e  name length (the name editors) */
    unsigned char  f1f;          /* +0x1f */
    int            f20;          /* +0x20 */
    unsigned char  f24;          /* +0x24 */
    char           rest[0x110 - 0x25];
} Profile;
#pragma pack(pop)

extern Profile g_temp_profile;                               /* 0x007cad60 */

/* A saved-game / profile list node (profiles.c's ProfileNode, 0x11c). */
typedef struct ProfileNode {
    struct ProfileNode* next;    /* +0x000 */
    Profile             p;       /* +0x004 */
    int                 valid;   /* +0x114 */
    unsigned char       slot;    /* +0x118 */
} ProfileNode;

extern ProfileNode* g_savedgame_list;                        /* 0x00798734 */

/* The live profile @ 0x0080ffa0 (profiles.c's CurProfile); the bytes read
 * here are named individually as screens3.c does. */
extern unsigned char g_have_profile;                         /* 0x0080ffd9  CurProfile+0x39 */
extern unsigned char g_cur_save_slot;                        /* 0x0080ffe4  CurProfile+0x44 */

/* ---- the three front-end state blocks (movie.c) --------------------------- */
typedef struct SavedUiState { int icons2_mode; } SavedUiState;
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

extern int           g_icons2_mode;                          /* 0x00668e38 */
extern FrontEndState g_front;                                /* 0x0080ff80 */
extern EditState     g_edit;                                 /* 0x008119b0 */
extern int           g_cur_screen;                           /* 0x0080ff84 */

/* ---- globals -------------------------------------------------------------- */
extern IconInputFn g_icon_handler1;                          /* 0x006687bc */
extern IconInputFn g_icon_handler2;                          /* 0x006687c0 */
extern IconInputFn g_sg_saved_handler1;                      /* 0x007986f8 */
extern IconInputFn g_sg_saved_handler2;                      /* 0x007986f4 */
extern IconInputFn g_opt_saved_handler1;                     /* 0x00798740 */
extern IconInputFn g_opt_saved_handler2;                     /* 0x0079873c */

extern Sprite* g_fe_sprite_674;       /* 0x00798674 PU_OKON.lls */
extern Sprite* g_fe_sprite_678;       /* 0x00798678 PU_OK.lls */
extern Sprite* g_fe_sprite_67c;       /* 0x0079867c RegClose.lls */
extern Sprite* g_fe_sprite_680;       /* 0x00798680 RegCloseON.lls */
extern Sprite* g_fe_sprite_684;       /* 0x00798684 PU_ClosePopUp.lls */
extern Sprite* g_fe_sprite_688;       /* 0x00798688 PU_ClosePopUpON.lls */
extern Icon*   g_np_icon_ok;          /* 0x007986d8 popup OK icon */
extern Icon*   g_np_close_icon;       /* 0x007986dc popup close icon */
extern int     g_delete_popup_up;     /* 0x007986e4 Reg_Delete_PopUp showing */
extern int     g_7986f0;              /* 0x007986f0 */

extern Sprite* g_lp_off1;             /* 0x00798694 RegProfileOff_1.lls */
extern Sprite* g_lp_off2;             /* 0x00798698 */
extern Sprite* g_lp_off3;             /* 0x0079869c */
extern Sprite* g_lp_off4;             /* 0x007986a0 */
extern Sprite* g_lp_off5;             /* 0x007986a4 */
extern Sprite* g_lp_off6;             /* 0x007986a8 */
extern Sprite* g_lp_off7;             /* 0x007986ac */
extern Sprite* g_lp_off8;             /* 0x007986b0 RegProfileOff_8.lls */

extern DDSurface* g_draw_surface;     /* 0x0066807c */
extern WinRect    g_clip_rect;        /* 0x004bdea0 */

/* Sprite names (.rdata; only the addresses are load-bearing). */
extern const char g_lls_pu_ok[];              /* 0x004baa70 "PU_OK.lls" */
extern const char g_lls_pu_ok_on[];           /* 0x004baa64 "PU_OKON.lls" */
extern const char g_lls_regclose[];           /* 0x004bf148 "RegClose.lls" */
extern const char g_lls_regclose_on[];        /* 0x004bf138 "RegCloseON.lls" */
extern const char g_lls_pu_close[];           /* 0x004bab78 "PU_ClosePopUp.lls" */
extern const char g_lls_pu_close_on[];        /* 0x004bab8c "PU_ClosePopUpON.lls" */

/* ---- callees -------------------------------------------------------------- */
extern Sprite* LoadSprite(const char* name, int mode);                 /* 0x00497ab0 */
extern Icon*   InsertIcon(short x, short y, unsigned short group, Sprite* s); /* 0x0046d6c0 */
extern void    SetIconSprite(Icon* p, Sprite* s);                       /* 0x0046d680 */
extern char*   GetString(int id);                                       /* 0x00498f50 */
extern char    UpDateCurrentProfile(void);                              /* 0x00491680 */
extern void    CloseFontEndCheckBox(void);                              /* 0x0048cc10 */
extern void    RemoveSaveGame(unsigned char slot);                      /* 0x0048ea10 */
extern char    ProfileCloseInput(Icon*, int);                           /* 0x0048d450 */
extern void*   SelectFont(void* dc, int font);                          /* 0x00454b40 */
extern void    PushRenderingStatusAndUnlockVideoSurface(void);          /* 0x00464080 */
extern void    PopRenderingStatus(void);                                /* 0x004641f0 */

/* ---- imports -------------------------------------------------------------- */
__declspec(dllimport) void*         __stdcall CreateRectRgn(int l, int t, int r, int b); /* [0x4ab0b8] */
__declspec(dllimport) void*         __stdcall SelectObject(void* dc, void* obj);         /* [0x4ab080] */
__declspec(dllimport) int           __stdcall SetBkMode(void* dc, int mode);             /* [0x4ab074] */
__declspec(dllimport) unsigned long __stdcall SetTextColor(void* dc, unsigned long c);   /* [0x4ab0b0] */
__declspec(dllimport) int           __stdcall DrawTextA(void* dc, const char* s, int n,
                                                        WinRect* rc, unsigned int fmt);  /* [0x4ab2ac] */
__declspec(dllimport) int           __stdcall DeleteObject(void* obj);                   /* [0x4ab09c] */

/* Defined below (the delete popup's OK handler, installed by InitSaveDeletePopUp). */
char SaveDeleteOkInput(Icon* icon, int msg);                            /* 0x0048e450 */

/* =========================================================================
 *  Marked-tile counters (pathmisc.c owns the table)
 * ========================================================================= */

/* Bumps the counter of the marked tile at `pos`; 1 if it was in the table. */
// FUNCTION: LEGOLAND 0x00489f90
int BumpSlotCounter(Pos* pos)
{
    int i = 0;
    unsigned short key = (unsigned short)((pos->x << 8) + pos->y);

    for (; i < 128; i++) {
        if (g_marked_tiles[i].key == key) {
            g_marked_tiles[i].count++;
            return 1;
        }
    }
    return 0;
}

/* The counter of the marked tile at `pos`, 0 if it is not in the table. */
// FUNCTION: LEGOLAND 0x00489fd0
unsigned short GetRideVisitCountAt(Pos* pos)
{
    int i = 0;
    unsigned short key = (unsigned short)((pos->x << 8) + pos->y);

    for (; i < 128; i++) {
        if (g_marked_tiles[i].key == key)
            return g_marked_tiles[i].count;
    }
    return 0;
}

/* =========================================================================
 *  Object classes
 * ========================================================================= */

/* Frees every class's instance list. */
// FUNCTION: LEGOLAND 0x0048a040
void FreeClassInstanceLists(void)
{
    ObjClass* c;
    InstNode* n;
    InstNode* next;

    for (c = g_objcls_head; c; c = c->next) {
        for (n = c->instances; n; n = next) {
            next = n->next;
            MemFree(n);
        }
        c->instances = 0;
    }
}

/* =========================================================================
 *  Free-play unlock table
 * ========================================================================= */

/* Finds the element's row in the free-play table by name and marks it
 * unlocked in the current profile (writing the profile back) if it was not. */
// FUNCTION: LEGOLAND 0x0048a6e0
void UnlockFreePlayEntry(DBElem* e)
{
    FPTableEntry* t;

    for (t = g_fp_table; strlen(t->name) != 0; t++) {
        if (_stricmp(e->name, t->name) == 0) {
            if (g_profile_unlocked[t->id] == 0) {
                g_profile_unlocked[t->id] = 1;
                UpDateCurrentProfile();
            }
            return;
        }
    }
}

/* Unlocks, for free play, every class whose element has flag 2 clear. */
// FUNCTION: LEGOLAND 0x0048a750
void UnlockClassesForFreePlay(void)
{
    ObjClass* c;
    DBElem*   e;

    for (c = g_objcls_head; c; c = c->next) {
        e = c->elem;
        if (!(e->flags & 2))
            UnlockFreePlayEntry(e);
    }
}

/* Empty routine taking the profile's 200-byte block (saveprof.c names it). */
// FUNCTION: LEGOLAND 0x0048a780
void InitProfileBlock(char* block)
{
}

/* Clears the +0x0c word of every free-play table row. */
// FUNCTION: LEGOLAND 0x0048a800
void ResetFreePlayTable(void)
{
    FPTableEntry* t;

    for (t = g_fp_table; strlen(t->name) != 0; t++)
        t->f0c = 0;
}

/* =========================================================================
 *  Profile list screen
 * ========================================================================= */

/* RegProfileOff_<slot>.lls for slot 1..8. */
// FUNCTION: LEGOLAND 0x0048c5e0
Sprite* GetProfileOffSprite(char slot)
{
    switch (slot) {
    case 1: return g_lp_off1;
    case 2: return g_lp_off2;
    case 3: return g_lp_off3;
    case 4: return g_lp_off4;
    case 5: return g_lp_off5;
    case 6: return g_lp_off6;
    case 7: return g_lp_off7;
    case 8: return g_lp_off8;
    }
    return 0;
}

/* OK + close icons (group 0xe) for the saved-game delete popup, 0x42 left /
 * 0x18 above the popup panel (screens2.c's InitProfileCheckBoxIcons twin). */
// FUNCTION: LEGOLAND 0x0048c860
void InitSaveDeletePopUp(Icon* popup)
{
    g_fe_sprite_678 = LoadSprite(g_lls_pu_ok, 4);
    g_fe_sprite_674 = LoadSprite(g_lls_pu_ok_on, 4);
    g_fe_sprite_67c = LoadSprite(g_lls_regclose, 4);
    g_fe_sprite_680 = LoadSprite(g_lls_regclose_on, 4);
    g_fe_sprite_684 = LoadSprite(g_lls_pu_close, 4);
    g_fe_sprite_688 = LoadSprite(g_lls_pu_close_on, 4);
    g_np_icon_ok = InsertIcon(popup->x - 0x42, popup->y - 0x18, 0xe, g_fe_sprite_678);
    g_np_icon_ok->help_id = 5;
    g_np_icon_ok->help = GetString(5);
    g_np_icon_ok->flags |= 0x2000;
    g_np_icon_ok->flags |= 0x4002;
    g_np_icon_ok->input = SaveDeleteOkInput;
    g_np_close_icon = InsertIcon(g_np_icon_ok->x + 0x24, g_np_icon_ok->y, 0xe, g_fe_sprite_67c);
    g_np_close_icon->help_id = 4;
    g_np_close_icon->help = GetString(4);
    g_np_close_icon->flags |= 0x2000;
    g_np_close_icon->flags |= 0x4002;
    g_np_close_icon->input = ProfileCloseInput;
}

/* =========================================================================
 *  Saved-game screen
 * ========================================================================= */

/* Stash the two icon handlers before the saved-game screen replaces them. */
// FUNCTION: LEGOLAND 0x0048d470
void SaveSavedGameIconHandlers(void)
{
    g_sg_saved_handler1 = g_icon_handler1;
    g_sg_saved_handler2 = g_icon_handler2;
}

/* Put them back. */
// FUNCTION: LEGOLAND 0x0048d490
void RestoreSavedGameIconHandlers(void)
{
    g_icon_handler1 = g_sg_saved_handler1;
    g_icon_handler2 = g_sg_saved_handler2;
}

/* Pushes a node for save `slot` onto the saved-game list; with valid != 0
 * the header's name and +0x24 byte are copied in (profiles.c's
 * AddNodeToProfileList twin). */
// FUNCTION: LEGOLAND 0x0048e0c0
void AddNodeToSavedGameList(int valid, Profile* hdr, char slot)
{
    ProfileNode* node = (ProfileNode*)MemAlloc(sizeof(ProfileNode));

    memset(node, 0, sizeof(ProfileNode));
    if (valid) {
        node->valid = 1;
        strcpy(node->p.name, hdr->name);
        node->slot = slot;
        node->p.f24 = hdr->f24;
        node->next = g_savedgame_list;
        g_savedgame_list = node;
    } else {
        node->valid = 0;
        node->slot = slot;
        node->next = g_savedgame_list;
        g_savedgame_list = node;
    }
}

/* Seeds the saved-game name editor with an existing name. */
// FUNCTION: LEGOLAND 0x0048e3d0
void SetTempProfileName(const char* name)
{
    strcpy(g_temp_profile.name, name);
    g_temp_profile.len = (unsigned char)strlen(name);
    g_7986f0 = 1;
}

/* Puts the OK / close popup icons back to their un-lit sprites. */
// FUNCTION: LEGOLAND 0x0048e420
void ResetSavePopupIcons(void)
{
    SetIconSprite(g_np_icon_ok, g_fe_sprite_678);
    SetIconSprite(g_np_close_icon, g_fe_sprite_67c);
}

/* The delete popup's OK icon: on a click with a save slot selected, close
 * the popup, delete that slot's files and rebuild the screen. */
// FUNCTION: LEGOLAND 0x0048e450
char SaveDeleteOkInput(Icon* icon, int msg)
{
    if ((msg & 2) && g_cur_save_slot) {
        CloseFontEndCheckBox();
        g_delete_popup_up = 0;
        RemoveSaveGame(g_cur_save_slot);
        g_cur_screen = -1;
        g_cur_save_slot = 0;
    }
    return 1;
}

/* =========================================================================
 *  Option screen
 * ========================================================================= */

/* Volume marker x -> volume 0..100 (the slider spans 241 pixels from 0x7c). */
// FUNCTION: LEGOLAND 0x0048eac0
int MarkerXToVolume(int x)
{
    return (x - 0x7c) * 100 / 241;
}

/* Volume 0..100 -> marker x. */
// FUNCTION: LEGOLAND 0x0048eaf0
int VolumeToMarkerX(int vol)
{
    return vol * 241 / 100 + 0x7c;
}

/* Stash the two icon handlers before the option screen replaces them. */
// FUNCTION: LEGOLAND 0x0048eb20
void SaveOptionIconHandlers(void)
{
    g_opt_saved_handler1 = g_icon_handler1;
    g_opt_saved_handler2 = g_icon_handler2;
}

/* Put them back. */
// FUNCTION: LEGOLAND 0x0048eb40
void RestoreOptionIconHandlers(void)
{
    g_icon_handler1 = g_opt_saved_handler1;
    g_icon_handler2 = g_opt_saved_handler2;
}

/* =========================================================================
 *  Title screen
 * ========================================================================= */

/* Push the front-end state the title screen's movie will clobber (movie.c's
 * RestoreFrontEndState is the pop). */
// FUNCTION: LEGOLAND 0x0048f9f0
void SaveFrontEndState(SavedUiState* ui, FrontEndState* screen, EditState* game)
{
    ui->icons2_mode = g_icons2_mode;
    *game = g_edit;
    *screen = g_front;
}

/* Non-zero when a profile is selected. */
// FUNCTION: LEGOLAND 0x0048fc30
int HaveCurrentProfile(void)
{
    return g_have_profile != 0;
}

/* Non-zero when the temp profile has a name. */
// FUNCTION: LEGOLAND 0x00491540
int TempProfileHasName(void)
{
    return g_temp_profile.name[0] != 0;
}

/* =========================================================================
 *  Text
 * ========================================================================= */

/* text.c's NewPrintCent with the text measured first: the centred,
 * vertically-centred single line is drawn into `rc` and the x at which it
 * ends (box centre + half the measured width) is returned so a cursor can
 * follow the text being typed. */
// FUNCTION: LEGOLAND 0x00491e40
int PrintTextGetEnd(const char* text, int font, WinRect rc, char white)
{
    void*   hdc;
    void*   rgn;
    void*   oldrgn;
    void*   oldfont;
    WinRect calc = rc;

    rgn = CreateRectRgn(g_clip_rect.left, g_clip_rect.top,
                        g_clip_rect.right, g_clip_rect.bottom);
    PushRenderingStatusAndUnlockVideoSurface();
    g_draw_surface->vtbl->GetDC(g_draw_surface, &hdc);
    SetBkMode(hdc, 1);
    if (white == 1)
        SetTextColor(hdc, 0xffffff);
    oldrgn = SelectObject(hdc, rgn);
    oldfont = SelectFont(hdc, font);
    DrawTextA(hdc, text, strlen(text), &calc, 0x425);  /* DT_CALCRECT|DT_VCENTER|DT_SINGLELINE|DT_CENTER */
    DrawTextA(hdc, text, strlen(text), &rc, 0x25);     /* DT_VCENTER|DT_SINGLELINE|DT_CENTER */
    SelectObject(hdc, oldfont);
    SelectObject(hdc, oldrgn);
    g_draw_surface->vtbl->ReleaseDC(g_draw_surface, hdc);
    PopRenderingStatus();
    DeleteObject(rgn);
    return (rc.left + rc.right) / 2 + (calc.right - calc.left) / 2;
}
