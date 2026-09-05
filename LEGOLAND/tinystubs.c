/* LEGOLAND -- scope E: UI, script, save, render and system micro-helpers.
 * VC6 SP3 /O2 /Gy /Gd. Types describe only the fields used here.
 * Verification and recovered mechanics: docs/lanes/scope-e.md.
 */
typedef struct Pos { int x, y; } Pos;
typedef struct ClipRect { int left, top, right, bottom; } ClipRect;
typedef struct Icon { struct Icon* next; char pad04[0x28]; void* input; } Icon;
typedef struct ScriptStep { struct ScriptStep* next; } ScriptStep;
typedef struct ScriptEvent { struct ScriptEvent* next; } ScriptEvent;
typedef struct Sample { struct Sample* next; } Sample;
typedef struct PathSquare {
    struct PathSquare* next; char pad04[0x1c]; unsigned int flags;
} PathSquare;
typedef struct Cursor { char pad00[0x140c]; int valid, error; } Cursor;
typedef struct ObjDef { char pad00[0x1c]; unsigned int flags; short kind; } ObjDef;
typedef struct Bloke { char pad00[0x78]; short stay, allowance; } Bloke;
typedef struct RepairOrder { char pad00[0x34]; float charge, amount; } RepairOrder;
typedef struct LocSet { char pad00[0x2c]; char* points; char* indices; } LocSet;
typedef struct CachedEntry { char pad00[0x1c]; void* sprite; } CachedEntry;
typedef struct Cell { char pad00[8]; unsigned short tile; char pad0a[10]; } Cell;
typedef struct RenderItem { char data[0x10]; } RenderItem;
typedef struct DIDevice DIDevice;
typedef struct DIVtbl { void* unused[2]; unsigned long (__stdcall *Release)(DIDevice*); } DIVtbl;
struct DIDevice { DIVtbl* vt; };

extern int g_gardener_count;                       /* 0x0079a8bc */
extern int g_mechanic_count;                       /* 0x0079a8cc */
extern Pos g_entrance_tile;                        /* 0x0066b460 */
extern int g_entrance_tile_time;                   /* 0x0066b468 */
extern int g_message_level;                       /* 0x00668960 */
extern Bloke* g_worker_on_mouse;                   /* 0x007fdff0 */
extern int g_help_face_state;                     /* 0x006687a4 */
extern int g_visitor_limit;                       /* 0x0083291c */
extern int g_park_metric;                         /* 0x00832918 */
extern int g_rep_accept_mode;                     /* 0x00798878 */
extern int g_script_start;                        /* 0x00668780 */
extern int g_speech_state;                        /* 0x0079a84c */
extern ClipRect g_fullscreen_clip;                /* 0x007fe020 */
extern Icon* g_side_icon_tail;                    /* 0x004ba87c */
extern int g_brick_lock;                          /* 0x004b90fc */
extern int g_new_obj_count;                       /* 0x007fdf74 */
extern int g_new_obj_index;                       /* 0x007fdf78 */
extern void* g_new_obj_defs[20];                  /* 0x007fded4 */
extern int g_info_active;                         /* 0x007fdfa0 */
extern int g_advisor_pose;                        /* 0x00665fec */
extern int g_advisor_pose_arg;                    /* 0x0081c09c */
extern int g_advisor_pose_timer;                  /* 0x0081c088 */
extern DIDevice* g_di_keyboard;                   /* 0x00668d8c */
extern int g_theme_enabled[4];                   /* 0x00668e20 */
extern RenderItem* g_render_a;                    /* 0x0062feec */
extern RenderItem* g_render2_a;                   /* 0x0062fef0 */
extern int g_render_b;                           /* 0x00655a4c */
extern int g_render2_b;                          /* 0x00655a50 */
extern int g_rep_line_count;                     /* 0x0079887c */
extern int g_rep_hint_count;                     /* 0x00798880 */
extern char* g_rep_lines[];                      /* 0x007cafa0 */
extern char* g_rep_hints[];                      /* 0x007cb140 */
extern int g_speech_fd;                          /* 0x007caca8 */
extern long g_speech_data_offset;                /* 0x007cacb4 */
extern unsigned int g_speech_bytes_left;         /* 0x007cacac */
extern unsigned int g_speech_data_length;        /* 0x0079ac04 */
extern int g_screen_popup;                       /* 0x0080ff80 */
extern Icon* g_info_icon_a;                      /* 0x007fdfdc */
extern Icon* g_info_icon_f;                      /* 0x007fdfe0 */
extern Icon* g_info_icon_g;                      /* 0x007fdea4 */
extern Icon* g_info_icon_h;                      /* 0x007fdfcc */
extern Cell** g_map_rows;                        /* 0x00801400 */
extern unsigned short* g_path_tile_base;         /* 0x00832bf0 */
extern PathSquare* g_path_squares;               /* 0x0066b44c */
extern int g_detail_images_count;               /* 0x0079a7cc */
extern Sample* g_playable_list;                  /* 0x007988cc */
extern int g_imt_state;                          /* 0x004bf778 */
extern int g_imt_cmd;                            /* 0x0079a6a4 */
extern int g_imt_cmd_arg;                        /* 0x0079a6a8 */
extern void* g_imt_event;                        /* 0x0079a6a0 */

