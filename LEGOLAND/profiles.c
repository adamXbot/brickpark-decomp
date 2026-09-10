/* LEGOLAND — player profiles, the saved-game list and save-file chunk framing.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Only struct
 * field offsets, callee argument counts and global addresses are load-bearing;
 * names are ours. Types are defined LOCALLY on purpose (legoland.h is owned
 * elsewhere; saveprof.c carries the same 0x110-byte Profile record).
 *
 * Codegen levers this file settled (all verified against the original):
 *  - A `char`-returning `x ? 1 : -1` / `x ? 0 : -1` is only lowered at BYTE
 *    width (`neg eax / sbb al,al / and al,2 / dec al`) when the ternary is
 *    first assigned to a `char` local; `return (char)(..)` and char-cast
 *    operands both stay dword.
 *  - An `unsigned char` loop counter is widened by VC6 into an int induction
 *    variable plus a separate trip counter (`mov esi,8 / mov ebx,esi ...
 *    dec esi / dec ebx / jne`; `for (i = 1; i < 8; i++)` -> esi=1, edi=7).
 *    An `int` counter gets no trip counter. When the byte is also passed to a
 *    `char` parameter it additionally keeps its home slot (`mov [esp+c],bl`
 *    ... `mov edx,[esp+10h] / push edx`).
 *  - `push esi` sinks past the directory guard when the FILE* lives in a
 *    post-guard block ending in ONE `rc = ..; return rc;` after an if/else;
 *    VC6 then duplicates that tail into both arms (SaveProfileToDisk).
 */
#include <stdio.h>
#include <string.h>
#include <io.h>
#include <direct.h>

#pragma intrinsic(strcpy, strlen, memcpy, memset)

/* ---- CRT ------------------------------------------------------------------
 * 0x0049e573 sprintf, 0x0049e5c5 printf, 0x0049f330 fopen, 0x0049f044 fread,
 * 0x004a069e fwrite, 0x0049efee fclose, 0x004a07a8 remove (DeleteFileA),
 * 0x004a07dd _mkdir (CreateDirectoryA), 0x0049e9ed _findfirst, 0x0049eab7
 * _findnext, 0x0049eb7c _findclose, 0x004aacbd _tell, 0x004a56c3 _lseek.
 * All plain CRT; the headers above give the prototypes. */

/* 0x0049e4ff / 0x0049e4d0 — the game's malloc / free wrappers. */
extern void* MemAlloc(unsigned int size);
extern void  MemFree(void* p);

/* 0x00453a20 DBPrintf (debug console). 0x00453ce0 is its sibling that the
 * save/load path reports failures through (SaveGame @0x47d8e0 uses it for
 * "cannot open" errors); it takes (fmt, ...) like DBPrintf. */
extern void DBPrintf(const char* fmt, ...);
extern void DBError(const char* fmt, ...);

/* ---- the profile record ---------------------------------------------------
 * 0x110 bytes, packed, and ALSO the on-disk record: "profiles\Profile%d.txt"
 * holds one of these per profile slot, and "profiles\%dsave%d.sh" (the
 * saved-game HEADER next to the "%dsave%d.sav" chunk file) holds one per save
 * slot — a snapshot of the profile at save time. The 15-byte region at +0x34 is
 * five fields (3 dwords, a word and a byte): UpDateCurrentProfile copies them
 * one at a time from the live profile, and AddNodeToProfileList copies the
 * group as one dword/dword/dword/word/byte struct assignment. */
#pragma pack(push, 1)
typedef struct ProfileStats {
    int            a;            /* +0x00 */
    int            b;            /* +0x04 */
    int            c;            /* +0x08 */
    unsigned short d;            /* +0x0c */
    unsigned char  e;            /* +0x0e */
} ProfileStats;                  /* 15 bytes */

