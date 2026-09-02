/* LEGOLAND — the front-end profile, save, title and option screens.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Only struct
 * field offsets, callee argument counts and global addresses are load-bearing;
 * names are ours. Types are defined LOCALLY on purpose (legoland.h is owned
 * elsewhere; the Icon record is the same 0x40-byte one iconui.c/fpui.c use).
 *
 * Screen layouts recovered here (all icons in group 7 unless noted; the
 * checkbox/popup buttons live in group 0xe so CloseFontEndCheckBox can drop
 * them as a unit):
 *
 *  Title screen (InitTitleScreen): backdrop TitleScreenBk.lls, then
 *    New_on_Title (0xca,0x138) / Free_on_Title (0x9a,8) / Reg_on_Title
 *    (0x18,0x71) / Exit_On_Title (0x1e1,0x13) / Load_on_Title (0x19,0x118)
 *    / MovieOn_On_Title (0x10e,0xa1). Without a current profile "New" is
 *    drawn through Dark_New_On_Title and "Free" is disabled.
 *  Profile list (InitListProfiles): backdrop Reg_ScreenBK.lls; one slot icon
 *    per profile at (0x80, 0x86 + 38*slot) using RegProfileOff_<slot>.lls,
 *    Accept_On_Reg at (0x1ef,0x14f), plus the delete icon (RegDelete /
 *    RegDeleteOn) that LightUpthisDeleteIcon parks beside the lit slot.
 *  Option screen (InitOptionScreen): backdrop OptionScreenBK.lls; Accept
 *    (0x13,0x14f), Exit (0x1e0,6), Load (0x168,0x13a), Save (0x1df,0xbd) and
 *    three volume rows (speech y=0x2e/0x33, music 0x72/0x77, fx 0xb6/0xbb):
 *    Down<n> at x=0x19c, Up<n> at x=0x26 and a VolMarker<n> whose x is the
 *    volume mapped through VolumeToMarkerX. The row/control number sits in
 *    the icon's +0x18 word (1..9). The three live volumes are snapshotted
 *    into 0x7cb314/0x7cb31c/0x7cb315 so the exit handler can restore them.
 *  Popups: InitProfileCheckBoxIcons / InitNewSaveGamePOPUP / InitExitCheckBox
 *    build an OK icon (PU_OK / PU_OKON) and a close icon (RegClose or
 *    PU_ClosePopUp, with their ON variants) relative to the popup panel.
 */
#include "legoland.h"

unsigned int strlen(const char*);
char* strcpy(char*, const char*);
#pragma intrinsic(strlen, strcpy)

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
        void*      owner;     /* +0x18  profile list icons: the Profile record */
        short      control;   /* +0x18  option screen: control number 1..9 */
    } u18;
    unsigned char  slot;      /* +0x1c  profile slot number */
    char           pad1d[3];  /* +0x1d */
    unsigned char  flags20;   /* +0x20  bit 0: profile slot icon */
    char           pad21[0x28 - 0x21];
    int          (*render)(struct Icon*);          /* +0x28 */
    char         (*input)(struct Icon*, int);      /* +0x2c */
    void*          widget;    /* +0x30 */
    unsigned int   flags;     /* +0x34 */
    char*          help;      /* +0x38  GetString(help_id) */
    int            help_id;   /* +0x3c */
} Icon;

typedef char (*IconInputFn)(Icon*, int);

/* Win32 RECT, passed BY VALUE to the text printers (text.c's WinRect). */
typedef struct WinRect {
    long left;     /* +0x00 */
    long top;      /* +0x04 */
    long right;    /* +0x08 */
    long bottom;   /* +0x0c */
} WinRect;

/* ---- the temp profile record (saveprof.c) ------------------------------ */
/* Only the name and its length byte are touched here: the text editors
 * append to name[] and keep the length at +0x1e. */
typedef struct Profile {
    char           name[0x1e];   /* +0x00 */
    unsigned char  len;          /* +0x1e */
    unsigned char  f1f;          /* +0x1f */
    char           rest[0x110 - 0x20];
} Profile;

extern Profile g_temp_profile;   /* 0x007cad60 */

/* ---- the profile list (profiles.c) ------------------------------------- */
typedef struct ProfileNode {
    struct ProfileNode* next;    /* +0x000 */
    char                p[0x110];/* +0x004  the Profile record (name first) */
    int                 valid;   /* +0x114 */
    unsigned char       slot;    /* +0x118 */
} ProfileNode;

extern ProfileNode* g_profile_list;   /* 0x00798890 */

/* ---- the live profile (profiles.c's CurProfile) ------------------------ */
extern int  g_vol_speech;        /* 0x0080ffc4 */
extern int  g_vol_music;         /* 0x0080ffc8 */
extern int  g_vol_sfx;           /* 0x0080ffcc */

/* ---- globals ------------------------------------------------------------ */
extern Pos   g_gfx_point;             /* 0x00813a44 mouse point */
extern int   g_screen_popup;          /* 0x0080ff80 pending front-end pop-up */
extern int   g_screen_mode;           /* 0x0080ff88 sub-mode of the current screen */
extern IconInputFn g_icon_handler1;   /* 0x006687bc */
extern IconInputFn g_icon_handler2;   /* 0x006687c0 */
extern Sprite* g_backdrop;            /* 0x00810148 full-screen background */

/* The front-end checkbox / popup sprites and icons @ 0x00798674.. */
extern Sprite* g_fe_sprite_674;       /* 0x00798674 PU_OKON.lls */
extern Sprite* g_fe_sprite_678;       /* 0x00798678 PU_OK.lls */
extern Sprite* g_fe_sprite_67c;       /* 0x0079867c RegClose.lls / PU_ClosePopUp.lls */
extern Sprite* g_fe_sprite_680;       /* 0x00798680 RegCloseON.lls / PU_ClosePopUpON.lls */
extern Sprite* g_fe_sprite_684;       /* 0x00798684 PU_ClosePopUp.lls */
extern Sprite* g_fe_sprite_688;       /* 0x00798688 PU_ClosePopUpON.lls */
extern Icon*   g_np_icon_ok;          /* 0x007986d8 popup OK icon */
extern Icon*   g_np_close_icon;       /* 0x007986dc popup close icon */
extern Icon*   g_accept_icon;         /* 0x007986e0 Accept_On_Reg */
extern int     g_delete_popup_up;     /* 0x007986e4 Reg_Delete_PopUp showing */
extern int     g_newprof_popup_up;    /* 0x007986e8 new-profile popup showing */
extern int     g_7986f0;              /* 0x007986f0 */

