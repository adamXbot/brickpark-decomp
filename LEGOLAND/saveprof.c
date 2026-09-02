/* LEGOLAND — save-game I/O and player-profile helpers.
 *
 * Reconstructed from original/legoland.exe (VC6 SP3, /O2 /Gy /Gd). Only
 * struct field offsets, callee argument counts and global addresses are
 * load-bearing; names are ours. Types are defined LOCALLY on purpose (the
 * shared header is owned elsewhere).
 */
#include <string.h>

/* ---- CRT ------------------------------------------------------------------
 * The save file is a plain CRT low-level file descriptor, not a RES archive
 * handle: the three callees are the VC6 CRT's _read (0x0049f4ca — it indexes
 * the _osfhnd / _osfile tables at 0x832c00 by fd>>5, fd&0x1f), _write
 * (0x004a63e4) and _tell (0x004aacbd = _lseek(fd, 0, SEEK_CUR)). */
extern int _read(int fd, void* buf, unsigned int n);
extern int _write(int fd, const void* buf, unsigned int n);

/* 0x006691b0 — the open save-game file descriptor. Neighbouring globals seen
 * in the un-exported helpers at 0x0047d790..: a table of file offsets
 * @ 0x006691bc indexed by a counter @ 0x006691fc (chunk directory being
 * built while writing). */
extern int g_savefile_fd;

/* -------------------------------------------------------------------------
 *  SaveGameRead (0x0047d730) / SaveGameWrite (0x0047d760)
 *
 * Both return 1 only when the WHOLE block was transferred. The `n` parameter
 * must be `unsigned int`: it is pushed straight through to _read/_write and
 * then compared with the byte count they return (`cmp eax,esi / sete dl`),
 * and the compiled shape (`xor edx,edx ... sete dl / mov eax,edx`) is the
 * relational-expression-as-return idiom.
 * ------------------------------------------------------------------------- */

// FUNCTION: LEGOLAND 0x0047d730
int SaveGameRead(void* buf, unsigned int n)
{
    return (unsigned int)_read(g_savefile_fd, buf, n) == n;
}

// FUNCTION: LEGOLAND 0x0047d760
int SaveGameWrite(const void* buf, unsigned int n)
{
    return (unsigned int)_write(g_savefile_fd, buf, n) == n;
}

/* -------------------------------------------------------------------------
 *  LoadObjectClass (0x00480b40)
 * ------------------------------------------------------------------------- */

/* The LLIDB element (same 20-byte record legoland.h calls LLElem): the
 * type/flags word is at +0x08. Bit 0 = "data loaded" (set by LLIDB_LoadData
 * itself, see llidb_load.c); bit 2 (0x4) is set HERE, and 0x004809d0 skips an
 * element whose bit 2 is already set (`test byte ptr [edi+8],4`), so 0x4 means
 * "loaded as an object class". */
typedef struct ClassElem {
    char*        name;        /* +0x00 */
    char*        image;       /* +0x04 */
    unsigned int type_flags;  /* +0x08 */
    void*        data;        /* +0x0c */
    unsigned int refcount;    /* +0x10 */
} ClassElem;

extern void* LLIDB_LoadData(ClassElem* elem);            /* 0x0047d3a0 */
/* 0x004809d0 (not exported): builds "<prefix><name>" into a stack buffer
 * (strcpy + strcat-ish rep movsd), looks the sibling up with ElemID and
 * LLIDB_LoadData's it unless its 0x4 bit is already set. Named for what it
 * does; the real name is unknown. */
extern void  LoadObjectClassSibling(ClassElem* elem);

/* Returns LLIDB_LoadData's result (the parsed class data, 0 on failure).
 * The sibling load happens unconditionally, even after a failed load. */
// FUNCTION: LEGOLAND 0x00480b40
void* LoadObjectClass(ClassElem* elem)
{
    void* data = LLIDB_LoadData(elem);

    if (data)
        elem->type_flags |= 4;
    LoadObjectClassSibling(elem);
    return data;
}

