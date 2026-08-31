/* LEGOLAND -- ride boarding and visitor-experience simulation. */
#include "legoland.h"
#include <string.h>

typedef struct RideObject RideObject;
typedef struct RideBloke RideBloke;
typedef struct RiderNode RiderNode;

struct RideObject {
    unsigned char pad00[4];
    void*         instances;         /* +0x04 */
    unsigned char pad08[4];
    int           base_x;            /* +0x0c */
    int           base_y;            /* +0x10 */
    unsigned char pad14[0x20 - 0x14];
    short         type;              /* +0x20 */
    unsigned char pad22[0x2e - 0x22];
    short         rider_capacity;    /* +0x2e */
    unsigned char pad30[4];
    short         ride_code;         /* +0x34 */
    short         base_attract;      /* +0x36 */
    short         target_trait;      /* +0x38 */
    unsigned char pad3a[0xc4 - 0x3a];
    void*         elem;              /* +0xc4 */
    unsigned char* visit_counts;     /* +0xc8 */
    RiderNode*    riders;            /* +0xcc */
};

struct RideBloke {
    RideBloke*    next;              /* +0x00 */
    void*         person;            /* +0x04 */
    unsigned char pad08[0x14 - 8];
    void*         current_item;      /* +0x14 */
    void*         previous_item;     /* +0x18 */
    unsigned char pad1c[0x60 - 0x1c];
    unsigned char action;            /* +0x60 */
    unsigned char pad61;
    unsigned short flags;            /* +0x62 */
    unsigned char pad64[4];
    int           x;                 /* +0x68 */
    int           y;                 /* +0x6c */
    unsigned short path_word;        /* +0x70 */
    unsigned char pad72[6];
    unsigned short stay_timer;       /* +0x78 */
    unsigned short mood;             /* +0x7a */
    unsigned short hunger;           /* +0x7c */
    unsigned char trait;             /* +0x7e */
    unsigned char speed;             /* +0x7f */
    unsigned char stat80;            /* +0x80 */
    unsigned char name_letter;       /* +0x81 */
    unsigned char pad82[6];
    void* favourite_ride1;           /* +0x88 */
    void* favourite_ride2;           /* +0x8c */
    void* favourite_ride3;           /* +0x90 */
    void* favourite_food;            /* +0x94 */
    unsigned char pad98[0xac - 0x98];
};

struct RiderNode {
    RiderNode*    next;              /* +0x00 */
    RiderNode*    prev;              /* +0x04 */
    RideBloke*    bloke;             /* +0x08 */
    unsigned short ride_id;          /* +0x0c */
    unsigned short pad0e;
    void*         person;            /* +0x10 */
};

typedef struct AttractionNode {
    struct AttractionNode* next;     /* +0x00 */
    RideObject*            item;     /* +0x04 */
    unsigned char          pad08[0x14 - 8];
    int                    score;    /* +0x14 */
} AttractionNode;

typedef struct RideTile {
    unsigned char x;
    unsigned char y;
} RideTile;

typedef struct RideInstance {
    struct RideInstance* next;       /* +0x00 */
    unsigned char        pad04[8];
    unsigned short       flags;      /* +0x0c */
    unsigned short       tile_key;   /* +0x0e */
} RideInstance;

typedef struct RideMapObject {
    unsigned char pad00[0x0c];
    RideObject*   object;            /* +0x0c */
} RideMapObject;

typedef struct RideAnim {
    int           frame_count;       /* +0x00 */
    int           seat_count;        /* +0x04 */
    unsigned char pad08[0x24 - 8];
    void**        seat_tracks;       /* +0x24 */
} RideAnim;

typedef struct RidePerson {
    unsigned char pad00[0x2c];
    int           ride_value;        /* +0x2c */
} RidePerson;

typedef struct BlokeSoundSource {
    int        kind;                 /* +0x00 */
    RideBloke* bloke;                /* +0x04 */
    int        pad08;
    int        pad0c;
} BlokeSoundSource;

extern AttractionNode* g_attraction_head; /* 0x00669248 */

extern signed char GetBlokeAgeGroup(RideBloke* bloke); /* 0x0044eb10 */
extern int GetBlokeNum(RideBloke* bloke);               /* 0x00482fb0 */
extern int GetBlokeCounter(RideObject* item, int index); /* 0x00480ee0 */
extern RideInstance* GetInstanceOfClass(RideObject* item, RideTile* tile);
extern Offset GetScreenCoordsForObject(void* instance, RideObject* item);
extern RiderNode* GetNthRider(int index, RideObject* item, void* instance);
extern RiderNode* FindFirstRider(RideObject* item, void* instance);
extern RiderNode* FindNextRider(RideObject* item, void* instance);
extern void PutOne3DBlokeOnRide(RideAnim* anim, int index, int frame,
                               void* person, int screen_x, int screen_y);