/* The profile-list screen sprites (KillListProfileSprite's nineteen). */
extern Sprite* g_lp_delete_on;        /* 0x0079868c RegDeleteOn.lls */
extern Sprite* g_lp_delete;           /* 0x00798690 RegDelete.lls */
extern Sprite* g_lp_off1;             /* 0x00798694 RegProfileOff_1.lls */
extern Sprite* g_lp_off2;             /* 0x00798698 */
extern Sprite* g_lp_off3;             /* 0x0079869c */
extern Sprite* g_lp_off4;             /* 0x007986a0 */
extern Sprite* g_lp_off5;             /* 0x007986a4 */
extern Sprite* g_lp_off6;             /* 0x007986a8 */
extern Sprite* g_lp_off7;             /* 0x007986ac */
extern Sprite* g_lp_off8;             /* 0x007986b0 RegProfileOff_8.lls */
extern Sprite* g_lp_on;               /* 0x007986b4 RegProfileON.lls */
extern Sprite* g_lp_delete_popup;     /* 0x007986b8 Reg_Delete_PopUp.lls */
extern Sprite* g_lp_diff_popup;       /* 0x007986bc Reg_Diff_PopUp.lls */
extern Sprite* g_lp_easy_on;          /* 0x007986c0 */
extern Sprite* g_lp_easy_off;         /* 0x007986c4 */
extern Sprite* g_lp_mid_on;           /* 0x007986c8 */
extern Sprite* g_lp_mid_off;          /* 0x007986cc */
extern Sprite* g_lp_hard_on;          /* 0x007986d0 */
extern Sprite* g_lp_hard_off;         /* 0x007986d4 */

extern Icon*   g_delete_icon;         /* 0x007cb360 the RegDelete icon */
extern char    g_lp_title[];          /* 0x007cb340 GetString(0x84) copy */

/* Save-game / exit popup state. */
extern int     g_saved_game_len;      /* 0x00798738 pixel width of the typed name */
extern int     g_profile_name_len;    /* 0x00798894 pixel width of the typed name */
extern int     g_exit_big_popup;      /* 0x00798754 */
extern Icon*   g_vol_marker_speech;   /* 0x00798748 */
extern Icon*   g_vol_marker_fx;       /* 0x0079874c */
extern Icon*   g_vol_marker_music;    /* 0x00798750 */
extern int     g_exit_7cb310;         /* 0x007cb310 */
extern char    g_opt_speech;          /* 0x007cb314 snapshot of g_vol_speech */
extern char    g_opt_sfx;             /* 0x007cb315 snapshot of g_vol_sfx */
extern int     g_exit_7cb318;         /* 0x007cb318 */
extern char    g_opt_music;           /* 0x007cb31c snapshot of g_vol_music */
extern int     g_exit_popup_up;       /* 0x007cb320 */

/* ---- callees ------------------------------------------------------------ */
extern Sprite* LoadSprite(const char* name, int mode);                 /* 0x00497ab0 */
extern void    KillSprite(Sprite* s);                                   /* 0x00497bd0 */
extern Icon*   InsertIcon(short x, short y, unsigned short group, Sprite* s); /* 0x0046d6c0 */
extern Icon*   LoadSpriteIcon(const char* name, int mode, int x, int y, int group); /* 0x0046d7b0 */
extern void    SetIconSprite(Icon* p, Sprite* s);                       /* 0x0046d680 */
extern char*   GetString(int id);                                       /* 0x00498f50 */
extern char    GetInputChar(void);                                      /* 0x004740b0 */
extern int     GetBlink(void);                                          /* 0x00499480 */
extern int     UpdateSoundVols(void);                                   /* 0x00495a90 */
extern void    DeleteProfileList(void);                                 /* 0x00491b50 */
extern char    LoadProfilesFormDisk(void);                              /* 0x00491470 */
extern void    PushRenderingStatusAndUnlockVideoSurface(void);          /* 0x00464080 */
extern void    PopRenderingStatus(void);                                /* 0x004641f0 */
extern void    InfoPrintCent(int len, const char* text, int font, WinRect rc, int centre); /* 0x004716a0 */

/* 0x0048c5e0 (not exported): RegProfileOff_<slot>.lls for slot 1..8. */
extern Sprite* GetProfileOffSprite(char slot);
/* 0x0048e420 (not exported): puts the OK / close popup icons back to their
 * un-lit sprites. */
extern void    ResetSavePopupIcons(void);
/* 0x0048eaf0 (not exported): volume 0..100 -> marker x (vol*241/100 + 0x7c). */
extern int     VolumeToMarkerX(int vol);
/* 0x0048eb20 (not exported): stash the two icon handlers in 0x798740/3c;
 * 0x0048eb40 restores them. */
extern void    SaveOptionIconHandlers(void);
/* 0x00474ed0 (not exported): drop the in-game interface icon groups. */
extern void    UnLoadInGameIcons(void);
/* 0x00499380 (not exported): pause the game timer (returns 1 if already). */
extern int     PauseGameTimer(void);
/* 0x0048fc30 (not exported): non-zero when a profile is selected (0x80ffd9). */
extern int     HaveCurrentProfile(void);
/* 0x0047f820 (not exported): GetGameTimer() - g_669204 (time since save). */
extern int     TimeSinceSave(void);
/* 0x00491e40 (not exported): NewPrintCent-style printer that RETURNS the x at
 * which the text ends (so the cursor can follow it). */
extern int     PrintTextGetEnd(const char* text, int font, WinRect rc, char white);
/* 0x00490fa0 (not exported): NewPrintCent twin used for the blinking cursor. */
extern void    PrintCursor(const char* text, int font, WinRect rc, char white);