extern int GetGameTimer(void);                           /* 0x00499430 */
extern void ClosePrimaryPopUp(void);                     /* 0x00473160 */
extern void SetClipping(ClipRect*);                      /* 0x0048a5c0 */
extern void ResetInfoStruct(void);                       /* 0x00471510 */
extern int SaveGameRead(void*, unsigned int);            /* 0x0047d730 */
extern void HeapFree_w(void*);                           /* 0x0049e4d0 */
extern long _lseek(int, long, int);                      /* 0x004a56c3 */
extern int ShowHelpPopup(int);                           /* 0x0046d280 */
extern int PrintSprite(void*, int, int, int, void*);      /* 0x004853a0 */
extern void SetMenuHelp(int slot, void* text);            /* 0x00475fe0 */
extern void RemoveNewObjectMarker(void*);                /* 0x00471ca0 */
extern void** DetailImage_AllocSlot(void);               /* 0x00496f30 */
extern int ResumeSinglyPausedSample(Sample*);            /* 0x00492910 */
extern int PauseSingleSample(Sample*);                   /* 0x00492800 */
extern __declspec(dllimport) int __stdcall SetEvent(void*); /* 0x004ab0f8 */
extern void UpdateEntranceTile(void);                    /* 0x00482a90 */
extern void ResolveEntrancePathSquare(Pos*);             /* 0x00482a40 */
extern void FreeScriptStep(ScriptStep*);                 /* 0x0046b520 */
extern void FreeScriptEvent(ScriptEvent*);               /* 0x00468940 */

// FUNCTION: LEGOLAND 0x0048b6c0
void FreePlayInit_48b6c0(void) {}
// FUNCTION: LEGOLAND 0x00476020
void RenderIconsHook(void) {}
// FUNCTION: LEGOLAND 0x00472090
void DrawPopUpEnd(void) {}
// FUNCTION: LEGOLAND 0x0045b170
void RenderViewCellProbe(Pos* tile) { (void)tile; }
// FUNCTION: LEGOLAND 0x00453cd0
void DebugErrorSink(const char* text) { (void)text; }

// FUNCTION: LEGOLAND 0x00499560
int GetMechanicCount(void) { return g_mechanic_count; }
// FUNCTION: LEGOLAND 0x00499550
int GetGardenerCount(void) { return g_gardener_count; }
// FUNCTION: LEGOLAND 0x00482b00
Pos* GetEntranceTile(void) { return &g_entrance_tile; }
// FUNCTION: LEGOLAND 0x004735b0
void ResetHelpKeyCursor(void) { g_message_level = 0; }
// FUNCTION: LEGOLAND 0x004700f0
Bloke* GetSelectedBloke(void) { return g_worker_on_mouse; }
// FUNCTION: LEGOLAND 0x0046d3a0
void SetHelpFaceTalking(void) { g_help_face_state = 4; }
// FUNCTION: LEGOLAND 0x0044ea40
int GetVisitorLimit(void) { return g_visitor_limit; }
// FUNCTION: LEGOLAND 0x00490600
void ShowInfoPanel(int mode) { g_rep_accept_mode = mode; }
// FUNCTION: LEGOLAND 0x00468d00
void ResetScriptTimer(void) { g_script_start = GetGameTimer(); }
// FUNCTION: LEGOLAND 0x0045f460
void ResetCursorFootprint(Cursor* c) { c->valid = 1; c->error = 0; }