extern void UpdatePersonPos(void* person, RideBloke* bloke);
extern void* ElemID(const char* name);
extern unsigned int Rand_Max(unsigned int maximum);
extern int Rand_Tween(int low, int high);
extern void InitBlokeName(RideBloke* bloke);              /* 0x00482c60 */
extern void* RandomFavouriteRide(void);                   /* 0x0044e790 */
extern void* RandomFavouriteFood(void);                   /* 0x0044e890 */
extern void NewLongTermAction(RideBloke* bloke, int action);
extern void* HeapAlloc_w(unsigned int size);
extern void PutBlokeInList(RideObject* item, RiderNode* rider);
extern void BlokeWalkAnim(RideBloke* bloke);
extern void KillAllSamplesFromSource(BlokeSoundSource* source);
extern void RemoveBlokeFromList(RideObject* item, RiderNode* rider);
extern int CountBlokesAtRideID(RideObject* item, unsigned short* ride_id);
extern void HeapFree_w(void* pointer);
extern int IsFavouriteFood(RideBloke* bloke, void* elem);
extern int IsFavouriteAttraction(RideBloke* bloke, void* elem);
extern void ApplyMoodEvent(RideBloke* bloke, int event, int amount);
extern void IncrementBlokeCounter(RideObject* item, int index);
int CalculateRideCode(int trait, RideObject* item, int visits);
void RemoveBlokeFromRide(RideObject* item, RiderNode* rider);

extern void* g_cafe_brolly_elem;       /* 0x006661c0 */
extern void* g_entrance_elem;          /* 0x006661c4 */
extern int g_visitor_count;            /* 0x006661bc */
extern unsigned int g_park_metric;     /* 0x00832918 */
extern int g_initial_hunger_max;       /* 0x004b8338 */
extern signed char g_next_name_letter; /* 0x004b8344 */

// FUNCTION: LEGOLAND 0x00441a60
void Put3DBlokesOnRide(RideObject* item, void* instance, int frame,
                       RideAnim* anim)
{
    Offset screen = GetScreenCoordsForObject(instance, item);
    int i;

    for (i = 0; i < anim->seat_count; ++i) {
        RiderNode* rider = GetNthRider(i, item, instance);
        if (rider && (rider->bloke->flags & 0x80))
            PutOne3DBlokeOnRide(anim, i, frame, rider->person,
                               screen.ox, screen.oy);
    }
}

// FUNCTION: LEGOLAND 0x00441ad0
void Put3DBlokesOnRide2(RideObject* item, void* instance)
{
    RiderNode* rider = FindFirstRider(item, instance);
    RideBloke* bloke;
    unsigned char mask;

    if (!rider)
        goto done;
    bloke = rider->bloke;
    mask = 0x80;
    while (bloke->flags & mask) {
        rider = FindNextRider(item, instance);
        if (!rider)
            goto done;
        bloke = rider->bloke;
    }
    if (!rider)
        goto done;
update:
    if (rider->person)
        UpdatePersonPos(rider->person, rider->bloke);
    rider = FindNextRider(item, instance);
    if (!rider)
        goto done;
    if (!(rider->bloke->flags & mask))
        goto repeat;
skip_second:
    rider = FindNextRider(item, instance);
    if (!rider)
        goto done;
    if (rider->bloke->flags & mask)
        goto skip_second;
repeat:
    if (rider)
        goto update;
done:
    return;
}

// FUNCTION: LEGOLAND 0x00442fa0
void Ride_SetFlagToNotLetAnyoneOn(RideTile* tile)
{
    int x = tile->x;
    int y = tile->y;
    Cell* cell;
    RideInstance* instance;

    if (x < 0 || x >= g_map->width || y < 0 || y >= g_map->height)
        cell = 0;
    else
        cell = &g_map_rows[y][x];
    instance = GetInstanceOfClass(((RideMapObject*)cell->obj)->object, tile);
    if (instance)
        instance->flags |= 2;
}

// FUNCTION: LEGOLAND 0x00443000
void Ride_ClearFlagToNotLetAnyoneOn(RideTile* tile)
{
    int x = tile->x;
    int y = tile->y;
    Cell* cell;
    RideInstance* instance;

    if (x < 0 || x >= g_map->width || y < 0 || y >= g_map->height)
        cell = 0;
    else
        cell = &g_map_rows[y][x];
    instance = GetInstanceOfClass(((RideMapObject*)cell->obj)->object, tile);
    if (instance)
        instance->flags &= ~2;
}