/* Icon input handlers (other lanes). */
extern char ProfileOkInput(Icon*, int);          /* 0x0048d400 */
extern char ProfileCloseInput(Icon*, int);       /* 0x0048d450 */
extern char SaveGameOkInput(Icon*, int);         /* 0x0048e720 */
extern char SaveGameCloseInput(Icon*, int);      /* 0x0048e810 */
extern char ExitBigOkInput(Icon*, int);          /* 0x0048f440 */
extern char ExitOkInput(Icon*, int);             /* 0x0048ef90 */
extern char ExitCloseInput(Icon*, int);          /* 0x0048f4b0 */
extern char TitleNewInput(Icon*, int);           /* 0x0048feb0 */
extern char TitleFreeInput(Icon*, int);          /* 0x0048ff20 */
extern char TitleRegInput(Icon*, int);           /* 0x0048ff70 */
extern char TitleExitInput(Icon*, int);          /* 0x0048f0a0 */
extern char TitleLoadInput(Icon*, int);          /* 0x0048fe20 */
extern char TitleMovieInput(Icon*, int);         /* 0x0048ffe0 */
extern char ProfileAcceptInput(Icon*, int);      /* 0x0048d300 */
extern char ProfileSlotInput(Icon*, int);        /* 0x0048d390 */
extern char ProfileEmptySlotInput(Icon*, int);   /* 0x0048d3c0 */
extern char NewProfileCloseInput(Icon*, int);    /* 0x004920a0 */
extern char DeleteIconInput(Icon*, int);         /* 0x0048cc30 */
extern char OptionAcceptInput(Icon*, int);       /* 0x0048efd0 */
extern char OptionLoadInput(Icon*, int);         /* 0x0048f550 */
extern char OptionSaveInput(Icon*, int);         /* 0x0048f050 */
extern char VolDownInput(Icon*, int);            /* 0x0048f760 */
extern char VolUpInput(Icon*, int);              /* 0x0048f5f0 */
extern char VolMarkerInput(Icon*, int);          /* 0x0048f8d0 */
extern int  RenderDarkTitleIcon(Icon*);          /* 0x0046e920 */

/* Sprite names (.rdata; only the addresses are load-bearing). */
extern const char g_lls_pu_ok[];              /* 0x004baa70 "PU_OK.lls" */
extern const char g_lls_pu_ok_on[];           /* 0x004baa64 "PU_OKON.lls" */
extern const char g_lls_regclose[];           /* 0x004bf148 "RegClose.lls" */
extern const char g_lls_regclose_on[];        /* 0x004bf138 "RegCloseON.lls" */
extern const char g_lls_regclose_on2[];       /* 0x004bf2d4 "RegCloseOn.lls" */
extern const char g_lls_pu_close[];           /* 0x004bab78 "PU_ClosePopUp.lls" */
extern const char g_lls_pu_close_on[];        /* 0x004bab8c "PU_ClosePopUpON.lls" */
extern const char g_lls_pu_bigpopup[];        /* 0x004bf4c4 "PU_BigPopupBK.lls" */
extern const char g_lls_pu_infopopup[];       /* 0x004bf4b0 "PU_InfoPopupBK.lls" */
extern const char g_lls_title_bk[];           /* 0x004bf574 "TitleScreenBk.lls" */
extern const char g_lls_new_on_title[];       /* 0x004bf560 "New_on_Title.lls" */
extern const char g_lls_dark_new_on_title[];  /* 0x004bf548 "Dark_New_On_Title.lls" */
extern const char g_lls_free_on_title[];      /* 0x004bf534 "Free_on_Title.lls" */
extern const char g_lls_reg_on_title[];       /* 0x004bf520 "Reg_on_Title.lls" */
extern const char g_lls_exit_on_title[];      /* 0x004bf50c "Exit_On_Title.lls" */
extern const char g_lls_load_on_title[];      /* 0x004bf4f8 "Load_on_Title.lls" */
extern const char g_lls_movie_on_title[];     /* 0x004bf4e0 "MovieOn_On_Title.lls" */
extern const char g_lls_reg_screen_bk[];      /* 0x004bf124 "Reg_ScreenBK.lls" */
extern const char g_lls_reg_delete_on[];      /* 0x004bf114 "RegDeleteOn.lls" */
extern const char g_lls_reg_delete[];         /* 0x004bf104 "RegDelete.lls" */
extern const char g_lls_reg_profile_on[];     /* 0x004bf0f0 "RegProfileON.lls" */
extern const char g_lls_reg_off1[];           /* 0x004bf0dc "RegProfileOff_1.lls" */
extern const char g_lls_reg_off2[];           /* 0x004bf0c8 */
extern const char g_lls_reg_off3[];           /* 0x004bf0b4 */
extern const char g_lls_reg_off4[];           /* 0x004bf0a0 */
extern const char g_lls_reg_off5[];           /* 0x004bf08c */
extern const char g_lls_reg_off6[];           /* 0x004bf078 */
extern const char g_lls_reg_off7[];           /* 0x004bf064 */
extern const char g_lls_reg_off8[];           /* 0x004bf050 "RegProfileOff_8.lls" */
extern const char g_lls_reg_delete_popup[];   /* 0x004bf038 "Reg_Delete_PopUp.lls" */
extern const char g_lls_reg_diff_popup[];     /* 0x004bf024 "Reg_Diff_PopUp.lls" */
extern const char g_lls_reg_easy_on[];        /* 0x004bf014 "Reg_Easy_On.lls" */
extern const char g_lls_reg_easy_off[];       /* 0x004bf000 "Reg_Easy_Off.lls" */
extern const char g_lls_reg_mid_on[];         /* 0x004beff0 "Reg_Mid_On.lls" */
extern const char g_lls_reg_mid_off[];        /* 0x004befe0 "Reg_Mid_Off.lls" */
extern const char g_lls_reg_hard_on[];        /* 0x004befd0 "Reg_Hard_On.lls" */
extern const char g_lls_reg_hard_off[];       /* 0x004befbc "Reg_Hard_Off.lls" */
extern const char g_lls_accept_on_reg[];      /* 0x004befa8 "Accept_On_Reg.lls" */
extern const char g_str_empty_slot[];         /* 0x004befa0 "EMPTY" */
extern const char g_lls_option_bk[];          /* 0x004bf49c "OptionScreenBK.lls" */
extern const char g_lls_accept_on_options[];  /* 0x004bf484 "Accept_On_Options.lls" */
extern const char g_lls_exit_on_options[];    /* 0x004bf470 "Exit_on_Options.lls" */
extern const char g_lls_load_on_options[];    /* 0x004bf45c "Load_on_Options.lls" */
extern const char g_lls_save_on_options[];    /* 0x004bf448 "Save_on_Options.lls" */
extern const char g_lls_down1[];              /* 0x004bf43c "Down1.lls" */
extern const char g_lls_up1[];                /* 0x004bf434 "Up1.lls" */
extern const char g_lls_vol_speech[];         /* 0x004bf420 "VolMarkerSpeech.lls" */
extern const char g_lls_down2[];              /* 0x004bf414 "Down2.lls" */
extern const char g_lls_up2[];                /* 0x004bf40c "Up2.lls" */
extern const char g_lls_vol_music[];          /* 0x004bf3f8 "VolMarkerMusic.lls" */
extern const char g_lls_down3[];              /* 0x004bf3ec "Down3.lls" */
extern const char g_lls_up3[];                /* 0x004bf3e4 "Up3.lls" */
extern const char g_lls_vol_fx[];             /* 0x004bf3d4 "VolMarkerFX.lls" */
extern const char g_str_cursor_bar[];         /* 0x004bf2e4 "|" */
extern const char g_str_cursor_space[];       /* 0x004b8ad4 " " */

