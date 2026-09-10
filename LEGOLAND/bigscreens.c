/* LEGOLAND — the progress, saved-game and profile front-end screens plus
 * the in-game interface set-up.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Only struct
 * field offsets, callee argument counts and global addresses are load-bearing;
 * names are ours. Types are defined LOCALLY on purpose (legoland.h is owned
 * elsewhere; the Icon record is the same 0x40-byte one iconui.c/screens2.c
 * use).
 *
 * Screen layouts recovered here:
 *
 *  Progress screen (InitProgressScreen, group 0x23 buttons + group 0x1c level
 *    markers): backdrop Progress_ScreenBK.lls; Accept_on_Progress at
 *    (0x20e,0x16f), GoBack_on_Progress at (0x208,0xb) and, unless the game
 *    is restarting a level, Tutorial_On_Progress at (0x174,0x16d). One marker
 *    per level 6..15 from the ten-entry table @ 0x004beb88 ({string id, x, y,
 *    lit sprite, dim sprite, lit name, dim name}): the current level gets the
 *    flashing lit marker, earlier / completed levels the dim one. Level <= 5
 *    skips the screen (0x0048bde0), level > 15 drops straight into the
 *    end-of-game screen (mode 8, alternate icon set).
 */
#include "legoland.h"

/* ---- the icon record (iconui.c) ---------------------------------------- */
typedef struct Icon {
    struct Icon*   next;      /* +0x00 */
    Sprite*        sprite;    /* +0x04 */
    void*          data;      /* +0x08 */
    short          x;         /* +0x0c */
    short          y;         /* +0x0e */
    short          w;         /* +0x10 */
    short          h;         /* +0x12 */
    unsigned short group;     /* +0x14 */
    short          pad16;     /* +0x16 */
    union {
        void*         owner;  /* +0x18  profile list icons: the Profile record */
        unsigned char level;  /* +0x18  progress screen: level number */
        unsigned char ctrl;   /* +0x18  in-game control icons: control number */
    } u18;
    union {
        unsigned char slot;   /* +0x1c  profile slot number */
        Sprite*       pressed;/* +0x1c  in-game control icons: pressed sprite */
    } u1c;
    union {
        unsigned char flags20;/* +0x20  bit 0: profile slot icon, 1/2: save type */
        Sprite*       normal; /* +0x20  in-game control icons: normal sprite */
    } u20;
    char           pad24[4];  /* +0x24 */
    int          (*render)(struct Icon*);          /* +0x28 */
    char         (*input)(struct Icon*, int);      /* +0x2c */
    void*          widget;    /* +0x30 */
    unsigned int   flags;     /* +0x34 */
    char*          help;      /* +0x38  GetString(help_id) */
    int            help_id;   /* +0x3c */
} Icon;

typedef char (*IconInputFn)(Icon*, int);

/* The map header as this file sees it: the current level lives at +0x28. */
typedef struct LevelMap {
    char  pad[0x10];   /* +0x00 */
    short f10;         /* +0x10  view width (0x280) */
    char  pad12[0xe];  /* +0x12 */
    short f20;         /* +0x20 */
    char  pad22[6];    /* +0x22 */
    int   level;       /* +0x28 */
} LevelMap;

extern LevelMap* g_level_map;         /* 0x004bcbf4 — the same object as g_map */

/* ---- the live profile (profiles.c's CurProfile) as this file sees it ----- */
#pragma pack(push, 1)
typedef struct CurProfile {
    char           name[0x20];      /* +0x00  0x0080ffa0 */
    int            f20;             /* +0x20 */
    int            f24;             /* +0x24 */
    int            f28;             /* +0x28 */
    int            f2c;             /* +0x2c */
    int            tail;            /* +0x30 */
    unsigned char  level_done[15];  /* +0x34  0x0080ffd4  one byte per level 1..15 */
    unsigned char  profile_slot;    /* +0x43  0x0080ffe3 */
    unsigned char  save_slot;       /* +0x44  0x0080ffe4  current save slot (PrintSavedGameDetails
                                     *        reads it as a dword through a cast: a union here
                                     *        would be 4 bytes wide and push f45 to +0x48) */
    unsigned char  f45;             /* +0x45  0x0080ffe5  save type (1 normal, 2 free) */
    char           block[200];      /* +0x46 */
} CurProfile;
#pragma pack(pop)

extern CurProfile g_cur_profile;      /* 0x0080ffa0 */

/* ---- the progress-screen level marker table @ 0x004beb88 --------------- */
typedef struct LevelMarker {
    int         str_id;    /* +0x00 help string */
    int         x;         /* +0x04 */
    int         y;         /* +0x08 */
    Sprite*     lit;       /* +0x0c */
    Sprite*     dim;       /* +0x10 */
    const char* lit_name;  /* +0x14 */
    const char* dim_name;  /* +0x18 */
} LevelMarker;             /* 0x1c */

extern LevelMarker g_level_markers[10];   /* 0x004beb88 .. 0x004beca0 */

/* ---- globals ------------------------------------------------------------ */
extern int     g_progress_resume;     /* 0x00798660 */
extern int     g_progress_798664;     /* 0x00798664 */
extern int     g_progress_798668;     /* 0x00798668 */
extern int     g_last_level;          /* 0x007cb394 */
extern int     g_pending_state;       /* 0x00832ba0 */
extern unsigned int g_ui_flags;       /* 0x00813a40 */
extern int     g_icons2_mode;         /* 0x00668e38 */
extern Sprite* g_backdrop;            /* 0x00810148 */
extern int     g_game_mode;           /* 0x008119b4 */
extern int     g_cur_screen;          /* 0x0080ff84 */
extern int     g_screen_mode;         /* 0x0080ff88 */
extern IconInputFn g_icon_handler1;   /* 0x006687bc */
extern IconInputFn g_icon_handler2;   /* 0x006687c0 */

/* Sprite names (.rdata; only the addresses are load-bearing). */
extern const char g_lls_progress_bk[];        /* 0x004bef44 "Progress_ScreenBK.lls" */
extern const char g_lls_accept_on_progress[]; /* 0x004bef2c "Accept_on_Progress.lls" */
extern const char g_lls_goback_on_progress[]; /* 0x004bef14 "GoBack_on_Progress.lls" */
extern const char g_lls_tutorial_on_progress[]; /* 0x004beef8 "Tutorial_On_Progress.lls" */

