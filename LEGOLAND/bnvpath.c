/* LEGOLAND BNV path following and its immediate helpers. */
#include "legoland.h"
#include <math.h>

/* Scratch word used by the x87-to-integer conversion helper. */
extern int g_bnv_fist_scratch; /* 0x00667c3c */

typedef struct BNVBloke {
    struct BNVBloke* next;       /* +0x00 */
    void*            person;     /* +0x04 */
    unsigned char    pad08[0x34];
    short            map_x;      /* +0x3c */
    short            map_y;      /* +0x3e */
    unsigned char    pad40[0x34];
    unsigned char    walk_frame; /* +0x74 */
    unsigned char    walk_delay; /* +0x75 */
    unsigned char    pad76[9];
    unsigned char    speed;      /* +0x7f */
    unsigned char    pad80[0x2c];
} BNVBloke;

typedef union BNVFixed {
    float real;
    int   fixed;
} BNVFixed;

typedef struct BNVPerson {
    unsigned char pad00[0x34];
    int           slope;       /* +0x34 */
    float         height;      /* +0x38 */
    unsigned char pad3c[0x1c];
    BNVFixed orientation[9]; /* +0x58..+0x7b */
} BNVPerson;

typedef struct BNVOrientation {
    float m[9]; /* +0x00..+0x23 */
} BNVOrientation;

typedef struct BNVVertex BNVVertex;

typedef struct BNVNameNode {
    unsigned char       pad00[4];
    struct BNVNameNode* next; /* +0x04 */
    BNVVertex*          vertices; /* +0x08 */
    char*               name; /* +0x0c */
    float               orientation[9]; /* +0x10..+0x30 */
} BNVNameNode;

typedef struct BNVNameList {
    int          count; /* +0x00 */
    BNVNameNode* head;  /* +0x04 */
} BNVNameList;

struct BNVVertex {
    short         x; /* +0x00 */
    short         y; /* +0x02 */
    float         z; /* +0x04 */
    unsigned char pad08[12];
};

typedef struct BNVBin {
    unsigned short pad00;
    unsigned short frame_count; /* +0x02 */
    unsigned char  pad04[0x1c];
    BNVNameList*   frames; /* +0x20 */
} BNVBin;

typedef struct BNVPath {
    BNVBin*       bin;            /* +0x00 */
    int           tag;            /* +0x04 */
    char          object_name[20];/* +0x08 */
    float         vertical_scale; /* +0x1c */
    float         z_base;         /* +0x20 */
    float         x;              /* +0x24 */
    float         y;              /* +0x28 */
    float         pad2c;
    float         step_x;         /* +0x30 */
    float         step_y;         /* +0x34 */
    float         pad38;
    float         person_height;  /* +0x3c */
    int           dframe;         /* +0x40 */
    int           recalc;         /* +0x44 */
} BNVPath;

extern int NameCompare(const char* left, const char* right); /* 0x004aab90 */
extern BNVNameList* GetBinVFrame(BNVBin* bin, int frame); /* 0x0044dd70 */
extern BNVVertex* GetVertex(BNVNameNode* object, int index); /* 0x0044ddf0 */