void UpdateProfileCheckBoxIcons(void);

/* Is the mouse inside the 0x24 x 0x1b button rectangle of `p`? (The shared
 * inline helper is what lets VC6 sink the ebx/esi/edi pushes past the
 * leading straight-line code in the two users below.) */
typedef struct IRect {
    int l;
    int t;
    int r;
    int b;
} IRect;

static __inline int MouseInRect(IRect rc)
{
    if (g_gfx_point.x < rc.r && rc.l < g_gfx_point.x &&
        g_gfx_point.y < rc.b && rc.t < g_gfx_point.y)
        return 1;
    return 0;
}

static __inline int MouseOverButton(Icon* p)
{
    IRect rc;
    rc.l = p->x;
    rc.t = p->y;
    rc.r = rc.l + 0x24;
    rc.b = rc.t + 0x1b;
    return MouseInRect(rc);
}

/* =========================================================================
 *  Profile list screen
 * ========================================================================= */

/* Enables the delete icon and parks it beside the lit profile slot: to the
 * right of the delete popup when `lit`, else on the slot row itself (pushed
 * down a row while the delete popup is up). Lights it when the mouse is on
 * it. */
// FUNCTION: LEGOLAND 0x0048cd50
void LightUpthisDeleteIcon(Icon* slot, int lit)
{
    g_delete_icon->flags &= ~0x400;
    if (lit) {
        g_delete_icon->y = slot->y + 0x1b;
        g_delete_icon->x = slot->x + 0xe1;
    } else {
        if (g_delete_popup_up)
            g_delete_icon->y = slot->y + 0x1b;
        else
            g_delete_icon->y = slot->y;
        g_delete_icon->x = slot->x + 0xff;
    }
    SetIconSprite(g_delete_icon, g_lp_delete);
    if (MouseOverButton(g_delete_icon))
        SetIconSprite(g_delete_icon, g_lp_delete_on);
}

/* Per-frame sprite selection for the popup OK / close icons: the plain
 * sprite, then the ON variant while the mouse is over the button. */
// FUNCTION: LEGOLAND 0x0048ce20
void UpdateProfileCheckBoxIcons(void)
{
    if (g_np_icon_ok)
        SetIconSprite(g_np_icon_ok, g_fe_sprite_678);
    if (g_newprof_popup_up)
        SetIconSprite(g_np_close_icon, g_fe_sprite_684);
    else
        SetIconSprite(g_np_close_icon, g_fe_sprite_67c);
    if (g_np_icon_ok) {
        if (MouseOverButton(g_np_icon_ok))
            SetIconSprite(g_np_icon_ok, g_fe_sprite_674);
    }
    if (MouseOverButton(g_np_close_icon)) {
        if (g_newprof_popup_up)
            SetIconSprite(g_np_close_icon, g_fe_sprite_688);
        else
            SetIconSprite(g_np_close_icon, g_fe_sprite_680);
    }
}

/* OK + close icons (group 0xe) for the profile-screen popups, 0x24 left /
 * 0x18 above the popup panel. */
// FUNCTION: LEGOLAND 0x0048c720
void InitProfileCheckBoxIcons(Icon* popup)
{
    g_fe_sprite_678 = LoadSprite(g_lls_pu_ok, 4);
    g_fe_sprite_674 = LoadSprite(g_lls_pu_ok_on, 4);
    g_fe_sprite_67c = LoadSprite(g_lls_regclose, 4);
    g_fe_sprite_680 = LoadSprite(g_lls_regclose_on, 4);
    g_fe_sprite_684 = LoadSprite(g_lls_pu_close, 4);
    g_fe_sprite_688 = LoadSprite(g_lls_pu_close_on, 4);
    g_np_icon_ok = InsertIcon(popup->x - 0x24, popup->y - 0x18, 0xe, g_fe_sprite_678);
    g_np_icon_ok->help_id = 2;
    g_np_icon_ok->help = GetString(2);
    g_np_icon_ok->flags |= 0x2000;
    g_np_icon_ok->flags |= 0x4002;
    g_np_icon_ok->input = ProfileOkInput;
    g_np_close_icon = InsertIcon(g_np_icon_ok->x + 0x24, g_np_icon_ok->y, 0xe, g_fe_sprite_67c);
    g_np_close_icon->help_id = 4;
    g_np_close_icon->help = GetString(4);
    g_np_close_icon->flags |= 0x2000;
    g_np_close_icon->flags |= 0x4002;
    g_np_close_icon->input = ProfileCloseInput;
}