/* ---- callees ------------------------------------------------------------ */
extern Sprite* LoadSprite(const char* name, int mode);                 /* 0x00497ab0 */
extern Icon*   InsertIcon(short x, short y, unsigned short group, Sprite* s); /* 0x0046d6c0 */
extern Icon*   LoadSpriteIcon(const char* name, int mode, int x, int y, int group); /* 0x0046d7b0 */
extern void    RemoveIconGroup(int group);                              /* 0x0046d520 */
extern char*   GetString(int id);                                       /* 0x00498f50 */
extern int     RenderFlashingSpriteIcon(Icon*);                         /* 0x0046e8a0 */
#ifndef LEGOLAND_PORTABLE
extern void    UpDateCurrentProfile(void);                              /* 0x00491680 */
#else
extern int UpDateCurrentProfile(void);                              /* 0x00491680 */
#endif

/* Progress-screen helpers (not exported). */
extern void    NormaliseLevelsDone(void);        /* 0x0048b6d0 */
extern void    LoadLevelMarkerSprites(void);     /* 0x0048b700 loads the table's sprites */
extern void    ReferenceLevelMarkerSprites(void);            /* 0x0048b740 */
extern void    SkipProgressScreen(void);         /* 0x0048bde0 level <= 5 */
extern char    ProgressLevelInput(Icon*, int);   /* 0x0048bb60 */
extern char    ProgressAcceptInput(Icon*, int);  /* 0x0048bc20 */
extern char    ProgressGoBackInput(Icon*, int);  /* 0x0048c020 */
extern char    ProgressTutorialInput(Icon*, int);/* 0x0048c090 */

/* ---- the profile / saved-game list node (profiles.c) ------------------- */
typedef struct Profile {
    char           name[0x20];   /* +0x00 */
    int            f20;          /* +0x20 */
    unsigned char  f24;          /* +0x24  save type: 1 = normal game */
    char           rest[0x110 - 0x25];
} Profile;

typedef struct ProfileNode {
    struct ProfileNode* next;    /* +0x000 */
    Profile             p;       /* +0x004 */
    int                 valid;   /* +0x114 */
    unsigned char       slot;    /* +0x118 */
} ProfileNode;

extern ProfileNode* g_savedgame_list;  /* 0x00798734 */

/* The mouse-hit record {type, object} @ 0x004bdd00 (rin.c's HitInfo). */
typedef struct HitInfo {
    int   type;    /* +0x00  2 = an icon */
    void* obj;     /* +0x04 */
} HitInfo;

extern HitInfo g_hit_info;            /* 0x004bdd00 */
extern Pos     g_gfx_point;           /* 0x00813a44 mouse point */

/* Front-end icon list and the profile / save screen state (screens2.c). */
extern Icon*   g_side_icons;          /* 0x006687c8 icon list head */
extern Icon*   g_delete_icon;         /* 0x007cb360 the RegDelete icon */
extern char    g_lp_title[];          /* 0x007cb340 GetString(0x84) copy */
extern Icon*   g_accept_icon;         /* 0x007986e0 Accept_On_Reg / Accept_On_Save */
extern int     g_delete_popup_up;     /* 0x007986e4 delete popup showing */
extern int     g_newprof_popup_up;    /* 0x007986e8 new-profile popup showing */
extern int     g_7986f0;              /* 0x007986f0 */
extern int     g_7986e4;              /* probe only */
extern int     g_newsave_popup_up;    /* 0x00798700 new-save popup showing */
extern Sprite* g_lp_delete_on;        /* 0x0079868c RegDeleteOn.lls */
extern Sprite* g_lp_delete;           /* 0x00798690 RegDelete.lls */
extern Sprite* g_lp_delete_popup;     /* 0x007986b8 Reg_Delete_PopUp.lls / RegSaveDouble_PopUp.lls */
extern Sprite* g_lp_diff_popup;       /* 0x007986bc Reg_Diff_PopUp.lls */
extern Sprite* g_save_slot_on;        /* 0x00798704 RegSaveSlotOn.lls */
extern Sprite* g_savebk[8];           /* 0x00798708 RegSaveOff_1..8.lls */
extern Sprite* g_save_type_normal;    /* 0x00798728 SaveType_Normal.lls */
extern Sprite* g_save_type_free;      /* 0x0079872c SaveType_Free.lls */
extern Sprite* g_save_corner_mask;    /* 0x00798730 RegCornerMask.lls */
extern int     g_exit_7cb310;         /* 0x007cb310 */
extern int     g_save_7cb324;         /* 0x007cb324 */
extern int     g_save_7cb328;         /* 0x007cb328 non-zero: loading, not saving */

extern const char g_lls_saved_game_screen[]; /* 0x004bf2a0 "Saved_Game_Screen.lls" */
extern const char g_lls_save_slot_on[];      /* 0x004bf28c "RegSaveSlotOn.lls" */
extern const char g_lls_save_off1[];         /* 0x004bf278 "RegSaveOff_1.lls" */
extern const char g_lls_save_off2[];         /* 0x004bf264 */
extern const char g_lls_save_off3[];         /* 0x004bf250 */
extern const char g_lls_save_off4[];         /* 0x004bf23c */
extern const char g_lls_save_off5[];         /* 0x004bf228 */
extern const char g_lls_save_off6[];         /* 0x004bf214 */
extern const char g_lls_save_off7[];         /* 0x004bf200 */
extern const char g_lls_save_off8[];         /* 0x004bf1ec "RegSaveOff_8.lls" */
extern const char g_lls_save_type_normal[];  /* 0x004bf1d8 "SaveType_Normal.lls" */
extern const char g_lls_save_type_free[];    /* 0x004bf1c4 "SaveType_Free.lls" */
extern const char g_lls_reg_delete_on2[];    /* 0x004bf1b4 "RegDeleteON.lls" */
extern const char g_lls_reg_delete[];        /* 0x004bf104 "RegDelete.lls" */
extern const char g_lls_save_double_popup[]; /* 0x004bf19c "RegSaveDouble_PopUp.lls" */
extern const char g_lls_corner_mask[];       /* 0x004bf188 "RegCornerMask.lls" */
extern const char g_lls_goback_on_savedgame[]; /* 0x004bf170 "GoBack_on_SavedGame.lls" */
extern const char g_lls_accept_on_save[];    /* 0x004bf15c "Accept_On_Save.lls" */
extern const char g_str_empty_slot[];        /* 0x004befa0 "EMPTY" */