/* -------------------------------------------------------------------------
 *  The temp profile record (0x007cad60, 0x110 bytes)
 *
 * Exactly 0x110 bytes: LoadDateIntoTempProfile (0x0048d8f0) does
 * fread(&g_temp_profile, 0x110, 1, f) into it and LoadProfilesFormDisk reads
 * profile files with the same size, so this is ALSO the on-disk profile
 * record. The layout is packed: the 200-byte block starts at the odd offset
 * +0xa3 and the trailing dword sits at +0x10b.
 * ------------------------------------------------------------------------- */
#pragma pack(push, 1)
typedef struct Profile {
    char           name[0x1e];   /* +0x00  player name (0x00491540 tests name[0]) */
    unsigned char  f1e;          /* +0x1e */
    unsigned char  f1f;          /* +0x1f */
    int            f20;          /* +0x20  reset to 5 */
    int            f24;          /* +0x24  NOT touched by the reset */
    int            f28;          /* +0x28  reset to 0x4b (75) */
    int            f2c;          /* +0x2c  reset to 0x4b */
    int            f30;          /* +0x30  reset to 0x4b */
    char           text[15];     /* +0x34  zeroed wholesale: VC6 unrolls the
                                  *        15-byte memset as 3 dwords + word +
                                  *        byte, all from the memset's eax=0 */
    char           block[200];   /* +0x43  zeroed wholesale (50 dwords) */
    /* +0x10b: the original stores a ZERO DWORD here and then a 1 BYTE at the
     * same address (`mov dword ptr [7cae6bh],edx / mov byte ptr [7cae6bh],1`),
     * so the source must have viewed these four bytes two ways. A union is the
     * only C that produces both stores; which field is "real" is unknowable
     * from this function (nothing else in the binary references +0x10b). */
    union {
        int           all;
        unsigned char first;
    } tail;                      /* +0x10b */
    unsigned char  f10f;         /* +0x10f  NOT touched by the reset */
} Profile;
#pragma pack(pop)

extern Profile g_temp_profile;   /* 0x007cad60 */

/* 0x0048a780 (not exported) is a bare `ret` — an empty routine taking the
 * 200-byte block by address. Its body was compiled away (or was a debug hook),
 * so the name is a guess at its intent. */
extern void InitProfileBlock(char* block);

// FUNCTION: LEGOLAND 0x004912e0
void ResetTempProfile(void)
{
    g_temp_profile.name[0] = 0;
    g_temp_profile.f20 = 5;
    g_temp_profile.f1e = 0;
    g_temp_profile.f28 = 0x4b;
    g_temp_profile.f2c = 0x4b;
    g_temp_profile.f30 = 0x4b;
    memset(g_temp_profile.text, 0, sizeof(g_temp_profile.text));
    memset(g_temp_profile.block, 0, sizeof(g_temp_profile.block));
    g_temp_profile.tail.all = 0;
    g_temp_profile.tail.first = 1;
    InitProfileBlock(g_temp_profile.block);
}

/* -------------------------------------------------------------------------
 *  InitNewProfilePoPUp (0x00491290)
 * ------------------------------------------------------------------------- */

/* The icon record (see panelui.c for the full 0x40-byte layout): x/y are
 * signed shorts at +0x0c/+0x0e, the flags dword is at +0x34, and a string
 * pointer / string id pair sits at +0x38/+0x3c (EnterNewProfileCheckBoxIcons
 * fills the same pair for its own icon). */
typedef struct Icon {
    struct Icon*   next;      /* +0x00 */
    void*          sprite;    /* +0x04 */
    int            f08;       /* +0x08 */
    short          x;         /* +0x0c */
    short          y;         /* +0x0e */
    short          w;         /* +0x10 */
    short          h;         /* +0x12 */
    unsigned short group;     /* +0x14 */
    char           pad16[0x34 - 0x16];
    unsigned int   flags;     /* +0x34 */
    char*          text;      /* +0x38  GetString(text_id) */
    int            text_id;   /* +0x3c */
} Icon;