/* Frees the nineteen profile-list screen sprites. */
// FUNCTION: LEGOLAND 0x0048ca40
void KillListProfileSprite(void)
{
    if (g_lp_delete_on) {
        KillSprite(g_lp_delete_on);
        g_lp_delete_on = 0;
    }
    if (g_lp_delete) {
        KillSprite(g_lp_delete);
        g_lp_delete = 0;
    }
    if (g_lp_off1) {
        KillSprite(g_lp_off1);
        g_lp_off1 = 0;
    }
    if (g_lp_off2) {
        KillSprite(g_lp_off2);
        g_lp_off2 = 0;
    }
    if (g_lp_off3) {
        KillSprite(g_lp_off3);
        g_lp_off3 = 0;
    }
    if (g_lp_off4) {
        KillSprite(g_lp_off4);
        g_lp_off4 = 0;
    }
    if (g_lp_off5) {
        KillSprite(g_lp_off5);
        g_lp_off5 = 0;
    }
    if (g_lp_off6) {
        KillSprite(g_lp_off6);
        g_lp_off6 = 0;
    }
    if (g_lp_off7) {
        KillSprite(g_lp_off7);
        g_lp_off7 = 0;
    }
    if (g_lp_off8) {
        KillSprite(g_lp_off8);
        g_lp_off8 = 0;
    }
    if (g_lp_on) {
        KillSprite(g_lp_on);
        g_lp_on = 0;
    }
    if (g_lp_delete_popup) {
        KillSprite(g_lp_delete_popup);
        g_lp_delete_popup = 0;
    }
    if (g_lp_diff_popup) {
        KillSprite(g_lp_diff_popup);
        g_lp_diff_popup = 0;
    }
    if (g_lp_easy_on) {
        KillSprite(g_lp_easy_on);
        g_lp_easy_on = 0;
    }
    if (g_lp_easy_off) {
        KillSprite(g_lp_easy_off);
        g_lp_easy_off = 0;
    }
    if (g_lp_mid_on) {
        KillSprite(g_lp_mid_on);
        g_lp_mid_on = 0;
    }
    if (g_lp_mid_off) {
        KillSprite(g_lp_mid_off);
        g_lp_mid_off = 0;
    }
    if (g_lp_hard_on) {
        KillSprite(g_lp_hard_on);
        g_lp_hard_on = 0;
    }
    if (g_lp_hard_off) {
        KillSprite(g_lp_hard_off);
        g_lp_hard_off = 0;
    }
}

/* Builds the profile-list screen: reloads the profiles from disk, loads the
 * screen's sprites, the Accept icon, then one slot icon per list node and
 * the (disabled) delete icon. */
// FUNCTION: LEGOLAND 0x0048c260
void InitListProfiles(void)
{
    ProfileNode* node;
    Icon* p;

    UpdateSoundVols();
    DeleteProfileList();
    LoadProfilesFormDisk();
    node = g_profile_list;
    g_backdrop = LoadSprite(g_lls_reg_screen_bk, 0);
    g_lp_delete_on = LoadSprite(g_lls_reg_delete_on, 4);
    g_lp_delete = LoadSprite(g_lls_reg_delete, 4);
    g_lp_on = LoadSprite(g_lls_reg_profile_on, 4);
    g_lp_off1 = LoadSprite(g_lls_reg_off1, 4);
    g_lp_off2 = LoadSprite(g_lls_reg_off2, 4);
    g_lp_off3 = LoadSprite(g_lls_reg_off3, 4);
    g_lp_off4 = LoadSprite(g_lls_reg_off4, 4);
    g_lp_off5 = LoadSprite(g_lls_reg_off5, 4);
    g_lp_off6 = LoadSprite(g_lls_reg_off6, 4);
    g_lp_off7 = LoadSprite(g_lls_reg_off7, 4);
    g_lp_off8 = LoadSprite(g_lls_reg_off8, 4);
    g_lp_delete_popup = LoadSprite(g_lls_reg_delete_popup, 4);
    g_lp_diff_popup = LoadSprite(g_lls_reg_diff_popup, 4);
    g_lp_easy_on = LoadSprite(g_lls_reg_easy_on, 4);
    g_lp_easy_off = LoadSprite(g_lls_reg_easy_off, 4);
    g_lp_mid_on = LoadSprite(g_lls_reg_mid_on, 4);
    g_lp_mid_off = LoadSprite(g_lls_reg_mid_off, 4);
    g_lp_hard_on = LoadSprite(g_lls_reg_hard_on, 4);
    g_lp_hard_off = LoadSprite(g_lls_reg_hard_off, 4);
    g_accept_icon = LoadSpriteIcon(g_lls_accept_on_reg, 4, 0x1ef, 0x14f, 7);
    g_accept_icon->help_id = 6;
    g_accept_icon->help = GetString(6);
    g_accept_icon->flags |= 0x2000;
    g_accept_icon->flags |= 0x4002;
    g_accept_icon->flags |= 0x400;
    g_accept_icon->input = ProfileAcceptInput;
    g_icon_handler1 = ProfileAcceptInput;
    g_icon_handler2 = NewProfileCloseInput;
    strcpy(g_lp_title, GetString(0x84));
    while (node) {
        if (node->valid) {
            p = InsertIcon(0x80, node->slot * 38 + 0x86, 7, GetProfileOffSprite(node->slot));
            p->help_id = 0;
            p->help = GetString(0);
            p->flags |= 0x6002;
            p->input = ProfileSlotInput;
            p->u18.owner = node->p;
            p->slot = node->slot;
            p->flags20 |= 1;
        } else {
            p = InsertIcon(0x80, node->slot * 38 + 0x86, 7, GetProfileOffSprite(node->slot));
            p->help_id = 1;
            p->help = GetString(1);
            p->flags |= 0x6002;
            p->input = ProfileEmptySlotInput;
            p->u18.owner = (void*)g_str_empty_slot;
            p->slot = node->slot;
            p->flags20 |= 1;
        }
        node = node->next;
    }
    g_delete_icon = InsertIcon(0, 0, 7, g_lp_delete);
    g_delete_icon->help_id = 2;
    g_delete_icon->help = GetString(2);
    g_delete_icon->flags |= 0x2000;
    g_delete_icon->flags |= 0x4002;
    g_delete_icon->flags |= 0x400;
    g_delete_icon->input = DeleteIconInput;
}

/* =========================================================================
 *  Text entry (new profile name / saved game name)
 * ========================================================================= */

/* One keystroke of the new-profile name editor plus its redraw: backspace
 * (-1) removes a character, printable characters append while the name is
 * under 31 characters and 0x7b pixels wide (a leading space is refused).
 * The text is drawn 0x14 right / 7 below the panel icon; the blinking cursor
 * follows the printed text's end. */