extern void    SetIconSprite(Icon* p, Sprite* s);                       /* 0x0046d680 */
extern void    LightUpthisDeleteIcon(Icon* slot, int lit);              /* 0x0048cd50 */
extern void    UpdateProfileCheckBoxIcons(void);                        /* 0x0048ce20 */
extern void    EnterNewProfile(Icon* panel);                            /* 0x00491bd0 */
extern void    DeleteSavedGameList(void);                               /* 0x0048e160 */
#ifndef LEGOLAND_PORTABLE
extern void    LoadSavedGamesList(char slot);                           /* 0x0048e190 */
#else
extern int LoadSavedGamesList(char slot);                           /* 0x0048e190 */
#endif
extern Sprite* GetSavePanelBK(char slot);                               /* 0x0048db50 */
extern char    DeleteIconInput(Icon*, int);                             /* 0x0048cc30 */
/* 0x00455e50 (not exported): the cached text blitter (money.c). */
extern void    PrintCachedText(const char* text, int x, int y, int w, int h,
                               int f1, int f2, int ink, int paper);
/* 0x0048c5e0 (not exported): RegProfileOff_<slot>.lls for slot 1..8. */
extern Sprite* GetProfileOffSprite(char slot);
/* 0x00491540 (not exported): non-zero when the temp profile has a name. */
extern int     TempProfileHasName(void);
/* 0x0048d470 (not exported). */
extern void    SavedGame_48d470(void);
/* Saved-game screen icon input handlers (other lanes). */
extern char    FreePlayGoBackInput(Icon*, int);     /* 0x0048fb80 */
extern char    SaveGoBackInput(Icon*, int);     /* 0x0048db10 */
extern char    SaveAcceptInput(Icon*, int);     /* 0x0048f5a0 */
extern char    SaveScreenAcceptInput(Icon*, int);     /* 0x0048da50 */
extern char    LoadAcceptInput(Icon*, int);     /* 0x0048d970 */
extern char    SaveSlotInput(Icon*, int);              /* 0x0048e4a0 */
extern char    SaveEmptySlotInput(Icon*, int);         /* 0x0048e4f0 */

/* =========================================================================
 *  Progress screen
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x0048b7e0
void InitProgressScreen(void)
{
    if (g_level_map->level < 1)
        g_level_map->level = 1;
    if (g_progress_resume == 0) {
        NormaliseLevelsDone();
        if (g_pending_state == 0) {
            if (g_progress_798668 == 0)
                g_level_map->level = g_last_level + 1;
        }
        if (g_pending_state == 1) {
            if (g_level_map->level <= 15) {
                g_cur_profile.level_done[g_level_map->level - 1] = 1;
                UpDateCurrentProfile();
            }
        }
    }
    if (g_level_map->level <= 5) {
        SkipProgressScreen();
        return;
    }
    if (g_level_map->level > 15) {
        g_icons2_mode = 1;
        g_backdrop = 0;
        g_game_mode = 2;
        g_cur_screen = -1;
        g_screen_mode = 8;
        g_ui_flags &= ~0x20;
        return;
    }
    {
    Icon* p;
    int i;
    int id;

    g_progress_798664 = 0;
    g_backdrop = LoadSprite(g_lls_progress_bk, 4);
    if (g_progress_resume == 0) {
        if (g_pending_state == 1 && g_level_map->level == 6)
            g_pending_state = 2;
        LoadLevelMarkerSprites();
        p = LoadSpriteIcon(g_lls_accept_on_progress, 4, 0x20e, 0x16f, 0x23);
        p->help_id = 0x262;
        p->help = GetString(0x262);
        p->flags |= 0x6002;
        p->input = ProgressAcceptInput;
        p = LoadSpriteIcon(g_lls_goback_on_progress, 4, 0x208, 0xb, 0x23);
        p->help_id = 0x26;
        p->help = GetString(0x26);
        p->flags |= 0x6002;
        p->input = ProgressGoBackInput;
        if (g_pending_state != 1) {
            p = LoadSpriteIcon(g_lls_tutorial_on_progress, 4, 0x174, 0x16d, 0x23);
            p->help_id = 0x258;
            p->help = GetString(0x258);
            p->flags |= 0x6002;
            p->input = ProgressTutorialInput;
        }
    }
    ReferenceLevelMarkerSprites();
    RemoveIconGroup(0x1c);
    if (g_pending_state == 1) {
        for (i = 0; i < 10; i++) {
            if (i + 5 == g_level_map->level - 1) {
                p = InsertIcon(g_level_markers[i].x, g_level_markers[i].y, 0x1c, g_level_markers[i].lit);
                if (p) {
                    p->render = RenderFlashingSpriteIcon;
                    id = g_level_markers[i].str_id;
                    p->help_id = id;
                    p->help = GetString(id);
                    p->u18.level = i + 5;
                    p->input = ProgressLevelInput;
                    p->flags |= 0x200a;
                }
            } else if (i + 5 < g_level_map->level - 1) {
                p = InsertIcon(g_level_markers[i].x, g_level_markers[i].y, 0x1c, g_level_markers[i].dim);
                if (p) {
                    id = g_level_markers[i].str_id;
                    p->help_id = id;
                    p->help = GetString(id);
                    p->flags |= 0x2000;
                }
            }
        }
    } else {
        for (i = 0; i < 10; i++) {
            if (i + 5 == g_level_map->level - 1) {
                p = InsertIcon(g_level_markers[i].x, g_level_markers[i].y, 0x1c, g_level_markers[i].lit);
                if (p) {
                    p->render = RenderFlashingSpriteIcon;
                    id = g_level_markers[i].str_id;
                    p->help_id = id;
                    p->help = GetString(id);
                    p->u18.level = i + 5;
                    p->input = ProgressLevelInput;
                    p->flags |= 0x600a;
                }
            } else if (g_cur_profile.level_done[i + 5] == 1) {
                p = InsertIcon(g_level_markers[i].x, g_level_markers[i].y, 0x1c, g_level_markers[i].dim);
                if (p) {
                    id = g_level_markers[i].str_id;
                    p->help_id = id;
                    p->help = GetString(id);
                    p->u18.level = i + 5;
                    p->input = ProgressLevelInput;
                    p->flags |= 0x6002;
                }
            }
        }
    }
    g_icon_handler1 = ProgressAcceptInput;
    g_icon_handler2 = ProgressGoBackInput;
    }
}

/* =========================================================================
 *  Profile list screen: per-frame details
 * ========================================================================= */