/* 0x00458930 -- the ST(0) FLOAT-TO-INT HELPER, not a BNV routine at all.
 *
 * Established by scope PORT-B5 (an .text scan for `e8`/`e9` relative branches
 * to this address): it has **110 direct call sites and one tail `jmp`**,
 * spread right across the image -- 0x00401a17, the 0x00419xxx person/vector
 * block, 0x00422exx, tri3d.c's 0x004861xx / 0x004863xx / 0x0048650x, and
 * 0x0049bxxx.  Every one of them sits immediately after an x87 computation and
 * reads the result out of eax, e.g. at BuildChannelTables 0x00486135:
 *
 *     fild  dword ptr [esp+0xc]  /  fmul [0x4ab550]  /  fld st(0)
 *     fmul  dword ptr [0x4ab444] /  call 0x458930    /  mov di, ax
 *
 * So this is the compiler's `(int)<float>` lowering: a `__ftol`-shaped helper
 * that takes the value in ST(0), `fistp`s it into the fixed scratch dword at
 * 0x00667c3c and returns it in eax.  It differs from the stock VC6 `_ftol` in
 * one way that matters -- it does NOT save-and-reload the control word to set
 * truncation, so with the game's own nearest/masked CW it ROUNDS TO NEAREST.
 *
 * WHY THE PORTABLE ARM IS UNREACHABLE.  Nothing in the tree names it: the
 * callers are all source-level casts, which clang lowers to a native
 * conversion instruction rather than to a call, so no portable caller exists
 * and none can be written -- an ST(0) argument has no C signature.  The trap
 * below is therefore dead code in the portable build, and the census keeps
 * reporting it because there genuinely is no C body, not because one is owed.
 *
 * WHAT IS OWED, and it is a MATCHING question, not a porting one: a source
 * cast that lowers to this helper rounds, while C requires truncation, so any
 * site whose C spells `(int)f` is already describing the wrong arithmetic and
 * the portable build silently differs from the original by up to one unit.
 * `LL_FISTP` in ll_portable.h is the round-to-nearest spelling the ported arms
 * use where the asm was explicit; the 110 implicit sites have not been
 * audited.  Recorded in docs/lanes/scope-port-b5.md. */
// FUNCTION: LEGOLAND 0x00458930
int sub_458930(void)
{
#ifndef LEGOLAND_PORTABLE
    __asm fistp dword ptr [g_bnv_fist_scratch]
#else
    /* Unreachable: see the note above -- 110 matching-build call sites, none
     * of them nameable from C. */
    LL_UNPORTED_ASM();
#endif
    return g_bnv_fist_scratch;
}

// FUNCTION: LEGOLAND 0x0044dda0
BNVNameNode* GetObjectFromName(BNVNameList* list, const char* name)
{
    if (list == 0)
        return 0;

    {
        BNVNameNode* node = list->head;
        int i;

        for (i = 0; i < list->count; ++i) {
            if (NameCompare(name, node->name) == 0)
                break;
            node = node->next;
        }
        return node;
    }
}

// FUNCTION: LEGOLAND 0x00483830
void sub_483830(BNVBloke* bloke)
{
    if (--bloke->walk_delay == 0) {
        bloke->walk_delay = 2;
        bloke->walk_frame = (unsigned char)((bloke->walk_frame + 1) & 7);
    }
}

// FUNCTION: LEGOLAND 0x00484950
void ApplyObjectOrientationToPerson(BNVPerson* person,
                                    BNVOrientation* orientation,
                                    int unused)
{
    float scale = 65536.0f;
    float value;

    person->orientation[0].real = orientation->m[0];
    person->orientation[1].real = orientation->m[6];
    person->orientation[2].real = -orientation->m[3];
    person->orientation[3].real = -orientation->m[2];
    person->orientation[4].real = -orientation->m[8];
    person->orientation[5].real = orientation->m[5];
    person->orientation[6].real = -orientation->m[1];
    person->orientation[7].real = -orientation->m[7];
    person->orientation[8].real = orientation->m[4];

    value = person->orientation[0].real;
#ifndef LEGOLAND_PORTABLE
    __asm fld value
    __asm fmul scale
    __asm fistp value
#else
    LL_FISTP_SCALE_INPLACE(value, scale);
#endif
    person->orientation[0].fixed = *(int*)&value;

    value = person->orientation[3].real;
#ifndef LEGOLAND_PORTABLE
    __asm fld value
    __asm fmul scale
    __asm fistp value
#else
    LL_FISTP_SCALE_INPLACE(value, scale);
#endif
    person->orientation[3].fixed = *(int*)&value;

    value = person->orientation[6].real;
#ifndef LEGOLAND_PORTABLE
    __asm fld value
    __asm fmul scale
    __asm fistp value
#else
    LL_FISTP_SCALE_INPLACE(value, scale);
#endif
    person->orientation[6].fixed = *(int*)&value;

    value = person->orientation[1].real;
#ifndef LEGOLAND_PORTABLE
    __asm fld value
    __asm fmul scale
    __asm fistp value
#else
    LL_FISTP_SCALE_INPLACE(value, scale);
#endif
    person->orientation[1].fixed = *(int*)&value;

    value = person->orientation[4].real;
#ifndef LEGOLAND_PORTABLE
    __asm fld value
    __asm fmul scale
    __asm fistp value
#else
    LL_FISTP_SCALE_INPLACE(value, scale);
#endif
    person->orientation[4].fixed = *(int*)&value;

    value = person->orientation[7].real;
#ifndef LEGOLAND_PORTABLE
    __asm fld value
    __asm fmul scale
    __asm fistp value
#else
    LL_FISTP_SCALE_INPLACE(value, scale);
#endif
    person->orientation[7].fixed = *(int*)&value;

    value = person->orientation[2].real;
#ifndef LEGOLAND_PORTABLE
    __asm fld value
    __asm fmul scale
    __asm fistp value
#else
    LL_FISTP_SCALE_INPLACE(value, scale);
#endif
    person->orientation[2].fixed = *(int*)&value;

    value = person->orientation[5].real;
#ifndef LEGOLAND_PORTABLE
    __asm fld value
    __asm fmul scale
    __asm fistp value
#else
    LL_FISTP_SCALE_INPLACE(value, scale);
#endif
    person->orientation[5].fixed = *(int*)&value;

    value = person->orientation[8].real;
#ifndef LEGOLAND_PORTABLE
    __asm fld value
    __asm fmul scale
    __asm fistp value
#else
    LL_FISTP_SCALE_INPLACE(value, scale);
#endif
    person->orientation[8].fixed = *(int*)&value;
}