// FUNCTION: LEGOLAND 0x00491bd0
void EnterNewProfile(Icon* panel)
{
    char cursor[2] = "|";
    unsigned char len = g_temp_profile.len;
    char c;
    WinRect rc;
    int cx;
    int mid;

    c = GetInputChar();
    if (c != 0) {
        if (c == -1 && len != 0) {
            len--;
            g_temp_profile.name[len] = 0;
        }
        if (len < 0x1f && g_profile_name_len < 0x7b && c > 0) {
            if (c == ' ') {
                if (len != 0) {
                    g_temp_profile.name[len++] = c;
                    g_temp_profile.name[len] = 0;
                }
            } else {
                g_temp_profile.name[len++] = c;
                g_temp_profile.name[len] = 0;
            }
        }
    }
    rc.top = panel->y + 7;
    rc.left = panel->x + 0x14;
    rc.bottom = rc.top + 0x13;
    rc.right = rc.left + 0xc0;
    if (len != 0)
        cx = PrintTextGetEnd(g_temp_profile.name, 2, rc, 1);
    else
        cx = (rc.right + rc.left) >> 1;
    mid = (rc.right + rc.left) >> 1;
    g_profile_name_len = (cx - mid) * 2;
    rc.top = panel->y + 5;
    strcpy(cursor, GetBlink() ? g_str_cursor_bar : g_str_cursor_space);
    rc.left = cx;
    rc.right = cx + 0x64;
    PrintCursor(cursor, 2, rc, 1);
    g_temp_profile.len = len;
}

/* The saved-game name editor: the same keystroke handling (name under 31
 * characters and 0xcb pixels; NOTE the shipped test order: any positive
 * character appends, so the "space only after a character" branch is dead)
 * in the popup's text box 0x28 right / 0x24 below the panel; the OK / close
 * icons are reset to their plain sprites whenever the mouse leaves the OK
 * button's column or row. */
// FUNCTION: LEGOLAND 0x0048e550
void EnterSaveGameDetails(Icon* panel)
{
    char cursor[2] = "|";
    unsigned char len = g_temp_profile.len;
    char c;
    WinRect rc;
    int cx;
    int mid;
    Icon* p;
    IRect rc2;

    c = GetInputChar();
    if (c != 0) {
        if (c == -1 && len != 0) {
            len--;
            g_temp_profile.name[len] = 0;
        }
        if (len < 0x1f && g_saved_game_len < 0xcb) {
            if (c > 0) {
                g_temp_profile.name[len++] = c;
                g_temp_profile.name[len] = 0;
            } else if (c == ' ' && len != 0) {
                g_temp_profile.name[len++] = c;
                g_temp_profile.name[len] = 0;
            }
        }
    }
    rc.top = panel->y + 0x24;
    rc.left = panel->x + 0x28;
    rc.bottom = rc.top + 0x11;
    rc.right = rc.left + 0xd7;
    if (len != 0)
        cx = PrintTextGetEnd(g_temp_profile.name, 2, rc, 1);
    else
        cx = (rc.right + rc.left) >> 1;
    mid = (rc.right + rc.left) >> 1;
    g_saved_game_len = (cx - mid) * 2;
    rc.top = panel->y + 0x20;
    strcpy(cursor, GetBlink() ? g_str_cursor_bar : g_str_cursor_space);
    rc.left = cx;
    rc.right = cx + 0x64;
    PrintCursor(cursor, 2, rc, 1);
    g_temp_profile.len = len;
    p = g_np_icon_ok;
    rc2.t = p->y;
    rc2.l = p->x;
    rc2.b = rc2.t + 0x1b;
    rc2.r = rc2.l + 0x48;
    if (rc2.r < g_gfx_point.x || g_gfx_point.x < rc2.l)
        ResetSavePopupIcons();
    if (rc2.b < g_gfx_point.y || g_gfx_point.y < rc2.t)
        ResetSavePopupIcons();
}

/* =========================================================================
 *  Save-game popup
 * ========================================================================= */

/* OK (0xbd right) and close (0xe1 right) icons, 0x18 above the popup panel,
 * in group 7. Their input handlers become the two active icon handlers. */
// FUNCTION: LEGOLAND 0x0048e280
void InitNewSaveGamePOPUP(Icon* popup)
{
    g_fe_sprite_678 = LoadSprite(g_lls_pu_ok, 4);
    g_fe_sprite_674 = LoadSprite(g_lls_pu_ok_on, 4);
    g_fe_sprite_67c = LoadSprite(g_lls_regclose, 4);
    g_fe_sprite_680 = LoadSprite(g_lls_regclose_on2, 4);
    g_np_icon_ok = InsertIcon(popup->x + 0xbd, popup->y - 0x18, 7, g_fe_sprite_678);
    g_np_icon_ok->help_id = 0x2f;
    g_np_icon_ok->help = GetString(0x2f);
    g_np_icon_ok->flags |= 0x2000;
    g_np_icon_ok->flags |= 0x4002;
    g_np_icon_ok->input = SaveGameOkInput;
    g_icon_handler1 = g_np_icon_ok->input;
    g_np_close_icon = InsertIcon(popup->x + 0xe1, popup->y - 0x18, 7, g_fe_sprite_67c);
    g_np_close_icon->help_id = 4;
    g_np_close_icon->help = GetString(4);
    g_np_close_icon->flags |= 0x2000;
    g_np_close_icon->flags |= 0x4002;
    g_np_close_icon->input = SaveGameCloseInput;
    g_icon_handler2 = g_np_close_icon->input;
    g_7986f0 = 0;
}

/* =========================================================================
 *  Exit checkbox (option screen)
 * ========================================================================= */

/* The "really exit?" popup at (x, y): the big panel (with the "save first"
 * text) when leaving the game with more than a minute of unsaved play,
 * else the small info panel; OK at (+0x7d, +0xdc / +0x78) and close beside
 * it, all in group 0xe. */