/* Draws the profile screen text: the title, then for every profile slot icon
 * the slot's name (white on the lit slot, black elsewhere) and, while a popup
 * is up, its prompt (0x85 delete / 0x86 new name). The lit slot's icon is
 * swapped for the popup panel and pulled up 0x1b pixels; the name editor runs
 * inside the new-profile popup. Also records a mouse hit on the strip left of
 * a slot icon and enables/disables the Accept icon. */
/* A print box, {left, top, right, bottom}. */
typedef struct IRect {
    int l;
    int t;
    int r;
    int b;
} IRect;

// FUNCTION: LEGOLAND 0x0048cf10
void PrintProfileDetails(void)
{
    Icon* p = g_side_icons;
    struct {
        void* owner;
        Icon* cur;
    } v;
    int   x;
    int   y;
    int   r;
    int   b;
    IRect rc;

    g_delete_icon->flags |= 0x400;
    y = 0x72;
    PrintCachedText(g_lp_title, 0x8d, y, 0xf0, 0x2e, 3, 0x25, 0xffffff, 0);
    if (p) {
        int  lit;
        char white;
        do {
            lit = 1;
            white = 1;
            if (!(p->flags & 0x400) && (p->u20.flags20 & 1)) {
                if (g_gfx_point.x >= p->x - 0x18 && g_gfx_point.x < p->x &&
                    g_gfx_point.y >= p->u1c.slot * 38 + 0x86 && g_gfx_point.y < p->y + p->w) {
                    g_hit_info.type = 2;
                    g_hit_info.obj = p;
                }
                if (g_cur_profile.profile_slot == p->u1c.slot) {
                    if (g_delete_popup_up) {
                        SetIconSprite(p, g_lp_delete_popup);
                        v.cur = p;
                        p->y = p->u1c.slot * 38 + 0x6b;
                        y = p->y + 0x22;
                    } else if (g_newprof_popup_up) {
                        EnterNewProfile(p);
                        lit = 0;
                        v.cur = p;
                    } else {
                        SetIconSprite(p, g_lp_diff_popup);
                        p->y = p->u1c.slot * 38 + 0x6b;
                        y = p->y + 0x22;
                        LightUpthisDeleteIcon(p, 1);
                        v.cur = p;
                    }
                } else {
                    SetIconSprite(p, GetProfileOffSprite(p->u1c.slot));
                    white = 0;
                    p->y = p->u1c.slot * 38 + 0x86;
                    y = p->y + 7;
                }
                b = y + 0x13;
                x = p->x + 0x14;
                r = x + 0xe0;
                v.owner = p->u18.owner;
                if (v.owner && lit) {
                    if (!(g_cur_profile.profile_slot - 1 == p->u1c.slot &&
                          (g_delete_popup_up || g_newprof_popup_up))) {
                        if (white)
                            PrintCachedText(v.owner, x, y, r - x, b - y, 2, 0x25, 0, 0xffffff);
                        else
                            PrintCachedText(v.owner, x, y, r - x, b - y, 2, 0x25, 0xffffff, 0);
                    }
                }
            }
            p = p->next;
        } while (p);
    }
    if (g_delete_popup_up) {
        rc.l = v.cur->x + 0x14;
        rc.t = v.cur->y + 7;
        rc.r = rc.l + 0x9b;
        rc.b = rc.t + 0x13;
        PrintCachedText(GetString(0x85), rc.l, rc.t, rc.r - rc.l, rc.b - rc.t, 2, 0x25, 0, 0xffffff);
        UpdateProfileCheckBoxIcons();
    } else if (g_newprof_popup_up) {
        rc.l = v.cur->x + 0x14;
        rc.t = v.cur->y - 0x14;
        rc.r = rc.l + 0xe0;
        rc.b = rc.t + 0x13;
        PrintCachedText(GetString(0x86), rc.l, rc.t, rc.r - rc.l, rc.b - rc.t, 2, 0x25, 0, 0xffffff);
        UpdateProfileCheckBoxIcons();
    }
    if (g_cur_profile.profile_slot != 0 && g_delete_popup_up == 0) {
        if (g_newprof_popup_up) {
            if (TempProfileHasName())
                g_accept_icon->flags &= ~0x400;
            else
                g_accept_icon->flags |= 0x400;
        } else {
            g_accept_icon->flags &= ~0x400;
        }
    } else {
        g_accept_icon->flags |= 0x400;
    }
}

/* =========================================================================
 *  Saved-game screen
 * ========================================================================= */

/* Backdrop Saved_Game_Screen.lls plus the slot-on / eight slot-off panels,
 * the two save-type stamps, the delete icon pair, the double-save popup and
 * the corner mask. GoBack_on_SavedGame at (0x1c2,0x13) and Accept_On_Save at
 * (0x1dc,0x142) get handlers by mode (save vs load, 0x7cb328; exit-to-save,
 * 0x7cb324 / 0x7cb310). One panel per saved-game slot at (0x51, 0x6f + 38*slot):
 * a real save shows its type (normal = bit 1, free play = bit 2), an empty
 * slot the "EMPTY" text. The RegDelete icon is parked at (0,0). */