/* Former sub_498cf0: the narration playback state is 3 while playing. */
// FUNCTION: LEGOLAND 0x00498cf0
int IsNarrationPlaying(void) { return g_speech_state == 3; }
// FUNCTION: LEGOLAND 0x00474820
char InGamePrimaryIcon(Icon* icon, int flags)
{
    (void)icon;
    if (flags & 2) ClosePrimaryPopUp();
    return 1;
}
// FUNCTION: LEGOLAND 0x0046df60
int RenderFullScreenIcon(Icon* icon)
{
    (void)icon;
    SetClipping(&g_fullscreen_clip);
    return 0;
}
// FUNCTION: LEGOLAND 0x0046d440
void LinkIcon(Icon* icon) { g_side_icon_tail->next = icon; g_side_icon_tail = icon; }
// FUNCTION: LEGOLAND 0x00457890
int BricksAreLimited(void) { return g_brick_lock == 0; }
// FUNCTION: LEGOLAND 0x00471d40
void CloseInfoPopUp(void)
{
    if (g_new_obj_count) { ResetInfoStruct(); g_info_active = 2; }
}
// FUNCTION: LEGOLAND 0x00471bf0
void ResetInfoSelection(void)
{
    if (g_info_active != 2) { g_new_obj_index = 0; g_new_obj_count = 0; }
}
// FUNCTION: LEGOLAND 0x0045f4b0
int CursorIsValid(Cursor* c) { return c->valid > 0; }
// FUNCTION: LEGOLAND 0x00444070
void SetAdvisorPose(int pose, int arg)
{
    g_advisor_pose = pose; g_advisor_pose_arg = arg; g_advisor_pose_timer = 0;
}
// FUNCTION: LEGOLAND 0x00499760
void SetOrderRepairAmount(RepairOrder* order, float amount)
{
    order->amount = amount; order->charge = amount * 1.5f;
}
/* Despite its historical name, this consumes only one four-byte value. */
// FUNCTION: LEGOLAND 0x0047d7e0
void SkipMeasuredBlock(void) { int length; SaveGameRead(&length, 4); }
/* Original leaves the released device pointer unchanged. */
// FUNCTION: LEGOLAND 0x00473a50
void KillKeyboardDevice(void)
{
    if (g_di_keyboard) g_di_keyboard->vt->Release(g_di_keyboard);
}
// FUNCTION: LEGOLAND 0x00474970
int LoadIconStateChunk(void) { return SaveGameRead(g_theme_enabled, 16) != 0; }
// FUNCTION: LEGOLAND 0x00473660
void ProcessHelpKeys(void)
{
    if (g_message_level && !IsNarrationPlaying()) ResetHelpKeyCursor();
}
// FUNCTION: LEGOLAND 0x00443120
RenderItem* RenderItem2_Alloc(void)
{
    RenderItem* r = g_render2_a++; ++g_render2_b; return r;
}
// FUNCTION: LEGOLAND 0x00442f50
RenderItem* RenderItem_Alloc(void)
{
    RenderItem* r = g_render_a++; ++g_render_b; return r;
}
// FUNCTION: LEGOLAND 0x0043f970
void FixUpLocSetPointers(LocSet* loc)
{
    loc->points += (unsigned int)loc; loc->indices += (unsigned int)loc;
}
/* Ownership is represented by the count; free the first table entry only. */
// FUNCTION: LEGOLAND 0x00490880
void FreeReportHintBuffer(void)
{
    if (g_rep_hint_count) HeapFree_w(g_rep_hints[0]); g_rep_hint_count = 0;
}
// FUNCTION: LEGOLAND 0x00490850
void FreeHelpTextBuffer(void)
{
    if (g_rep_line_count) HeapFree_w(g_rep_lines[0]); g_rep_line_count = 0;
}
// FUNCTION: LEGOLAND 0x00498120
void RewindNarrationSource(void)
{
    _lseek(g_speech_fd, g_speech_data_offset, 0);
    g_speech_bytes_left = g_speech_data_length;
}
// FUNCTION: LEGOLAND 0x00496570
int PanFromOffset(int offset)
{
    offset *= 4;
    if (offset > 10000) return 10000;
    if (offset < -10000) offset = -10000;
    return offset;
}
// FUNCTION: LEGOLAND 0x0048fc00
void ProcessScreenPopup(void)
{
    if (g_screen_popup && ShowHelpPopup(g_screen_popup)) g_screen_popup = 0;
}
// FUNCTION: LEGOLAND 0x00471470
void DisablePopUpInputs(void)
{
    g_info_icon_a->input = 0; g_info_icon_f->input = 0;
    g_info_icon_g->input = 0; g_info_icon_h->input = 0;
}
/* Semantics: bit 0 or bit 2 returns 2, otherwise 1. VC6 substitutes a
 * setne/inc for the second branch. Direct returns, a char result local,
 * signed/unsigned parameter variants, ternaries and masked switches did
 * not restore the original branch. No artificial side effect added. */
