/* LEGOLAND -- coaster shade / blit table callees (inventory group 4).
 * VC6 SP3 /O2 /Gy /Gd. Types are local; offsets describe the original ABI.
 * Scope LL4 (docs/SCOPE_LL4_coaster_shades.md). Notes: docs/lanes/scope-ll4.md.
 */

typedef struct Vec3f { float x, y, z; } Vec3f;
typedef struct TrackNode TrackNode;
typedef struct RouteGeom RouteGeom;

struct TrackNode {
    int flags;                 /* +00 */
    unsigned int square;       /* +04 */
    void* cls;                 /* +08 */
    void* desc;                /* +0c */
    void* owner;               /* +10 */
    int jin[2];                /* +14 */
    TrackNode* prev;           /* +1c */
    int jout[2];               /* +20 */
    TrackNode* next;           /* +28 */
};

/* Render-object / geometry link. Advance walks +0x50; retreat walks +0x54
 * and, on a node change, the last +0x50 link of the previous piece. */
struct RouteGeom {
    unsigned char pad00[0x50];
    RouteGeom* next;           /* +50 */
    RouteGeom* prev;           /* +54 */
};

typedef struct RoutePos {
    TrackNode* node;           /* +00 */
    RouteGeom* geom;           /* +04 */
    Vec3f pos;                 /* +08 */
} RoutePos;

extern void* g_coaster_tab_c[];                                /* 0x004d89c8 */
extern RouteGeom* GetTrackNodeWorldPos(TrackNode* node, Vec3f* out); /* 0x0041cff0 */

/* Indexed reader for the .ltx table LoadCoasterData fills through
 * CoasterModel_LoadLTX. Sibling of LoadCoasterMesh / LoadCoasterMeshTex /
 * GetCoasterModelSize, which look up a name first; this one takes the
 * part index FindCoasterPart already resolved. */
// FUNCTION: LEGOLAND 0x00420780
void* GetCoasterTexture(int index)
{
    return g_coaster_tab_c[index];
}

/* Inverse of TrackCursor_AdvanceGeometry: one step back along the piece's
 * geometry chain. A live prev is stored and the function returns; falling
 * off the first object moves onto the previous track piece (via +0x1c) and
 * then walks that piece's +0x50 chain to its last object. */
// FUNCTION: LEGOLAND 0x0041f880
void TrackCursor_RetreatGeometry(RoutePos* cursor)
{
    RouteGeom* geom = cursor->geom->prev;
    if (geom) {
        cursor->geom = geom;
        return;
    }
    cursor->node = cursor->node->prev;
    geom = GetTrackNodeWorldPos(cursor->node, &cursor->pos);
    cursor->geom = geom;
    if (geom->next) {
        do {
            cursor->geom = cursor->geom->next;
        } while (cursor->geom->next);
    }
}