// FUNCTION: LEGOLAND 0x0048d4b0
void InitSavedGameScreen(void)
{
    Icon* p;
    ProfileNode* node;

    g_backdrop = LoadSprite(g_lls_saved_game_screen, 0);
    g_cur_profile.save_slot = 0;
    g_save_slot_on = LoadSprite(g_lls_save_slot_on, 4);
    g_savebk[0] = LoadSprite(g_lls_save_off1, 4);
    g_savebk[1] = LoadSprite(g_lls_save_off2, 4);
    g_savebk[2] = LoadSprite(g_lls_save_off3, 4);
    g_savebk[3] = LoadSprite(g_lls_save_off4, 4);
    g_savebk[4] = LoadSprite(g_lls_save_off5, 4);
    g_savebk[5] = LoadSprite(g_lls_save_off6, 4);
    g_savebk[6] = LoadSprite(g_lls_save_off7, 4);
    g_savebk[7] = LoadSprite(g_lls_save_off8, 4);
    g_save_type_normal = LoadSprite(g_lls_save_type_normal, 4);
    g_save_type_free = LoadSprite(g_lls_save_type_free, 4);
    g_lp_delete_on = LoadSprite(g_lls_reg_delete_on2, 4);
    g_lp_delete = LoadSprite(g_lls_reg_delete, 4);
    g_lp_delete_popup = LoadSprite(g_lls_save_double_popup, 4);
    g_save_corner_mask = LoadSprite(g_lls_corner_mask, 4);
    p = LoadSpriteIcon(g_lls_goback_on_savedgame, 4, 0x1c2, 0x13, 7);
    p->flags |= 0x6002;
    if (g_save_7cb324) {
        p->input = FreePlayGoBackInput;
        p->help_id = 0x26;
        p->help = GetString(0x26);
    } else {
        p->input = SaveGoBackInput;
        p->help_id = 0x28;
        p->help = GetString(0x28);
    }
    g_icon_handler2 = p->input;
    if (g_save_7cb328 == 0) {
        g_accept_icon = LoadSpriteIcon(g_lls_accept_on_save, 4, 0x1dc, 0x142, 7);
        g_accept_icon->flags |= 0x2000;
        g_accept_icon->flags |= 0x4002;
        g_accept_icon->flags |= 0x400;
        if (g_exit_7cb310) {
            g_accept_icon->input = SaveAcceptInput;
            g_accept_icon->help_id = 0x29;
            g_accept_icon->help = GetString(0x29);
        } else {
            g_accept_icon->input = SaveScreenAcceptInput;
            g_accept_icon->help_id = 0x2a;
            g_accept_icon->help = GetString(0x2a);
        }
    } else {
        g_accept_icon = LoadSpriteIcon(g_lls_accept_on_save, 4, 0x1dc, 0x142, 7);
        g_accept_icon->help_id = 0x2b;
        g_accept_icon->help = GetString(0x2b);
        g_accept_icon->flags |= 0x2000;
        g_accept_icon->flags |= 0x4002;
        g_accept_icon->flags |= 0x400;
        g_accept_icon->input = LoadAcceptInput;
    }
    g_icon_handler1 = g_accept_icon->input;
    SavedGame_48d470();
    DeleteSavedGameList();
    LoadSavedGamesList(g_cur_profile.profile_slot);
    node = g_savedgame_list;
    while (node) {
        if (node->valid) {
            p = InsertIcon(0x51, node->slot * 38 + 0x6f, 7, GetSavePanelBK(node->slot));
            p->input = SaveSlotInput;
            p->flags |= 0x4002;
            p->u18.owner = &node->p;
            p->u1c.slot = node->slot;
            if (node->p.f24 == 1) {
                p->u20.flags20 |= 3;
                p->help_id = 0x2c;
                p->help = GetString(0x2c);
            } else {
                p->u20.flags20 |= 5;
                p->help_id = 0x2d;
                p->help = GetString(0x2d);
            }
        } else {
            p = InsertIcon(0x51, node->slot * 38 + 0x6f, 7, GetSavePanelBK(node->slot));
            p->help_id = 0x2e;
            p->help = GetString(0x2e);
            p->flags |= 0x6002;
            p->input = SaveEmptySlotInput;
            p->u18.owner = (void*)g_str_empty_slot;
            p->u1c.slot = node->slot;
            p->u20.flags20 |= 1;
        }
        node = node->next;
    }
    g_delete_icon = InsertIcon(0, 0, 7, g_lp_delete);
    g_delete_icon->help_id = 5;
    g_delete_icon->help = GetString(5);
    g_delete_icon->flags |= 0x2000;
    g_delete_icon->flags |= 0x4002;
    g_delete_icon->flags |= 0x400;
    g_delete_icon->input = DeleteIconInput;
}

/* =========================================================================
 *  Saved-game screen: per-frame details
 * ========================================================================= */

/* Win32 RECT, passed BY VALUE to the text printers (text.c's WinRect). */
typedef struct WinRect {
    long left;     /* +0x00 */
    long top;      /* +0x04 */
    long right;    /* +0x08 */
    long bottom;   /* +0x0c */
} WinRect;

/* The 5th PrintSprite argument (money.c's BlitCtx). */
typedef struct BlitCtx {
    int kind;                            /* +0x00 */
    struct { Icon* p; int n; } owner;    /* +0x04, +0x08 */
} BlitCtx;

extern void NewPrintCent(const char* text, int font, WinRect rc, char white); /* 0x00491d60 */
extern int  PrintSprite(Sprite* s, int x, int y, int mode, BlitCtx* ctx);     /* 0x004853a0 */
extern void EnterSaveGameDetails(Icon* panel);                                /* 0x0048e550 */

/* Draws the saved-game screen text: the "save"/"load" heading (0x4ba /
 * 0x4b0) and the profile name in the top box, then every slot panel's name
 * and save-type stamp. The current slot's panel becomes the popup while a
 * popup is up (delete popup: pulled up a row; new-save popup: the name
 * editor with the pending save type stamped), else the lit slot panel. The
 * popup prompt (0x8c / 0xc44 new save, 0x85 delete) is printed under a
 * corner mask. The Accept icon follows whether a slot is selected.
 *
 * CLOSED 2026-09-03 (lane bigscreens, ~200 measured variants in
 * scratchpad/bigscreens/q*.py) after sitting at 3 mismatches for several
 * rounds: the header's 0x14e landed in ebp instead of ebx.  The cause was
 * never the header or the loop but a CONSTANT-CSE web: the popup tails'
 * `push 2` (the NewPrintCent font) pairs with any one of the loop's 2s
 * (`push 2`, `flags20 & 2`, `f45 == 2`); VC6 materialises that shared
 * constant at the loop preheader, which is the same basic block as the
 * header, and the phantom web claims ebx ahead of the header's crossing
 * values (it is later spilled in the loop and rematerialised as immediates,
 * so it leaves no code).  Proofs: tail font 3, or all three loop 2s
 * changed, give `mov ebx,14Eh`; a tail G(rc, K) demotes ebx exactly for
 * the K already used as a non-lea immediate in the join block or the loop
 * (2, 4, 7, 0x28, 0x1400, 0x24, 0x38, 0x56, 0x5c, 0x6a, 0x14e, 0x4ba) and
 * never for 0/1, lea-folded addends or constants first used after the
 * loop; a partner placed before the guard's `je`, inside the guard body,
 * in a second guarded block ahead of the header, or after the loop does
 * not demote; any branch between the header and the loop (a guarded
 * store, `if (g_side_icons)`, `if (p != (Icon*)1)`) un-demotes but costs
 * its own instructions.  Use counts, the constant's spelling (2u, 2L,
 * '\002', (short)2, 2.0, 1+1, sizeof, ternary), propagated `int font =
 * 2` locals, dead `= 2` initialisers, lit's type or scope, callee return
 * and parameter types, WinRect field types, an inlined loop or header
 * helper, goto/for/do loop forms and tautological conditions all leave
 * ebp.
 *
 * What closes it is the DEGENERATE BRANCH below: the delete icon cached in
 * `d`, the guard on `d`, and the header printed in both arms of
 * `if (!d) ... else ...`.  VC6 merges the identical arms late (after the
 * constant web has been placed in the then-arm's block, away from the loop
 * preheader) and the negated form leaves no ghost load or test; `if (d)`
 * with the same arms, the guard's store folded into either arm, the global
 * instead of `d`, or only one of the two calls duplicated all fail (37, 1,
 * 6 mismatches).  Semantics are unchanged: both arms print the same header.
 * Lever, for the playbook: a repeated non-trivial constant whose partner
 * use is in a loop and whose other use is after the loop is a phantom
 * callee-saved web born in the loop preheader; when the preheader is the
 * block holding call-crossing header values it takes ebx from them. */