// WIP-FUNCTION: LEGOLAND 0x0046f2e0  (60.0% audit span; 9i/18B body vs 10i/20B; first mismatch 6: setne/inc replaces the final conditional; audit includes one padding nop)
char DefaultIconInput(Icon* icon, char flags)
{
    (void)icon;
    if (flags & 1) return 2;
    {
        char result;
        if (flags & 4) result = 2; else result = 1;
        return result;
    }
}
// FUNCTION: LEGOLAND 0x0045eab0
int ClassAllowsObjects(ObjDef* def)
{
    if (def) return !(def->flags & 0x200000);
    return 0;
}
// FUNCTION: LEGOLAND 0x0045cb90
void ResetPathTile(Pos* tile)
{
    g_map_rows[tile->y][tile->x].tile = *g_path_tile_base;
}
// FUNCTION: LEGOLAND 0x00485f00
void PrintSpriteXY(void* sprite, int x, int y) { PrintSprite(sprite, x, y, 0, 0); }
// FUNCTION: LEGOLAND 0x00481ee0
void ClearPathSquareVisited(void)
{
    PathSquare* p;
    for (p = g_path_squares; p; p = p->next) p->flags &= ~1u;
}
// FUNCTION: LEGOLAND 0x00476000
void ClearMenuHelp(void)
{
    int i; for (i = 0; i < 4; ++i) SetMenuHelp(i, 0);
}
// FUNCTION: LEGOLAND 0x004714e0
void ClearNewObjectMarkers(void)
{
    while (g_new_obj_count) RemoveNewObjectMarker(g_new_obj_defs[0]);
}
// FUNCTION: LEGOLAND 0x00496fc0
int RegisterDetailImage(void* image)
{
    void** slot = DetailImage_AllocSlot();
    if (slot) { *slot = image; ++g_detail_images_count; return 1; }
    return 0;
}
// FUNCTION: LEGOLAND 0x00492850
void ResumePausedSamples(void)
{
    Sample* s; for (s = g_playable_list; s; s = s->next) ResumeSinglyPausedSample(s);
}
// FUNCTION: LEGOLAND 0x00492830
void InitOptionSamples(void)
{
    Sample* s; for (s = g_playable_list; s; s = s->next) PauseSingleSample(s);
}
// FUNCTION: LEGOLAND 0x0045f480
void SetCursorError(Cursor* c, int error)
{
    if (c->valid >= -error) { c->valid = -error; c->error = error; }
}
// FUNCTION: LEGOLAND 0x0045ead0
int IsBuildableClass(ObjDef* def)
{
    if (def && def->kind && def->kind != 2) return 1;
    return 0;
}
// FUNCTION: LEGOLAND 0x00455ec0
void PrintCachedEntry(CachedEntry* entry, int x, int y)
{
    PrintSprite(entry->sprite, x, y, 0, 0);
}
// FUNCTION: LEGOLAND 0x0044eab0
int HasBlokeStayedTooLong(Bloke* bloke)
{
    return bloke->stay - bloke->allowance / 2 > g_park_metric * 2;
}
// FUNCTION: LEGOLAND 0x00492ca0
void SetThemeInTransition(int theme)
{
    if (g_imt_state == 1 || g_imt_state == 2) {
        g_imt_cmd = 3; g_imt_cmd_arg = theme % 5; SetEvent(g_imt_event);
    }
}
// FUNCTION: LEGOLAND 0x00482b20
void RefreshEntranceTile(int force)
{
    int now = GetGameTimer();
    if (now - g_entrance_tile_time > 4000 || force) {
        g_entrance_tile_time = now; UpdateEntranceTile(); ResolveEntrancePathSquare(&g_entrance_tile);
    }
}
// FUNCTION: LEGOLAND 0x0046b560
void FreeScriptStepList(ScriptStep* step)
{
    if (step) { if (step->next) FreeScriptStepList(step->next); FreeScriptStep(step); }
}
// FUNCTION: LEGOLAND 0x00468970
void FreeScriptEventList(ScriptEvent* event)
{
    if (event) { if (event->next) FreeScriptEventList(event->next); FreeScriptEvent(event); }
}