// FUNCTION: LEGOLAND 0x00484cd0
int UpdateBlokeFromBNVPath(BNVBloke* bloke, BNVPath* path)
{
    struct UpdateLocals {
        float      sum_x;
        float      sum_y;
        float      sum_z;
        int        temp_i;
        BNVPerson* person;
    } local;
    BNVNameList* frame;
    BNVNameNode* object;
    BNVVertex* vertex;
    float delta_x;
    float delta_y;
    float delta_z;
    float inverse_length;
    float angle;
    int height;
    int i;
    int dframe = path->dframe;

    local.person = (BNVPerson*)bloke->person;
    local.sum_z = 0.0f;
    local.sum_x = 0.0f;
    local.sum_y = 0.0f;

    if (dframe == path->bin->frame_count)
        return 0;

    frame = GetBinVFrame(path->bin, dframe);
    object = GetObjectFromName(frame, path->object_name);
    for (i = 0; i < 8; ++i) {
        vertex = GetVertex(object, i);
        local.temp_i = vertex->x;
        local.sum_x += local.temp_i;
        local.temp_i = vertex->y;
        local.sum_y += local.temp_i;
        local.sum_z += vertex->z;
    }

    delta_x = (float)(local.sum_x * 0.125 - path->x);
    delta_y = (float)(local.sum_y * 0.125 - path->y);
    if (path->recalc == 0) {
        float speed;

        path->x += path->step_x;
        path->y += path->step_y;
        speed = (float)bloke->speed;
        if (delta_x * delta_x + delta_y * delta_y < speed * speed) {
            path->recalc = 1;
            dframe = ++path->dframe;
        }
    }

    if (dframe > 0)
        frame = GetBinVFrame(path->bin, dframe - 1);
    else
        frame = GetBinVFrame(path->bin, 0);
    object = GetObjectFromName(frame, path->object_name);
    inverse_length = 1.0f /
        (float)sqrt(object->orientation[0] * object->orientation[0] +
                    object->orientation[1] * object->orientation[1] +
                    object->orientation[2] * object->orientation[2]);
    object->orientation[0] *= inverse_length;
    object->orientation[1] *= inverse_length;
    object->orientation[2] *= inverse_length;
    object->orientation[3] *= inverse_length;
    object->orientation[4] *= inverse_length;
    object->orientation[5] *= inverse_length;
    object->orientation[6] *= inverse_length;
    object->orientation[7] *= inverse_length;
    object->orientation[8] *= inverse_length;
    ApplyObjectOrientationToPerson((BNVPerson*)bloke->person,
                                   (BNVOrientation*)object->orientation, 0);

    if (path->recalc != 0) {
        path->recalc = 0;
        local.sum_z = 0.0f;
        local.sum_x = 0.0f;
        local.sum_y = 0.0f;
        if (dframe == path->bin->frame_count)
            return 0;

        frame = GetBinVFrame(path->bin, dframe);
        object = GetObjectFromName(frame, path->object_name);
        for (i = 0; i < 8; ++i) {
            vertex = GetVertex(object, i);
            local.sum_x += vertex->x;
            local.sum_y += vertex->y;
            local.sum_z += vertex->z;
        }

        delta_x = (float)(local.sum_x * 0.125 - path->x);
        delta_y = (float)(local.sum_y * 0.125 - path->y);
        delta_z = (float)(local.sum_z * 0.125 - path->z_base);
#ifndef LEGOLAND_PORTABLE
        height = (int)(delta_z * path->vertical_scale + 8192.0f);
#else
        height = LL_FISTP(delta_z * path->vertical_scale + 8192.0f); /* PORT-M5 */
#endif
        ((BNVPerson*)bloke->person)->slope = height >> 8;
        angle = (float)atan2(delta_y, delta_x);
        path->step_x = (float)(cos(angle) * (float)bloke->speed * 0.25);
        path->step_y = (float)(sin(angle) * (float)bloke->speed * 0.25);
    }

    local.person->height = path->person_height + path->person_height;
#ifndef LEGOLAND_PORTABLE
    bloke->map_x = (short)(int)(path->x * 0.5f);
    bloke->map_y = (short)(int)(path->y * 0.5f);
#else
    /* PORT-M5: 0x00484fbc / 0x00484fce round, and a halving is exactly the
     * case where truncation and rounding differ on every odd coordinate. */
    bloke->map_x = (short)LL_FISTP(path->x * 0.5f);
    bloke->map_y = (short)LL_FISTP(path->y * 0.5f);
#endif
    sub_483830(bloke);
    return 1;
}