/* WIP -- our 116 instructions are the original's 119 minus exactly
 * `push ebx` / `xor ebx,ebx` / `pop ebx`: the original parks its constant zero
 * in ebx (`mov [798754],ebx` = g_exit_big_popup, `mov [7cb318],ebx`,
 * `mov [7cb310],ebx`) where we emit three `mov dword ptr [..],0` immediates.
 * Nothing else differs - insert those three and the whole body lines up.
 *
 * MEASURED THIS PASS (probe programs in scratchpad/oldwips/z*.c):
 *  - VC6 hoists a constant zero into a callee-saved register at FOUR OR MORE
 *    live uses, never at three.  Three dword stores (what we have) always come
 *    out as immediates, whatever their placement, however many calls separate
 *    them, and whether they are written as literals or through an `int` /
 *    `char` carrier (the carrier is constant-propagated away).  A dead store
 *    does not count: it is removed before the decision.
 *  - The register is ESI unless the zero has a live BYTE use, in which case it
 *    is EBX.  A word (16-bit) store, a `push 0` argument, an `== 0` compare, an
 *    array-index use and a byte store of a DIFFERENT value all still give esi;
 *    only `*(char*)&g = 0` (a live byte store of the zero itself) gives ebx.
 *  - Both facts reproduce inside this function: adding any fourth dword zero
 *    store (`g_7986f0 = 0;`) gives 120 instructions that match the original
 *    everywhere except esi-for-ebx and the extra store (9 mismatches);
 *    replacing that fourth store with `*(char*)&g_7986f0 = 0;` produces
 *    `push ebx` / `xor ebx,ebx` and makes indices 0..113 match EXACTLY
 *    (5 mismatches, all from the extra byte store at the tail).
 *
 * So the original had a fourth use of the zero and at least one of its uses
 * was byte-class - yet the disassembly contains no byte store, no `bl`/`bh`
 * operand and no fourth zero store anywhere in 0x48f0f0..0x48f2c4.  Every
 * "free" byte use tried is removed before the count is taken: a dead
 * `*(char*)&g = 0` killed by a following dword store, a byte field of an
 * unused local struct, `if (0) ...`, a char local, a char inline parameter,
 * and duplicate/adjacent stores.
 *
 * Also worth knowing: this is the ONLY function in legoland.exe whose sole
 * callee-saved push is `push ebx` together with `xor ebx,ebx` - every other
 * zero-in-ebx site (Create3DPerson 0x43f8c0, LLIDB_LoadODFData 0x47bf70,
 * PrintSprite 0x4853a0, InitGameInterface 0x4749d0 ...) also pushes esi and/or
 * edi, i.e. there the zero got ebx because esi was already taken.  Either this
 * function's source has a byte-class zero whose store VC6 deleted after
 * register allocation, or something forced a second callee-saved value that
 * later evaporated.  Duplicating the small-panel block into both arms hoping
 * for a cross-jump does NOT work - VC6 keeps both copies (126 instructions). */
// WIP-FUNCTION: LEGOLAND 0x0048f0f0  (116/119 insns; only push ebx / xor ebx,ebx / pop ebx missing - see note)
void InitExitCheckBox(int x, int y)
{
    Icon* panel;
    short dy;

    SaveOptionIconHandlers();
    g_fe_sprite_678 = LoadSprite(g_lls_pu_ok, 4);
    g_fe_sprite_674 = LoadSprite(g_lls_pu_ok_on, 4);
    g_fe_sprite_67c = LoadSprite(g_lls_pu_close, 4);
    g_fe_sprite_680 = LoadSprite(g_lls_pu_close_on, 4);
    g_fe_sprite_684 = LoadSprite(g_lls_pu_close, 4);
    g_fe_sprite_688 = LoadSprite(g_lls_pu_close_on, 4);
    if (g_screen_mode == 5) {
        g_icon_handler1 = ExitBigOkInput;
        if (TimeSinceSave() > 60000) {
            g_exit_big_popup = 1;
            panel = LoadSpriteIcon(g_lls_pu_bigpopup, 4, x, y, 0xe);
            dy = 0xdc;
            goto have_panel;
        }
    } else {
        g_icon_handler1 = ExitOkInput;
    }
    g_exit_big_popup = 0;
    panel = LoadSpriteIcon(g_lls_pu_infopopup, 4, x, y, 0xe);
    dy = 0x78;
have_panel:
    g_np_icon_ok = InsertIcon(panel->x + 0x7d, panel->y + dy, 0xe, g_fe_sprite_678);
    g_np_icon_ok->help_id = 0x4e;
    g_np_icon_ok->help = GetString(0x4e);
    g_np_icon_ok->flags |= 0x2000;
    g_np_icon_ok->flags |= 0x4002;
    g_np_icon_ok->input = g_icon_handler1;
    g_np_close_icon = InsertIcon(g_np_icon_ok->x + 0x24, g_np_icon_ok->y, 0xe, g_fe_sprite_67c);
    g_np_close_icon->help_id = 4;
    g_np_close_icon->help = GetString(4);
    g_np_close_icon->flags |= 0x2000;
    g_np_close_icon->flags |= 0x4002;
    g_np_close_icon->input = ExitCloseInput;
    g_exit_7cb318 = 0;
    g_exit_7cb310 = 0;
    g_icon_handler2 = ExitCloseInput;
}

/* Draws the exit popup's two text blocks (title in the 0x106..0x1b6 column,
 * then the question) and refreshes the OK / close icons. */
// FUNCTION: LEGOLAND 0x0048f2d0
void PrintExitCheckBox(void)
{
    WinRect rc;

    if (g_exit_popup_up) {
        PushRenderingStatusAndUnlockVideoSurface();
        rc.left = 0x106;
        rc.top = 0x2e;
        rc.right = 0x1b6;
        rc.bottom = 0x47;
        if (g_exit_7cb318)
            InfoPrintCent(strlen(GetString(0x88)), GetString(0x88), 1, rc, 1);
        else
            InfoPrintCent(strlen(GetString(0x89)), GetString(0x89), 2, rc, 1);
        rc.left = 0x106;
        rc.top = 0x4b;
        rc.right = 0x1b6;
        rc.bottom = 0xae;
        if (g_exit_7cb318) {
            InfoPrintCent(strlen(GetString(0x8a)), GetString(0x8a), 1, rc, 1);
        } else if (g_exit_big_popup) {
            rc.bottom = 0x112;
            InfoPrintCent(strlen(GetString(0x884)), GetString(0x884), 1, rc, 1);
        } else {
            InfoPrintCent(strlen(GetString(0x8b)), GetString(0x8b), 1, rc, 1);
        }
        PopRenderingStatus();
        UpdateProfileCheckBoxIcons();
    }
}