// FUNCTION: LEGOLAND 0x0048dd00
void PrintSavedGameDetails(void)
{
    Icon* p = g_side_icons;
    Icon* cur;
    void* owner;
    WinRect rc;
    int sy;
    int ty;
    Icon* d = g_delete_icon;

    if (d)
        d->flags |= 0x400;
    /* Degenerate branch, both arms identical -- see the note above: VC6
     * merges them late and that is what keeps the shared constant 2 out of
     * ebx during the header. Do not "simplify". */
    if (!d) {
        rc.left = 0x5c;
        rc.top = 0x24;
        rc.right = 0x14e;
        rc.bottom = 0x56;
        NewPrintCent(GetString(g_save_7cb328 ? 0x4b0 : 0x4ba), 0, rc, 1);
        rc.top = 0x38;
        rc.bottom = 0x6a;
        NewPrintCent(g_cur_profile.name, 1, rc, 1);
    } else {
        rc.left = 0x5c;
        rc.top = 0x24;
        rc.right = 0x14e;
        rc.bottom = 0x56;
        NewPrintCent(GetString(g_save_7cb328 ? 0x4b0 : 0x4ba), 0, rc, 1);
        rc.top = 0x38;
        rc.bottom = 0x6a;
        NewPrintCent(g_cur_profile.name, 1, rc, 1);
    }
    while (p) {
        int lit = 1;
        if (!(p->flags & 0x1400) && (p->u20.flags20 & 1)) {
            if (g_cur_profile.save_slot == p->u1c.slot) {
                cur = p;
                if (g_delete_popup_up) {
                    SetIconSprite(p, g_lp_delete_popup);
                    p->y = p->u1c.slot * 38 + 0x54;
                    sy = p->y + 0x1b;
                    ty = p->y + 0x22;
                    LightUpthisDeleteIcon(p, 0);
                } else if (g_newsave_popup_up) {
                    SetIconSprite(p, g_lp_delete_popup);
                    p->y = p->u1c.slot * 38 + 0x54;
                    ty = p->y + 0x22;
                    sy = p->y;
                    EnterSaveGameDetails(p);
                    lit = 0;
                    if (g_cur_profile.f45 == 1)
                        PrintSprite(g_save_type_normal, p->x + 7, sy + 0x21, 0, 0);
                    if (g_cur_profile.f45 == 2)
                        PrintSprite(g_save_type_free, p->x + 7, sy + 0x21, 0, 0);
                    goto name;
                } else {
                    SetIconSprite(p, g_save_slot_on);
                    p->y = p->u1c.slot * 38 + 0x6f;
                    ty = p->y + 7;
                    sy = p->y;
                    if (g_save_7cb328)
                        LightUpthisDeleteIcon(p, 0);
                }
            } else {
                SetIconSprite(p, GetSavePanelBK(p->u1c.slot));
                ty = p->y + 7;
                sy = p->y;
            }
            if (p->u20.flags20 & 2)
                PrintSprite(g_save_type_normal, p->x + 7, sy + 6, 0, 0);
            if (p->u20.flags20 & 4)
                PrintSprite(g_save_type_free, p->x + 7, sy + 6, 0, 0);
name:
            rc.top = ty;
            rc.bottom = ty + 0x13;
            rc.left = p->x + 0x28;
            owner = p->u18.owner;
            rc.right = rc.left + 0xd7;
            if (owner && lit) {
                if (!((*(unsigned int*)&g_cur_profile.save_slot & 0xff) - 1 == p->u1c.slot &&
                      (g_delete_popup_up || g_newsave_popup_up)))
                    NewPrintCent(owner, 2, rc, 1);
            }
        }
        p = p->next;
    }
    if (g_newsave_popup_up) {
        rc.left = cur->x + 8;
        rc.top = cur->y + 7;
        rc.right = rc.left + 0xae;
        rc.bottom = rc.top + 0x13;
        PrintSprite(g_save_corner_mask, cur->x, cur->y, 0, 0);
        if (g_7986f0)
            NewPrintCent(GetString(0xc44), 2, rc, 1);
        else
            NewPrintCent(GetString(0x8c), 2, rc, 1);
        UpdateProfileCheckBoxIcons();
    } else if (g_delete_popup_up) {
        rc.left = cur->x + 0x14;
        rc.top = cur->y + 7;
        rc.right = rc.left + 0x9b;
        rc.bottom = rc.top + 0x13;
        PrintSprite(g_save_corner_mask, cur->x, cur->y, 0, 0);
        NewPrintCent(GetString(0x85), 2, rc, 1);
        UpdateProfileCheckBoxIcons();
    }
    if (g_accept_icon) {
        if (g_cur_profile.save_slot)
            g_accept_icon->flags &= ~0x400;
        else
            g_accept_icon->flags |= 0x400;
    }
}

/* =========================================================================
 *  In-game interface
 * ========================================================================= */

/* The in-game control icon positions @ 0x004bb04c: four theme buttons
 * (y = 0x17b) then path / query / eraser / map / options (y = 0x1a2). */