/* 0x0046d7b0: LoadSprite(name, mode) + InsertIcon(x, y, group, sprite). The x
 * and y parameters are `short`: the caller builds them with 16-bit arithmetic
 * (`mov cx,[eax+0xe] / sub cx,0x1b / push ecx`) and InsertIcon itself reads
 * them with `mov ax,word ptr [esp+..]`. */
extern Icon* LoadSpriteIcon(const char* name, int mode, short x, short y, int group);
extern char* GetString(int id);                          /* 0x00498f50 */
extern void  EnterNewProfileCheckBoxIcons(Icon* panel);  /* 0x0048c650 */

/* The popup's background sprite name @ 0x004bf6e4 (string contents live in
 * .rdata; only its address is load-bearing here). */
extern const char g_new_profile_popup_lls[];

/* Builds the "enter new profile" popup 0x1b pixels above the parent icon,
 * gives it string 0x50, marks it with flag 0x2000 (the same bit the checkbox
 * icons get), adds the checkbox icons and resets the temp profile. */
// FUNCTION: LEGOLAND 0x00491290
void InitNewProfilePoPUp(Icon* parent)
{
    Icon* panel = LoadSpriteIcon(g_new_profile_popup_lls, 4,
                                 parent->x, parent->y - 0x1b, 0x15);

    panel->text_id = 0x50;
    panel->text = GetString(0x50);
    panel->flags |= 0x2000;
    EnterNewProfileCheckBoxIcons(panel);
    ResetTempProfile();
}

/* -------------------------------------------------------------------------
 *  LoadSourceImage (0x00497300)
 * ------------------------------------------------------------------------- */

extern void* CreateSourceImage(const char* name, int kind);  /* 0x00497280 */
extern int   __BMPLoader(void* image);                       /* 0x0044e010 */
extern void  KillImage(void* image);                         /* 0x00497510 */

/* Allocates the image record (name copied inline after the 0x18-byte header)
 * and decodes the BMP into it; a decode failure frees the record again. The
 * null-record early return hands back the callee's own 0 (no `xor eax,eax`),
 * so it is written as `return image`, not `return 0`. */
// FUNCTION: LEGOLAND 0x00497300
void* LoadSourceImage(const char* name, int kind)
{
    void* image = CreateSourceImage(name, kind);

    if (!image)
        return image;

    if (!__BMPLoader(image)) {
        KillImage(image);
        return 0;
    }
    return image;
}

/* -------------------------------------------------------------------------
 *  UnLoad_PopUpInfo (0x00471450)
 * ------------------------------------------------------------------------- */

extern void RemoveIconGroup(int group);   /* 0x0046d520 */
/* 0x00471170 (not exported): guarded by the "popup info loaded" flag
 * @ 0x00668958, it KillSprite()s the popup's sprites @ 0x006688e0.. and
 * clears the flag. */
extern void KillPopUpInfoSprites(void);

/* The true extent of the original is exactly these four instructions
 * (18 bytes, 0x00471450..0x00471462, then nop padding to the next routine at
 * 0x00471470): the body ends in a void tail `jmp` to a routine that lies
 * BEFORE it and has no `ret` of its own. tools/audit.py certifies it
 * ([OK] 4i/18B, 0 mismatches) since it treats an unconditional jmp out of the
 * function as a terminator, but the shared tools/match.py (and so verify.py)
 * still walk to the first `ret`, run through the padding into 0x00471470 and
 * score it 64%. Held as WIP so verify.py stays green; promote once match.py
 * gains the same terminator rule (see docs/DECOMP.md, "Tail-jump functions").
 * Any non-tail C form emits call/ret and stops matching. */
// WIP-FUNCTION: LEGOLAND 0x00471450  (100% by audit.py; match.py cannot bound a tail-jmp function)
void UnLoad_PopUpInfo(void)
{
    RemoveIconGroup(0x2c3);
    KillPopUpInfoSprites();
}
