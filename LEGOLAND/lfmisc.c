/* LEGOLAND -- small log-flume, boat, rider and walk-path helpers.
 * VC6 SP3 /O2 /Gy /Gd. Layouts are local and retain original edge cases.
 */
typedef struct Pos { int x, y; } Pos;
typedef struct Link { struct Link* next; } Link;
typedef struct PersonNameView { unsigned char pad00[0x84]; int sex; } PersonNameView;
typedef struct Bloke {
    int unused;
    PersonNameView* person;
    unsigned char pad08[0x2c];
    unsigned char walking;
    unsigned char pad35[3];
    unsigned short path_index;
    unsigned char pad3a[0x16];
    void* path;
    unsigned char pad54[0xc];
    unsigned char action;
    unsigned char pad61[0x22];
    unsigned char first_name, last_name;
} Bloke;
typedef struct Person { unsigned char pad00[0x50]; void* pose; } Person;
typedef struct Pump { unsigned char pad00[0xc]; struct Pump* next; } Pump;
typedef struct JcBoat {
    int unused, x, y, next_x, next_y;
    unsigned char pad14[0x3dc-0x14];
    int from, state;
} JcBoat;
typedef struct LFPiece {
    struct LFPiece* next;
    struct LFPiece* prev;
    struct LFPiece* fwd;
    unsigned char pad0c[0x14];
    void* def;
    void* run;
    struct LFPiece* parent;
    struct LFPiece* sub;
    struct LFPiece* end_a;
} LFPiece;
typedef struct LFBoat { unsigned char pad00[0x14]; LFPiece* piece; } LFBoat;
typedef struct LFQueue { void* path; Link* head; Link* tail; } LFQueue;
typedef struct RenderItem { int unused; LFBoat* boat; struct RenderItem* next; } RenderItem;
typedef struct RenderList { RenderItem* head; } RenderList;
typedef struct BsBoat { unsigned char pad00[0x3f0]; struct BsBoat* next; } BsBoat;

extern Pump* g_pump_list;                                      /* 0x004cbea4 */
extern BsBoat* g_bs_boats;                                     /* 0x004cc03c */
extern void* g_lfdr_def;                                       /* 0x004c8d6c */
extern void HeapFree_w(void* ptr);                             /* 0x0049e4d0 */
extern int rand(void);                                        /* 0x0049e4b2 */
extern void Pump_Remove(Pump* pump);                           /* 0x00411b20 */
extern void JcBoat_Animate(JcBoat* boat, int from, int to);      /* 0x00433840 */
extern void GetTileDimensions(int* width, int* height);        /* 0x00460540 */
extern void LFBoat_Draw(LFBoat* boat, LFPiece* piece, int mode); /* 0x0040ae90 */

/* BuildWalkPath allocates the header and all points in one block. */
// FUNCTION: LEGOLAND 0x00412290
void FreeWalkPath(void* path)
{
    if (path) HeapFree_w(path);
}

/* Set a scripted walk path, reset its index and advance the action byte. */
// FUNCTION: LEGOLAND 0x004122d0
void WalkPath_Board(void* path, Bloke* bloke)
{
    bloke->path = path;
    bloke->path_index = 0;
    bloke->walking = 1;
    bloke->action++;
}

/* Resolve an animation's zero-based ordinal without checking the chain end. */
// FUNCTION: LEGOLAND 0x00412470
Link* LFAnim_FromId(Link* node, int index)
{
    Link* cursor = node;
    while (index-- != 0) cursor = cursor->next;
    return cursor;
}

/* Same unchecked ordinal lookup for a rider list. */
// FUNCTION: LEGOLAND 0x0042d540
Link* NthRiderNode(Link* node, int index)
{
    Link* cursor = node;
    while (index-- != 0) cursor = cursor->next;
    return cursor;
}

/* Return the node's ordinal, or the chain length if it is absent. */
// FUNCTION: LEGOLAND 0x004123a0
int LFAnim_SaveId(Link* node, Link* target)
{
    int index = 0;
    while (node) {
        if (node == target) break;
        node = node->next;
        index++;
    }
    return index;
}