// FUNCTION: LEGOLAND 0x004850b0
void BNVPath_SetDFrame(BNVBloke* bloke, BNVPath* path, int dframe)
{
    BNVNameList* frame;
    BNVNameNode* object;
    BNVVertex* vertex;
    float sum_x;
    float sum_y;
    float sum_z;
    float delta_x;
    float delta_y;
    float delta_z;
    float angle;
    int height;
    int i;

    path->dframe = dframe;
    sum_z = 0.0f;
    sum_x = 0.0f;
    sum_y = 0.0f;
    path->recalc = 0;

    frame = GetBinVFrame(path->bin, dframe);
    object = GetObjectFromName(frame, path->object_name);
    for (i = 0; i < 8; ++i) {
        vertex = GetVertex(object, i);
        sum_x += vertex->x;
        sum_y += vertex->y;
        sum_z += vertex->z;
    }

    delta_x = (float)(sum_x * 0.125);
    delta_y = (float)(sum_y * 0.125);
    path->x = delta_x;
    path->y = delta_y;
    sum_z = 0.0f;
    sum_x = 0.0f;
    sum_y = 0.0f;
    path->recalc = 0;

    frame = GetBinVFrame(path->bin, ++dframe);
    object = GetObjectFromName(frame, path->object_name);
    for (i = 0; i < 8; ++i) {
        vertex = GetVertex(object, i);
        sum_x += vertex->x;
        sum_y += vertex->y;
        sum_z += vertex->z;
    }

    delta_x = (float)(sum_x * 0.125 - path->x);
    delta_y = (float)(sum_y * 0.125 - path->y);
    delta_z = (float)(sum_z * 0.125 - path->z_base);
#ifndef LEGOLAND_PORTABLE
    height = (int)(delta_z * path->vertical_scale + 8192.0f);
#else
    height = LL_FISTP(delta_z * path->vertical_scale + 8192.0f); /* PORT-M5 */
#endif
    ((BNVPerson*)bloke->person)->slope = height >> 8;
    angle = (float)atan2(delta_y, delta_x);
    path->step_x = (float)(cos(angle) * (float)bloke->speed * 0.25);
    path->step_y = (float)(sin(angle) * (float)bloke->speed * 0.25);
}