// FUNCTION: LEGOLAND 0x0044e920
void InitBlokeAI(RideBloke* bloke)
{
    if (!g_cafe_brolly_elem)
        g_cafe_brolly_elem = ElemID("SHARK CAFE BROLLY");
    if (!g_entrance_elem)
        g_entrance_elem = ElemID("ENTRANCE 1");

    ++g_visitor_count;
    bloke->speed = (unsigned char)Rand_Tween(12, 24);
    bloke->stay_timer = (unsigned short)Rand_Max(g_park_metric);
    bloke->mood = (unsigned short)Rand_Tween(10, 50);
    bloke->trait = (unsigned char)(Rand_Tween(0, 140) - 20);
    bloke->stat80 = (unsigned char)Rand_Tween(5, 10);
    bloke->hunger = (unsigned short)Rand_Tween(0, g_initial_hunger_max);
    bloke->current_item = 0;
    bloke->previous_item = 0;
    bloke->name_letter = g_next_name_letter;
    ++g_next_name_letter;
    if (g_next_name_letter > 'Z')
        g_next_name_letter = 'A';

    InitBlokeName(bloke);
    bloke->favourite_ride1 = RandomFavouriteRide();
    bloke->favourite_ride2 = RandomFavouriteRide();
    bloke->favourite_ride3 = RandomFavouriteRide();
    bloke->favourite_food = RandomFavouriteFood();
    NewLongTermAction(bloke, 2);
}

// FUNCTION: LEGOLAND 0x0048a390
int GetAllBlokesOffRide(RideObject* item, unsigned short ride_id)
{
    RiderNode* rider = item->riders;

    if (rider) {
        RiderNode* next;
        unsigned short id = ride_id;

        do {
            next = rider->next;
            if (rider->ride_id == id) {
                RideBloke* bloke = rider->bloke;
                unsigned short flags = bloke->flags;
                if (!(flags & 0x40)) {
                    flags |= 8;
                    bloke->flags = flags;
                    ++bloke->action;
                }
            }
            rider = next;
        } while (rider);
    }
    return 1;
}

// FUNCTION: LEGOLAND 0x0049a0d0
void PutWorkerOnRide(RideBloke* bloke, Cell* cell)
{
    RiderNode* rider = (RiderNode*)HeapAlloc_w(sizeof(RiderNode));

    if (rider) {
        memset(rider, 0, sizeof(RiderNode));
        rider->bloke = bloke;
        rider->person = bloke->person;
        bloke->flags |= 0x20;
        rider->ride_id = *(unsigned short*)&cell->bx;
        PutBlokeInList(((RideMapObject*)cell->obj)->object, rider);
    }
}

// FUNCTION: LEGOLAND 0x0048a100
void RemoveBlokeFromRide(RideObject* item, RiderNode* rider)
{
    RideBloke* bloke = rider->bloke;

    RemoveBlokeFromList(item, rider);
    if (CountBlokesAtRideID(item, &rider->ride_id) == 0) {
        int x = *(unsigned char*)&rider->ride_id;
        int y = *((unsigned char*)&rider->ride_id + 1);
        Cell* cell;

        if (x < 0 || x >= g_map->width || y < 0 || y >= g_map->height)
            cell = 0;
        else
            cell = &g_map_rows[y][x];
        cell->flags &= ~4;
    }

    HeapFree_w(rider);
    bloke->flags &= ~0x20;

    if (item->type == 5) {
        if (IsFavouriteFood(bloke, item->elem)) {
            ApplyMoodEvent(
                bloke, 9,
                CalculateRideCode(
                    bloke->trait,
                    (RideObject*)((Elem*)bloke->current_item)->data,
                    GetBlokeCounter(
                        (RideObject*)((Elem*)bloke->current_item)->data,
                        GetBlokeNum(bloke))));
        } else {
            ApplyMoodEvent(
                bloke, 10,
                CalculateRideCode(
                    bloke->trait,
                    (RideObject*)((Elem*)bloke->current_item)->data,
                    GetBlokeCounter(
                        (RideObject*)((Elem*)bloke->current_item)->data,
                        GetBlokeNum(bloke))));
        }
        bloke->hunger = 0;
    } else {
        if (IsFavouriteAttraction(bloke, item->elem)) {
            ApplyMoodEvent(
                bloke, 11,
                CalculateRideCode(
                    bloke->trait,
                    (RideObject*)((Elem*)bloke->current_item)->data,
                    GetBlokeCounter(
                        (RideObject*)((Elem*)bloke->current_item)->data,
                        GetBlokeNum(bloke))));
        } else {
            ApplyMoodEvent(
                bloke, 12,
                CalculateRideCode(
                    bloke->trait,
                    (RideObject*)((Elem*)bloke->current_item)->data,
                    GetBlokeCounter(
                        (RideObject*)((Elem*)bloke->current_item)->data,
                        GetBlokeNum(bloke))));
        }
    }

    bloke->stay_timer += (unsigned short)(
        GetBlokeCounter((RideObject*)((Elem*)bloke->current_item)->data,
                        GetBlokeNum(bloke)) * 50);
    bloke->previous_item = bloke->current_item;
    IncrementBlokeCounter((RideObject*)((Elem*)bloke->current_item)->data,
                          GetBlokeNum(bloke));
    NewLongTermAction(bloke, 23);
}