typedef struct Profile {
    char           name[0x20];   /* +0x00  player name */
    int            f20;          /* +0x20 */
    unsigned char  f24;          /* +0x24  (byte: StoreNewSaveGameToDisk writes
                                  *        `mov byte ptr [7cad84h],al`) */
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

extern Profile g_temp_profile;   /* 0x007cad60 */

/* ---- the live ("current") profile @ 0x0080ffa0 -----------------------------
 * Same fields as the record but laid out differently: the three save/slot
 * bytes sit between the stats and the 200-byte block, and the +0x10b dword
 * lives at +0x30. Other lanes name +0x24/+0x28/+0x2c g_vol_speech /
 * g_vol_music / g_vol_sfx (0x80ffc4/c8/cc). +0x43 is the profile slot number
 * (1..8, the %d of "Profile%d.txt"); +0x44 the current save slot. */
#pragma pack(push, 1)
typedef struct CurProfile {
    char           name[0x20];   /* +0x00  0x0080ffa0 */
    int            f20;          /* +0x20  0x0080ffc0 -> Profile.f20 */
    int            f24;          /* +0x24  0x0080ffc4 -> Profile.f28 */
    int            f28;          /* +0x28  0x0080ffc8 -> Profile.f2c */
    int            f2c;          /* +0x2c  0x0080ffcc -> Profile.f30 */
    int            tail;         /* +0x30  0x0080ffd0 -> Profile.tail (+0x10b) */
    ProfileStats   stats;        /* +0x34  0x0080ffd4 */
    unsigned char  profile_slot; /* +0x43  0x0080ffe3 */
    unsigned char  save_slot;    /* +0x44  0x0080ffe4 */
    unsigned char  f45;          /* +0x45  0x0080ffe5 -> Profile.f24 */
    char           block[200];   /* +0x46  0x0080ffe6 */
} CurProfile;
#pragma pack(pop)

extern CurProfile g_cur_profile; /* 0x0080ffa0 */

/* ---- the profile list -----------------------------------------------------
 * One 0x11c-byte node per profile slot (8 slots, walked by LoadProfilesFormDisk
 * from slot 8 down to 1, so slot 1 ends up at the head). Node = next pointer +
 * a Profile record + "loaded" flag + the slot number. DeleteProfileList
 * (listdel.c) frees them by the next pointer alone. */
typedef struct ProfileNode {
    struct ProfileNode* next;    /* +0x000 */
    Profile             p;       /* +0x004 */
    int                 valid;   /* +0x114  1 = file was present */
    unsigned char       slot;    /* +0x118 */
} ProfileNode;                   /* 0x11c */

extern ProfileNode* g_profile_list;   /* 0x00798890 */

/* 0x0048e0c0 (not exported): the saved-game list twin of AddNodeToProfileList,
 * onto g_savedgame_list @ 0x00798734. Same (valid, record, slot) contract. */
extern void AddNodeToSavedGameList(int valid, Profile* hdr, char slot);

/* ---- save-file chunk framing ---------------------------------------------- */
extern int SaveGameRead(void* buf, unsigned int n);          /* 0x0047d730 */
extern int SaveGameWrite(const void* buf, unsigned int n);   /* 0x0047d760 */
extern int g_savefile_fd;                                    /* 0x006691b0 */

/* The chunk directory being built while writing: a stack of file offsets, one
 * per open measured block. BeginMeasuredBlock pushes the offset of a 4-byte
 * length placeholder; EndMeasuredBlock pops it and back-patches the placeholder
 * with the END offset (an absolute file position, not a byte count). */
extern int g_chunk_offsets[];    /* 0x006691bc */
extern int g_chunk_depth;        /* 0x006691fc */

/* The element list of the loaded save-game map: an array of LLIDB element
 * pointers @ 0x00669200 with its count @ 0x006691b4 (the save file refers to
 * elements by index into this list). */
extern int*  g_elist;            /* 0x00669200 */
extern int   g_elist_count;      /* 0x006691b4 */

/* ---- sprites / icons (see panelui.c, iconui.c) ---------------------------- */
typedef struct Icon {
    struct Icon*   next;      /* +0x00 */
    void*          sprite;    /* +0x04 */
    int            f08;       /* +0x08 */
    short          x;         /* +0x0c */
    short          y;         /* +0x0e */
    short          w;         /* +0x10 */
    short          h;         /* +0x12 */
    unsigned short group;     /* +0x14 */
    char           pad16[0x28 - 0x16];
    int          (*render)(struct Icon*);          /* +0x28 */
    char         (*input)(struct Icon*, int);      /* +0x2c */
    int            f30;       /* +0x30 */
    unsigned int   flags;     /* +0x34 */
    char*          text;      /* +0x38  GetString(text_id) */
    int            text_id;   /* +0x3c */
} Icon;

extern void* LoadSprite(const char* name, int mode);                   /* 0x00497ab0 */
#ifndef LEGOLAND_PORTABLE
extern void  KillSprite(void* s);                                       /* 0x00497bd0 */
#else
extern int KillSprite(void* s);                                       /* 0x00497bd0 */
#endif
extern Icon* InsertIcon(short x, short y, int group, void* sprite);     /* 0x0046d6c0 */
extern char* GetString(int id);                                         /* 0x00498f50 */
extern char  NewProfileCloseInput(Icon* icon, int msg);                 /* 0x004920a0 */

/* The front-end checkbox / popup sprites @ 0x00798674.. and the new-profile
 * popup's close icon @ 0x007986dc (+ a companion @ 0x007986d8). */
extern void* g_fe_sprite_674;    /* 0x00798674 */
extern void* g_fe_sprite_678;    /* 0x00798678 */
extern void* g_fe_sprite_67c;    /* 0x0079867c  RegClose.lls */
extern void* g_fe_sprite_680;    /* 0x00798680  RegCloseON.lls */
extern void* g_fe_sprite_684;    /* 0x00798684  PU_ClosePopUp.lls */
extern void* g_fe_sprite_688;    /* 0x00798688  PU_ClosePopUpON.lls */
extern void* g_np_icon_extra;    /* 0x007986d8 */
extern Icon* g_np_close_icon;    /* 0x007986dc */
extern char (*g_active_input_cb)(Icon*, int);   /* 0x006687c0 */

/* ---- map teardown --------------------------------------------------------- */
#ifndef LEGOLAND_PORTABLE
extern void  LLIDB_UnLoadData(void* elem);      /* 0x0047d450 */
#else
extern int LLIDB_UnLoadData(void* elem);      /* 0x0047d450 */
#endif
extern void  ClearOverlays(void);               /* 0x00462ce0 */
extern void  ClearMapCells(void);                  /* 0x00463680 */
extern void  sub_4828f0(void);                  /* 0x004828f0 */
extern void* g_extra_elems[];                   /* 0x007fd660 */
extern int   g_extra_elem_count;                /* 0x007fdb84 */
extern void* g_terrain_elem;                    /* 0x00801410 */
extern void* g_terrain_elem_2;                  /* 0x00801404 */
extern int   g_map_loaded;                      /* 0x00667d50 */

/* ---- save path helpers ---------------------------------------------------- */
extern int  SaveGame(const char* path);         /* 0x0047d8e0 */
extern void SetWaitSpriteRect(int a, int b);           /* 0x00466360 */
extern void ClearWaitSprite(void);                   /* 0x004663c0 */
extern void ResetSaveTimer(void);               /* 0x0047f810: g_669204 = GetGameTimer() */
extern int  ReturnFrom_ProfileDir(void);        /* 0x004913e0: `return 1` */

/* String literals (.rdata; only the addresses are load-bearing). */
extern const char g_str_profiles[];         /* 0x004b9174 "profiles" */
extern const char g_fmt_sav_dir[];          /* 0x004b9164 "%s\\%dsave%d.sav" */
extern const char g_fmt_sh[];               /* 0x004bf2bc "profiles\\%dsave%d.sh" */
extern const char g_fmt_sh_dir[];           /* 0x004bf3a0 "%s\\%dsave%d.sh" */
extern const char g_fmt_sav[];              /* 0x004bf730 "profiles\\%dsave%d.sav" */
extern const char g_fmt_profile[];          /* 0x004bf718 "profiles\\Profile%d.txt" */
extern const char g_mode_r[];               /* 0x004bf2b8 "r" */
extern const char g_mode_wplus[];           /* 0x004beb70 "w+" */
extern const char g_msg_cannot_open[];      /* 0x004bf6fc "\ncannot open output file" */
extern const char g_msg_del_sav[];          /* 0x004bf3b0 "Failed to delete saved game %s\n" */
extern const char g_msg_del_sh[];           /* 0x004bf378 "Failed to delete saved game Header %s\n" */
extern const char g_msg_save_failed[];      /* 0x004bf360 "Failed to save game %s" */
extern const char g_msg_no_dir[];           /* 0x004bf33c "Failed to move to profile folder" */
extern const char g_msg_cannot_open_s[];    /* 0x004bf320 "\ncannot open output file %s" */
extern const char g_msg_hdr_failed[];       /* 0x004bf2fc "Failed to write to save header %s" */
extern const char g_msg_saved_ok[];         /* 0x004bf2f0 "Saved OK %s" */
extern const char g_lls_regclose[];         /* 0x004bf148 "RegClose.lls" */
extern const char g_lls_regclose_on[];      /* 0x004bf138 "RegCloseON.lls" */
extern const char g_lls_pu_close[];         /* 0x004bab78 "PU_ClosePopUp.lls" */
extern const char g_lls_pu_close_on[];      /* 0x004bab8c "PU_ClosePopUpON.lls" */

/* =========================================================================
 *  Save-file chunk framing
 * ========================================================================= */

/* Writes a 4-byte placeholder at the current position and remembers where it
 * went. Returns 1 on success. */
// FUNCTION: LEGOLAND 0x0047d790
int BeginMeasuredBlock(void)
{
    int placeholder;
    int pos = _tell(g_savefile_fd);

    placeholder = 0;
    if (pos == -1)
        return 0;
    g_chunk_offsets[g_chunk_depth++] = pos;
    return SaveGameWrite(&placeholder, 4) != 0;
}

/* Pops the innermost placeholder, back-patches it with the current (end)
 * offset and seeks back to the end. */
// FUNCTION: LEGOLAND 0x0047d800
int EndMeasuredBlock(void)
{
    int end = _tell(g_savefile_fd);

    if (end == -1)
        return 0;
    g_chunk_depth--;
    if (_lseek(g_savefile_fd, g_chunk_offsets[g_chunk_depth], 0) == -1)
        return 0;
    if (!SaveGameWrite(&end, 4))
        return 0;
    return _lseek(g_savefile_fd, end, 0) != -1;
}

/* [sic] export spelling. *pval holds an element pointer on entry; on return it
 * holds that element's index in the save-game element list, or -1. */
// FUNCTION: LEGOLAND 0x0047d880
int FindeIneList(int* pval)
{
    int i;

    for (i = 0; i < g_elist_count; i++) {
        if (*pval == g_elist[i]) {
            *pval = i;
            return 1;
        }
    }
    *pval = -1;
    return 0;
}

/* =========================================================================
 *  Profile directory
 * ========================================================================= */

/* Makes sure the "profiles" directory exists (creating it if not). Returns 1
 * when it is usable. Note the _findclose on a failed handle — as shipped. */
// FUNCTION: LEGOLAND 0x00491360
int Goto_ProfileDir(void)
{
    struct _finddata_t fd;
    long h;
    int isdir = 0;

    h = _findfirst(g_str_profiles, &fd);
    if (h != -1) {
        do {
            if (fd.attrib & _A_SUBDIR)
                isdir = 1;
        } while (_findnext(h, &fd) != -1);
    }
    _findclose(h);
    if (!isdir)
        return _mkdir(g_str_profiles) == 0;
    return 1;
}

/* Reads the saved-game header "profiles\<profile>save<slot>.sh" into the temp
 * profile record. */
// FUNCTION: LEGOLAND 0x0048d8f0
int LoadDateIntoTempProfile(int profile, int slot)
{
    char path[0x84];
    FILE* f;

    sprintf(path, g_fmt_sh, profile, slot);
    if (Goto_ProfileDir()) {
        f = fopen(path, g_mode_r);
        if (f) {
            fread(&g_temp_profile, 0x110, 1, f);
            fclose(f);
            ReturnFrom_ProfileDir();
            return 1;
        }
    }
    return 0;
}

/* Counts the profile files present (slots 1..7). Returns -1 if the directory
 * is unusable. */
// FUNCTION: LEGOLAND 0x004913f0
char ScanForProfiles(void)
{
    char path[0x78];
    unsigned char found = 0;

    if (!Goto_ProfileDir())
        return -1;
    {
        unsigned char i;
        FILE* f;

        for (i = 1; i < 8; i++) {
            sprintf(path, g_fmt_profile, i);
            f = fopen(path, g_mode_r);
            if (!f) {
                printf(g_msg_cannot_open);
            } else {
                found++;
                fclose(f);
            }
        }
    }
    if (!ReturnFrom_ProfileDir())
        return -1;
    return found;
}

/* Writes the temp profile record to the current profile's file. */
// FUNCTION: LEGOLAND 0x00491910
char SaveProfileToDisk(void)
{
    char path[0x78];

    if (!Goto_ProfileDir())
        return -1;
    {
        /* Post-guard scope: this is what lets VC6 sink `push esi` past the
         * directory check (and it then duplicates the shared tail into both
         * arms rather than jumping to it). */
        FILE* f;
        char rc;

        sprintf(path, g_fmt_profile, g_cur_profile.profile_slot);
        f = fopen(path, g_mode_wplus);
        if (!f) {
            printf(g_msg_cannot_open);
        } else {
            fwrite(&g_temp_profile, 0x110, 1, f);
            fclose(f);
        }
        rc = ReturnFrom_ProfileDir() ? 1 : -1;
        return rc;
    }
}

/* Deletes one save slot's .sav and .sh files of the current profile. */
// FUNCTION: LEGOLAND 0x0048ea10
void RemoveSaveGame(unsigned char slot)
{
    char path[0x84];

    if (!Goto_ProfileDir())
        return;
    sprintf(path, g_fmt_sav_dir, g_str_profiles, g_cur_profile.profile_slot, slot);
    if (remove(path))
        DBPrintf(g_msg_del_sav, path);
    sprintf(path, g_fmt_sh_dir, g_str_profiles, g_cur_profile.profile_slot, slot);
    if (remove(path))
        DBPrintf(g_msg_del_sh, path);
    ReturnFrom_ProfileDir();
}

/* Deletes a profile's file and all eight of its save slots. */
// FUNCTION: LEGOLAND 0x00491ab0
int RemoveProfile(unsigned char profile)
{
    char path[0x78];

    if (!Goto_ProfileDir())
        return -1;
    {
        unsigned char i;

        sprintf(path, g_fmt_profile, profile);
        remove(path);
        for (i = 8; i > 0; i--) {
            sprintf(path, g_fmt_sav, profile, i);
            remove(path);
            sprintf(path, g_fmt_sh, profile, i);
            remove(path);
        }
    }
    return ReturnFrom_ProfileDir() ? 0 : -1;
}

/* Front-end checkbox sprites teardown. */
// FUNCTION: LEGOLAND 0x0048c9a0
void KillFrontEndCheckBoxSprite(void)
{
    if (g_fe_sprite_674) {
        KillSprite(g_fe_sprite_674);
        g_fe_sprite_674 = 0;
    }
    if (g_fe_sprite_678) {
        KillSprite(g_fe_sprite_678);
        g_fe_sprite_678 = 0;
    }
    if (g_fe_sprite_680) {
        KillSprite(g_fe_sprite_680);
        g_fe_sprite_680 = 0;
    }
    if (g_fe_sprite_67c) {
        KillSprite(g_fe_sprite_67c);
        g_fe_sprite_67c = 0;
    }
    if (g_fe_sprite_684) {
        KillSprite(g_fe_sprite_684);
        g_fe_sprite_684 = 0;
    }
    if (g_fe_sprite_688) {
        KillSprite(g_fe_sprite_688);
        g_fe_sprite_688 = 0;
    }
}

/* Loads the new-profile popup's close-button sprites and inserts the close
 * icon 0xe1 right / 0x1e below the popup, with string 4 and the close
 * handler. */
// FUNCTION: LEGOLAND 0x0048c650
void EnterNewProfileCheckBoxIcons(Icon* popup)
{
    g_fe_sprite_67c = LoadSprite(g_lls_regclose, 4);
    g_fe_sprite_680 = LoadSprite(g_lls_regclose_on, 4);
    g_fe_sprite_684 = LoadSprite(g_lls_pu_close, 4);
    g_fe_sprite_688 = LoadSprite(g_lls_pu_close_on, 4);
    g_np_icon_extra = 0;
    g_np_close_icon = InsertIcon(popup->x + 0xe1, popup->y + 0x1e, 0xe, g_fe_sprite_684);
    g_np_close_icon->text_id = 4;
    g_np_close_icon->text = GetString(4);
    g_np_close_icon->flags |= 0x2000;
    g_np_close_icon->flags |= 0x4002;
    g_np_close_icon->input = NewProfileCloseInput;
    g_active_input_cb = g_np_close_icon->input;
}

/* Releases everything the loaded save-game map pulled in. */
// FUNCTION: LEGOLAND 0x0047f760
void UnloadSaveGameMap(void)
{
    int i;

    if (g_elist) {
        for (i = 0; i < g_elist_count; i++)
            LLIDB_UnLoadData((void*)g_elist[i]);
        MemFree(g_elist);
        g_elist = 0;
    }
    for (i = 0; i < g_extra_elem_count; i++)
        LLIDB_UnLoadData(g_extra_elems[i]);
    ClearMapCells();
    LLIDB_UnLoadData(g_terrain_elem);
    if (g_terrain_elem_2)
        LLIDB_UnLoadData(g_terrain_elem_2);
    ClearOverlays();
    sub_4828f0();
    g_map_loaded = 0;
}

/* =========================================================================
 *  Profile list
 * ========================================================================= */

/* Pushes a node for `slot` onto the profile list; with valid != 0 the record
 * is copied in (everything except +0x24). */
// FUNCTION: LEGOLAND 0x004919c0
void AddNodeToProfileList(int valid, Profile* p, char slot)
{
    ProfileNode* node = (ProfileNode*)MemAlloc(sizeof(ProfileNode));

    memset(node, 0, sizeof(ProfileNode));
    if (valid) {
        strcpy(node->p.name, p->name);
        node->p.f20 = p->f20;
        node->p.f28 = p->f28;
        node->p.f2c = p->f2c;
        node->p.f30 = p->f30;
        node->slot = slot;
        node->valid = 1;
        node->p.stats = p->stats;
        memcpy(node->p.block, p->block, sizeof(node->p.block));
        node->p.tail = p->tail;
        node->next = g_profile_list;
        g_profile_list = node;
    } else {
        node->slot = slot;
        node->next = g_profile_list;
        g_profile_list = node;
    }
}

/* Reads "profiles\Profile8.txt" .. "Profile1.txt" into the profile list. */
// FUNCTION: LEGOLAND 0x00491470
char LoadProfilesFormDisk(void)
{
    char rc;
    unsigned char slot;
    char path[0x78];
    Profile p;
    FILE* f;

    if (!Goto_ProfileDir())
        return -1;
    for (slot = 8; slot != 0; slot--) {
        sprintf(path, g_fmt_profile, slot);
        f = fopen(path, g_mode_r);
        if (!f) {
            printf(g_msg_cannot_open);
            AddNodeToProfileList(0, &p, slot);
        } else {
            fread(&p, 0x110, 1, f);
            AddNodeToProfileList(1, &p, slot);
            fclose(f);
        }
    }
    rc = ReturnFrom_ProfileDir() ? 0 : -1;
    return rc;
}

/* Reads the eight saved-game headers of `profile` into the saved-game list. */
// FUNCTION: LEGOLAND 0x0048e190
char LoadSavedGamesList(unsigned char profile)
{
    char rc;
    unsigned char slot;
    char path[0x78];
    Profile hdr;
    FILE* f;

    if (!Goto_ProfileDir())
        return -1;
    for (slot = 8; slot != 0; slot--) {
        sprintf(path, g_fmt_sh, profile, slot);
        f = fopen(path, g_mode_r);
        if (!f) {
            AddNodeToSavedGameList(0, &hdr, slot);
        } else {
            fread(&hdr, 0x110, 1, f);
            AddNodeToSavedGameList(1, &hdr, slot);
            fclose(f);
        }
        memset(&hdr, 0, sizeof(hdr));
    }
    rc = ReturnFrom_ProfileDir() ? 0 : -1;
    return rc;
}

/* Snapshots the live profile into a record and writes it to its file. */
// FUNCTION: LEGOLAND 0x00491680
char UpDateCurrentProfile(void)
{
    char rc;
    Profile p;
    char path[0x78];
    FILE* f;

    memset(&p, 0, sizeof(p));
    strcpy(p.name, g_cur_profile.name);
    p.f20 = g_cur_profile.f20;
    p.f28 = g_cur_profile.f24;
    p.f2c = g_cur_profile.f28;
    p.f30 = g_cur_profile.f2c;
    p.stats.a = g_cur_profile.stats.a;
    p.stats.b = g_cur_profile.stats.b;
    p.stats.c = g_cur_profile.stats.c;
    p.stats.d = g_cur_profile.stats.d;
    p.stats.e = g_cur_profile.stats.e;
    memcpy(p.block, g_cur_profile.block, sizeof(p.block));
    p.tail = g_cur_profile.tail;
    if (!Goto_ProfileDir())
        return -1;
    sprintf(path, g_fmt_profile, g_cur_profile.profile_slot);
    f = fopen(path, g_mode_wplus);
    if (!f) {
        printf(g_msg_cannot_open);
    } else {
        fwrite(&p, 0x110, 1, f);
        fclose(f);
    }
    rc = ReturnFrom_ProfileDir() ? 1 : -1;
    return rc;
}

/* Rewrites the current save slot's header with the live volume/flag fields. */
// FUNCTION: LEGOLAND 0x00491550
char UpDateCurrentSaveSlotInfo(void)
{
    char rc;
    Profile p;
    char path[0x78];
    FILE* f;

    memset(&p, 0, sizeof(p));
    if (!LoadDateIntoTempProfile(g_cur_profile.profile_slot, g_cur_profile.save_slot))
        return -1;
    strcpy(p.name, g_temp_profile.name);
    p.f28 = g_cur_profile.f24;
    p.f2c = g_cur_profile.f28;
    p.f30 = g_cur_profile.f2c;
    p.f24 = g_cur_profile.f45;
    if (!Goto_ProfileDir())
        return -1;
    sprintf(path, g_fmt_sh, g_cur_profile.profile_slot, g_cur_profile.save_slot);
    f = fopen(path, g_mode_wplus);
    if (!f) {
        printf(g_msg_cannot_open);
    } else {
        fwrite(&p, 0x110, 1, f);
        fclose(f);
    }
    rc = ReturnFrom_ProfileDir() ? 1 : -1;
    return rc;
}

/* Writes the .sav chunk file, then the .sh header snapshot next to it. */
// FUNCTION: LEGOLAND 0x0048e870
char StoreNewSaveGameToDisk(void)
{
    char shpath[0xc8];
    char savpath[0x100];
    FILE* f;

    sprintf(savpath, g_fmt_sav_dir, g_str_profiles, g_cur_profile.profile_slot,
            g_cur_profile.save_slot);
    SetWaitSpriteRect(0, 0);
    ResetSaveTimer();
    if (!SaveGame(savpath)) {
        DBError(g_msg_save_failed, savpath);
        g_cur_profile.save_slot = 0;
        remove(savpath);
        ClearWaitSprite();
        return -1;
    }
    ClearWaitSprite();
    g_temp_profile.f24 = g_cur_profile.f45;
    g_temp_profile.f20 = g_cur_profile.f20;
    g_temp_profile.f28 = g_cur_profile.f24;
    g_temp_profile.f2c = g_cur_profile.f28;
    g_temp_profile.f30 = g_cur_profile.f2c;
    sprintf(shpath, g_fmt_sh, g_cur_profile.profile_slot, g_cur_profile.save_slot);
    if (!Goto_ProfileDir()) {
        DBError(g_msg_no_dir);
        return -1;
    }
    f = fopen(shpath, g_mode_wplus);
    if (!f) {
        DBError(g_msg_cannot_open_s, shpath);
    } else {
        if (!fwrite(&g_temp_profile, 0x110, 1, f))
            DBError(g_msg_hdr_failed, shpath);
        fclose(f);
    }
    if (!ReturnFrom_ProfileDir())
        return -1;
    DBError(g_msg_saved_ok, shpath);
    return 1;
}