extern Pos g_if_icon_pos[9];              /* 0x004bb04c */

/* The 16-byte control-icon state block @ 0x007fdd70 (fpui.c's CtrlIconState):
 * here it holds the four theme icons. */
typedef struct ThemeIcons {
    Icon* legoland;   /* +0x00 0x007fdd70 */
    Icon* western;    /* +0x04 0x007fdd74 */
    Icon* castle;     /* +0x08 0x007fdd78 */
    Icon* adventure;  /* +0x0c 0x007fdd7c */
} ThemeIcons;
extern ThemeIcons g_theme_icons;          /* 0x007fdd70 */

/* The side-panel scroll state @ 0x007fdd80. */
typedef struct PanelState {
    char f00;    /* +0x00 0x007fdd80 */
    int  f04;    /* +0x04 0x007fdd84 */
    int  f08;    /* +0x08 0x007fdd88 */
    char f0c;    /* +0x0c 0x007fdd8c */
} PanelState;
extern PanelState g_panel_state;          /* 0x007fdd80 */

/* Four pointer-drag state bytes @ 0x007fe114. */
typedef struct DragState {
    char f0;     /* 0x007fe114 */
    char f1;     /* 0x007fe115 */
    char f2;     /* 0x007fe116 */
    char f3;     /* 0x007fe117 */
} DragState;
extern DragState g_drag_state;            /* 0x007fe114 */

extern int     g_interface_loaded;        /* 0x00668ebc */
extern void*   g_env_class;               /* 0x007fd624 environment class (PATH CONTROL) */
extern Icon*   g_western_icon;            /* 0x00668e3c */
extern Icon*   g_brief_icon;              /* 0x00668e9c */
extern Icon*   g_script_end_icon;         /* 0x00668eb8 */
extern int     g_menu_index;              /* 0x004baff8 */
extern int     g_state_810140;            /* 0x00810140 */
extern char    g_66861c[];                /* 0x0066861c */
extern Sprite* g_ci_path_pressed;         /* 0x007fdcd0 IF_PathIconPressed.lls */
extern Sprite* g_ci_path;                 /* 0x007fdd50 IF_PathIcon.lls */
extern Sprite* g_ci_query_pressed;        /* 0x007fdcd4 */
extern Sprite* g_ci_query;                /* 0x007fdd54 */
extern Sprite* g_ci_eraser_pressed;       /* 0x007fdcd8 */
extern Sprite* g_ci_eraser;               /* 0x007fdd58 */
extern Sprite* g_ci_map_pressed;          /* 0x007fdcdc */
extern Sprite* g_ci_map;                  /* 0x007fdd5c */
extern Sprite* g_ci_options;              /* 0x007fdd60 */
extern Sprite* g_theme_legoland_off;      /* 0x007fdd40 */
extern Sprite* g_theme_western_off;       /* 0x007fdd44 */
extern Sprite* g_theme_castle_off;        /* 0x007fdd48 */
extern Sprite* g_theme_adv_off;           /* 0x007fdd4c */
extern Sprite* g_ci_brief2;               /* 0x00668e94 BriefIcon2.lls */
extern Sprite* g_ci_brief;                /* 0x00668e98 BriefIcon.lls */
extern Sprite* g_ci_script_end;           /* 0x00668ea0 ScriptEnd.lls */

extern const char g_env_class_name[];     /* 0x004b8a70 "PATH CONTROL" */
extern const char g_lls_bar_energy[];     /* 0x004bb48c "Bar_Energy.lls" */
extern const char g_lls_bar_coins[];      /* 0x004bb47c "Bar_Coins.lls" */

/* The object class record as this file sees it: the help text at +0x7c. */
typedef struct EnvClass {
    char  pad[0x7c];  /* +0x00 */
    char* help;       /* +0x7c */
} EnvClass;

/* The LLIDB element: its data pointer at +0xc. */
typedef struct IfElem {
    char      pad[0xc];  /* +0x00 */
    EnvClass* data;      /* +0x0c */
} IfElem;

extern int  LLIDB_FindElement(const char* name, IfElem** out, unsigned int* idx); /* 0x0047b330 */
extern int  RenderGBarSpriteIcon(Icon*);            /* 0x0046e850 */
extern int  RenderEnergyBar(Icon*);                 /* 0x0046e4d0 */
extern int  RenderMoneyBar(Icon*);                  /* 0x0046e670 */
extern int  RenderBriefIcon(Icon*);                 /* 0x0046e040 */
extern int  RenderScriptEndIcon(Icon*);             /* 0x00443e30 */
extern char PathIconInput(Icon*, int);              /* 0x00474fc0 */
extern char QueryIconInput(Icon*, int);             /* 0x00475000 */
extern char EraserIconInput(Icon*, int);            /* 0x00475040 */
extern char MapIconInput(Icon*, int);               /* 0x00475080 */
extern char OptionsIconInput(Icon*, int);           /* 0x00475120 */
extern char LegolandThemeInput(Icon*, int);         /* 0x004751a0 */
extern char WesternThemeInput(Icon*, int);          /* 0x004754b0 */
extern char CastleThemeInput(Icon*, int);           /* 0x004753a0 */
extern char AdventureThemeInput(Icon*, int);        /* 0x004752a0 */
extern char BriefIconInput(Icon*, int);             /* 0x00474f40 */
extern char ScriptEndIconInput(Icon*, int);         /* 0x00474fa0 */
#ifndef LEGOLAND_PORTABLE
extern void UpdateHelpIconForText(void* block);      /* 0x00491240 */
#else
extern int UpdateHelpIconForText(void* block);      /* 0x00491240 */
#endif
extern void StopScript(int stop);                   /* 0x0046b240 */
extern int  ScriptRunning(void);             /* 0x0046b280 */
extern void ToggleHelpIcon(int on);                 /* 0x004748a0 */
extern void InitPopUpInfo(void);                    /* 0x00470bb0 */
extern void ResetThemeMenuFlags(void);                 /* 0x00474590 */
extern void ResetMoveAWorkerStruct(void);           /* 0x00470930 */
extern void RemoveObjectListIcons(int group);       /* 0x0046fb40 */
extern void UpdateThemeIconsFromProfile(void);                 /* 0x00476180 */
extern void UpdateThemeIconsFromFlags(void);                 /* 0x00474990 */