// FUNCTION: LEGOLAND 0x0048a2e0
void RemoveAllBlokesFromRide(RideObject* item, RideTile tile)
{
    BlokeSoundSource source;
    int exit_x = item->base_x + tile.x;
    int exit_y = item->base_y + tile.y;
    RiderNode* rider = item->riders;
    int zero = 0;

    source.kind = 1;
    if (rider != 0) {
        RiderNode* next;
        unsigned short ride_id = *(unsigned short*)&tile;

        do {
            next = rider->next;
            if (rider->ride_id == ride_id) {
                RideBloke* bloke = rider->bloke;
                ((RidePerson*)bloke->person)->ride_value = zero;
                bloke->x = exit_x << 8;
                bloke->y = exit_y << 8;
                bloke->path_word = (unsigned short)zero;
                RemoveBlokeFromRide(item, rider);
                BlokeWalkAnim(bloke);
                bloke->flags &= ~0x80;
                source.bloke = bloke;
                KillAllSamplesFromSource(&source);
                zero = 0;
            }
            rider = next;
        } while (rider != 0);
    }
}

// FUNCTION: LEGOLAND 0x00481410
int CalculateViewRideCode(int trait, RideObject* item, int visits)
{
    int penalty;

    if (item->target_trait > trait)
        penalty = (item->target_trait - trait) * 3;
    else
        penalty = (trait - item->target_trait) * 3;

    penalty -= 10;
    if (penalty < 0)
        penalty = 0;
    else if (penalty > 100)
        penalty = 100;

    if (visits)
        return item->base_attract + (4 - (1 << visits)) * 25 - penalty;
    return item->base_attract + 100 - penalty;
}

// FUNCTION: LEGOLAND 0x00481480
int CalculateRideCode(int trait, RideObject* item, int visits)
{
    int penalty;

    if (item->target_trait > trait)
        penalty = (item->target_trait - trait) * 3;
    else
        penalty = (trait - item->target_trait) * 3;
    penalty -= 10;
    if (penalty < 0)
        penalty = 0;
    return (item->ride_code - penalty + 200) >> visits;
}

static __inline int CalculateAttractivenessFromBase(RideObject* item,
                                                     int visits, int penalty,
                                                     int age, int viewing)
{
    int score;

    if (visits)
        score = item->base_attract + (4 - (1 << visits)) * 25 - penalty;
    else
        score = item->base_attract + 100 - penalty;

    switch (age) {
    case 0:
        if (item->type == 5)
            return -100;
        break;
    case 1:
        if (item->type == 5 && viewing == 0)
            return -100;
        break;
    case 2:
        if (item->type == 5) {
            if (score < 20)
                score = 20;
            if (viewing)
                return score * 2;
        }
        break;
    case 3:
    case 4:
        if (item->type == 5) {
            if (score < 20)
                score = 50;
            score *= viewing ? 6 : 4;
            break;
        }
        score = 0;
        break;
    }
    return score;
}

// FUNCTION: LEGOLAND 0x004814c0
int Calc_Item_Attractiveness(RideObject* item, RideBloke* bloke, int viewing)
{
    int trait;
    int age;
    int visits;
    int target;
    int penalty;

    trait = bloke->trait;
    age = GetBlokeAgeGroup(bloke);
    target = item->target_trait;
    visits = GetBlokeCounter(item, GetBlokeNum(bloke));
    if (target > trait)
        penalty = target - trait;
    else
        penalty = (trait - target) * 2;
    penalty -= 10;
    if (penalty < 0)
        penalty = 0;

    return CalculateAttractivenessFromBase(item, visits, penalty, age,
                                            viewing);
}

// FUNCTION: LEGOLAND 0x004815e0
void CalculateRideCodes(RideBloke* bloke)
{
    AttractionNode* node = g_attraction_head;

    while (node) {
        node->score = Calc_Item_Attractiveness(node->item, bloke, 0);
        node = node->next;
    }
}