/* Cache each next link before the per-pump destructor unlinks and frees it. */
// FUNCTION: LEGOLAND 0x00411bd0
void Pump_FreeAll(void)
{
    Pump* pump = g_pump_list;
    while (pump) {
        Pump* next = pump->next;
        Pump_Remove(pump);
        pump = next;
    }
}

/* Free the optional pose block, then the person; self is not null-checked. */
// FUNCTION: LEGOLAND 0x0043f870
void Free3DPerson(Person* person)
{
    if (person->pose) HeapFree_w(person->pose);
    HeapFree_w(person);
}

/* Leave the Jungle Cruise station, aiming five cells south in state 4. */
// FUNCTION: LEGOLAND 0x004333b0
void JcBoat_Depart(JcBoat* boat)
{
    JcBoat_Animate(boat, boat->from, 4);
    boat->state = 4;
    boat->next_y = boat->y + 5;
}

/* Preserve the doubled-size arithmetic and signed shifts of the original. */
// FUNCTION: LEGOLAND 0x00411290
Pos LFQuadTopLeft(void)
{
    int width, height;
    Pos out;
    GetTileDimensions(&width, &height);
    width <<= 1;
    height <<= 1;
    out.x = width >> 1;
    out.y = height >> 1;
    return out;
}

/* The boat's zero-based piece index inside the parent's drop sub-route. */
// FUNCTION: LEGOLAND 0x004117e0
int LFBoat_DropStep(LFBoat* boat)
{
    int step = 0;
    LFPiece* target = boat->piece;
    LFPiece* piece = target->parent->end_a;
    while (piece) {
        if (piece == target) return step;
        piece = piece->fwd;
        step++;
    }
    return -1;
}

/* Sub_411650: test whether the current piece belongs to LOG FLUME DROP. */
// FUNCTION: LEGOLAND 0x00411650
int LFBoat_IsOnDrop(LFBoat* boat)
{
    /* A free volatile read preserves the original pointer-register chain. */
    LFPiece* parent = *(LFPiece* volatile*)&boat->piece;
    parent = parent->parent;
    if (parent && parent != (LFPiece*)-1 && parent->def == g_lfdr_def)
        return 1;
    return 0;
}

/* Original requires BOTH head and tail null for an empty queue. It does not
 * clear node->next or repair an inconsistent head/tail pair. */
// FUNCTION: LEGOLAND 0x00411e30
void LFQueue_Append(LFQueue* queue, Link* node)
{
    if (!queue->head && !queue->tail) {
        queue->head = node;
        queue->tail = node;
    } else {
        queue->tail->next = node;
        queue->tail = node;
    }
}

/* Right corner adds one doubled width to the top-left coordinate. */
// FUNCTION: LEGOLAND 0x00411220
Pos LFQuadTopRight(void)
{
    int width, height;
    Pos out;
    GetTileDimensions(&width, &height);
    width <<= 1;
    height <<= 1;
    out.x = (width >> 1) + width;
    out.y = height >> 1;
    return out;
}

/* Render each non-null boat in the list over this piece, using mode 1. */
// FUNCTION: LEGOLAND 0x0040ca30
void LFDrawBoatList(RenderList* list, LFPiece* piece)
{
    RenderItem* item = list->head;
    while (item) {
        if (item->boat) LFBoat_Draw(item->boat, piece, 1);
        item = item->next;
    }
}

/* Choose a first-name index from the 3D person's sex group, then one of 107 surnames. */
// FUNCTION: LEGOLAND 0x00482c60
void InitBlokeName(Bloke* bloke)
{
    if (bloke->person->sex)
        bloke->first_name = (unsigned int)rand() % 90;
    else
        bloke->first_name = (unsigned int)rand() % 83;
    bloke->last_name = (unsigned int)rand() % 107;
}

/* Original bug retained: an empty head is dereferenced unless it equals the
 * requested pointer. A missing boat in a nonempty list is left untouched. */
// FUNCTION: LEGOLAND 0x00418f90
void BsBoat_Destroy(BsBoat* boat)
{
    BsBoat* node = g_bs_boats;
    BsBoat* prev = 0;
    while (node != boat) {
        prev = node;
        node = node->next;
        if (!node) return;
    }
    if (node) {
        if (prev) prev->next = node->next;
        else g_bs_boats = node->next;
        HeapFree_w(boat);
    }
}