/* Builds the in-game interface once: the PATH CONTROL environment class is
 * bound to the path icon, the five control icons (group 0x93) and the four
 * theme icons (group 0x9a) come from the position table, plus the energy and
 * coin bars, the briefing icon and the script-end icon. Then the script and
 * help state, the popup info, the side panel and the object list. */
// FUNCTION: LEGOLAND 0x004749d0
void InitGameInterface(int resume)
{
    IfElem* elem;
    EnvClass* env;
    Icon* p;

    if (g_interface_loaded == 0) {
        g_interface_loaded = 1;
        LLIDB_FindElement(g_env_class_name, &elem, 0);
        env = elem->data;
        g_env_class = env;
        p = InsertIcon(g_if_icon_pos[4].x, g_if_icon_pos[4].y, 0x93, g_ci_path);
        p->help = env->help;
        p->help_id = -1;
        p->u18.ctrl = 1;
        p->u1c.pressed = g_ci_path_pressed;
        p->u20.normal = g_ci_path;
        p->data = env;
        p->render = RenderGBarSpriteIcon;
        p->input = PathIconInput;
        p->flags &= ~0x200;
        p->flags |= 0x300a;
        p = InsertIcon(g_if_icon_pos[5].x, g_if_icon_pos[5].y, 0x93, g_ci_query);
        p->help_id = 0x5a;
        p->help = GetString(0x5a);
        p->input = QueryIconInput;
        p->flags |= 0x6002;
        p->u18.ctrl = 2;
        p->u1c.pressed = g_ci_query_pressed;
        p->u20.normal = g_ci_query;
        p = InsertIcon(g_if_icon_pos[6].x, g_if_icon_pos[6].y, 0x93, g_ci_eraser);
        p->help_id = 0x5b;
        p->help = GetString(0x5b);
        p->input = EraserIconInput;
        p->flags |= 0x6002;
        p->u18.ctrl = 3;
        p->u1c.pressed = g_ci_eraser_pressed;
        p->u20.normal = g_ci_eraser;
        p = InsertIcon(g_if_icon_pos[7].x, g_if_icon_pos[7].y, 0x93, g_ci_map);
        p->help_id = 0x5c;
        p->help = GetString(0x5c);
        p->input = MapIconInput;
        p->flags |= 0x6002;
        p->u18.ctrl = 4;
        p->u1c.pressed = g_ci_map_pressed;
        p->u20.normal = g_ci_map;
        p = InsertIcon(g_if_icon_pos[8].x, g_if_icon_pos[8].y, 0x93, g_ci_options);
        p->help_id = 0x5d;
        p->help = GetString(0x5d);
        p->input = OptionsIconInput;
        p->flags |= 0x6002;
        p->u20.normal = g_ci_options;
        p = InsertIcon(g_if_icon_pos[0].x, g_if_icon_pos[0].y, 0x9a, g_theme_legoland_off);
        p->help_id = 0x5e;
        p->help = GetString(0x5e);
        p->flags |= 0x6002;
        p->input = LegolandThemeInput;
        g_theme_icons.legoland = p;
        p = InsertIcon(g_if_icon_pos[1].x, g_if_icon_pos[1].y, 0x9a, g_theme_western_off);
        g_western_icon = p;
        p->help_id = 0x5f;
        p->help = GetString(0x5f);
        p->flags |= 0x6002;
        p->input = WesternThemeInput;
        g_theme_icons.western = p;
        p = InsertIcon(g_if_icon_pos[2].x, g_if_icon_pos[2].y, 0x9a, g_theme_castle_off);
        p->help_id = 0x60;
        p->help = GetString(0x60);
        p->flags |= 0x6002;
        p->input = CastleThemeInput;
        g_theme_icons.castle = p;
        p = InsertIcon(g_if_icon_pos[3].x, g_if_icon_pos[3].y, 0x9a, g_theme_adv_off);
        p->help_id = 0x61;
        p->help = GetString(0x61);
        p->flags |= 0x6002;
        p->input = AdventureThemeInput;
        g_theme_icons.adventure = p;
        p = LoadSpriteIcon(g_lls_bar_energy, 4, 0x180, 6, 0x9a);
        p->render = RenderEnergyBar;
        p->flags |= 0x400a;
        p = LoadSpriteIcon(g_lls_bar_coins, 4, 0x1c, 6, 0x9a);
        p->render = RenderMoneyBar;
        p->flags |= 0x400a;
        p = InsertIcon(0x19e, 0x179, 0x9a, g_ci_brief2);
        p->help_id = 0x24e;
        p->help = GetString(0x24e);
        p->u18.owner = g_ci_brief;
        p->flags |= 0x600a;
        p->input = BriefIconInput;
        p->render = RenderBriefIcon;
        g_brief_icon = p;
        p->flags |= 0x400;
        UpdateHelpIconForText(g_66861c);
        p = InsertIcon(0x20a, 0x17a, 0x93, g_ci_script_end);
        p->u20.normal = 0;
        p->u1c.pressed = 0;
        p->help_id = 0x24f;
        p->help = GetString(0x24f);
        p->flags |= 0x4008;
        p->input = ScriptEndIconInput;
        p->render = RenderScriptEndIcon;
        g_script_end_icon = p;
        if (resume != 0) {
            StopScript(0);
            if (g_cur_profile.f45 == 2)
                ToggleHelpIcon(0);
            else
                ToggleHelpIcon(1);
        } else if (ScriptRunning()) {
            StopScript(1);
        } else {
            StopScript(0);
            if (g_cur_profile.f45 == 2)
                ToggleHelpIcon(0);
            else
                ToggleHelpIcon(1);
        }
        InitPopUpInfo();
    }
    ResetThemeMenuFlags();
    g_drag_state.f0 = 0;
    g_drag_state.f3 = 0;
    g_drag_state.f2 = 0;
    g_drag_state.f1 = 0;
    ResetMoveAWorkerStruct();
    g_panel_state.f00 = 2;
    g_panel_state.f0c = (char)0x86;
    g_level_map->f20 = 0;
    g_level_map->f10 = 0x280;
    g_panel_state.f04 = 1;
    g_panel_state.f08 = 0;
    g_menu_index = 5;
    RemoveObjectListIcons(0xd2);
    UpdateThemeIconsFromProfile();
    if (g_state_810140 != 0)
        UpdateThemeIconsFromFlags();
}