/* =========================================================================
 *  Title screen
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x0048fc40
void InitTitleScreen(void)
{
    Icon* p;

    UnLoadInGameIcons();
    g_backdrop = LoadSprite(g_lls_title_bk, 0);
    p = LoadSpriteIcon(g_lls_new_on_title, 4, 0xca, 0x138, 7);
    p->help_id = 7;
    p->help = GetString(7);
    p->flags |= 0x6002;
    p->input = TitleNewInput;
    if (!HaveCurrentProfile()) {
        p->u18.owner = LoadSprite(g_lls_dark_new_on_title, 4);
        p->render = RenderDarkTitleIcon;
        p->flags |= 8;
    } else {
        p->u18.owner = 0;
    }
    g_icon_handler1 = TitleNewInput;
    p = LoadSpriteIcon(g_lls_free_on_title, 4, 0x9a, 8, 7);
    p->help_id = 8;
    p->help = GetString(8);
    p->flags |= 0x6002;
    p->input = TitleFreeInput;
    if (!HaveCurrentProfile())
        p->flags |= 0x400;
    p = LoadSpriteIcon(g_lls_reg_on_title, 4, 0x18, 0x71, 7);
    p->help_id = 0xa;
    p->help = GetString(0xa);
    p->flags |= 0x6002;
    p->input = TitleRegInput;
    p = LoadSpriteIcon(g_lls_exit_on_title, 4, 0x1e1, 0x13, 7);
    p->help_id = 0x3e8;
    p->help = GetString(0x3e8);
    p->flags |= 0x6002;
    p->input = TitleExitInput;
    g_icon_handler2 = TitleExitInput;
    p = LoadSpriteIcon(g_lls_load_on_title, 4, 0x19, 0x118, 7);
    p->help_id = 9;
    p->help = GetString(9);
    p->flags |= 0x6002;
    p->input = TitleLoadInput;
    p = LoadSpriteIcon(g_lls_movie_on_title, 4, 0x10e, 0xa1, 7);
    p->help_id = 0x2bf;
    p->help = GetString(0x2bf);
    p->flags |= 0x6002;
    p->input = TitleMovieInput;
    g_screen_popup = 0x898;
}

/* =========================================================================
 *  Option screen
 * ========================================================================= */

// FUNCTION: LEGOLAND 0x0048eb60
void InitOptionScreen(void)
{
    Icon* p;
    int mx;

    PauseGameTimer();
    g_opt_speech = (char)g_vol_speech;
    g_opt_music = (char)g_vol_music;
    g_opt_sfx = (char)g_vol_sfx;
    g_backdrop = LoadSprite(g_lls_option_bk, 0);
    p = LoadSpriteIcon(g_lls_accept_on_options, 4, 0x13, 0x14f, 7);
    p->help_id = 0x3c;
    p->help = GetString(0x3c);
    p->flags |= 0x6002;
    p->input = OptionAcceptInput;
    g_icon_handler1 = 0;
    p = LoadSpriteIcon(g_lls_exit_on_options, 4, 0x1e0, 6, 7);
    p->help_id = 0x3e;
    p->help = GetString(0x3e);
    p->flags |= 0x6002;
    p->input = TitleExitInput;
    g_icon_handler2 = TitleExitInput;
    p = LoadSpriteIcon(g_lls_load_on_options, 4, 0x168, 0x13a, 7);
    p->help_id = 0x3f2;
    p->help = GetString(0x3f2);
    p->flags |= 0x6002;
    p->input = OptionLoadInput;
    p = LoadSpriteIcon(g_lls_save_on_options, 4, 0x1df, 0xbd, 7);
    p->help_id = 0x3f;
    p->help = GetString(0x3f);
    p->flags |= 0x6002;
    p->input = OptionSaveInput;
    SaveOptionIconHandlers();
    p = LoadSpriteIcon(g_lls_down1, 4, 0x19c, 0x2e, 7);
    p->help_id = 0x46;
    p->help = GetString(0x46);
    p->flags |= 0x6002;
    p->input = VolDownInput;
    p->u18.control = 1;
    p = LoadSpriteIcon(g_lls_up1, 4, 0x26, 0x2e, 7);
    p->help_id = 0x45;
    p->help = GetString(0x45);
    p->flags |= 0x6002;
    p->input = VolUpInput;
    p->u18.control = 2;
    mx = VolumeToMarkerX(g_vol_speech);
    p = LoadSpriteIcon(g_lls_vol_speech, 4, mx, 0x33, 7);
    p->help_id = 0x47;
    p->help = GetString(0x47);
    p->flags |= 0x6002;
    p->input = VolMarkerInput;
    p->u18.control = 3;
    g_vol_marker_speech = p;
    p = LoadSpriteIcon(g_lls_down2, 4, 0x19c, 0x72, 7);
    p->help_id = 0x49;
    p->help = GetString(0x49);
    p->flags |= 0x6002;
    p->input = VolDownInput;
    p->u18.control = 4;
    p = LoadSpriteIcon(g_lls_up2, 4, 0x26, 0x72, 7);
    p->help_id = 0x48;
    p->help = GetString(0x48);
    p->flags |= 0x6002;
    p->input = VolUpInput;
    p->u18.control = 5;
    mx = VolumeToMarkerX(g_vol_music);
    p = LoadSpriteIcon(g_lls_vol_music, 4, mx, 0x77, 7);
    p->help_id = 0x4a;
    p->help = GetString(0x4a);
    p->flags |= 0x6002;
    p->input = VolMarkerInput;
    p->u18.control = 6;
    g_vol_marker_music = p;
    p = LoadSpriteIcon(g_lls_down3, 4, 0x19c, 0xb6, 7);
    p->help_id = 0x4c;
    p->help = GetString(0x4c);
    p->flags |= 0x6002;
    p->input = VolDownInput;
    p->u18.control = 7;
    p = LoadSpriteIcon(g_lls_up3, 4, 0x26, 0xb6, 7);
    p->help_id = 0x4b;
    p->help = GetString(0x4b);
    p->flags |= 0x6002;
    p->input = VolUpInput;
    p->u18.control = 8;
    mx = VolumeToMarkerX(g_vol_sfx);
    p = LoadSpriteIcon(g_lls_vol_fx, 4, mx, 0xbb, 7);
    p->help_id = 0x4d;
    p->help = GetString(0x4d);
    p->flags |= 0x6002;
    p->input = VolMarkerInput;
    p->u18.control = 9;
    g_vol_marker_fx = p;
    g_exit_popup_up = 0;
    g_exit_big_popup = 0;
}
