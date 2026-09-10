#ifdef LEGOLAND_PORTABLE
#define PU_NextInput PU_NextInput_vc6_body
#endif
/* LEGOLAND: small icon/help, script and report/free-play handlers. */
typedef struct Pos { int x, y; } Pos;
typedef struct ClipRect { int left, top, right, bottom; } ClipRect;
typedef struct Sprite Sprite;
typedef struct Icon {
    struct Icon* next; Sprite* sprite; void* data;
    short x, y, w, h; unsigned short group; short f16;
    int f18; char* text; int f20; void* f24;
    int (*render)(struct Icon*);
    char (*input)(struct Icon*, int, short, short);
    void* widget; unsigned int flags; char* help; int help_id;
} Icon;
typedef struct Indicator {
    struct Indicator* next; unsigned int flags;
    char pad08[0x14]; void (*callback)(struct Indicator*);
} Indicator;
typedef struct ScriptEvent {
    struct ScriptEvent* next; void* elem; char* text; int kind;
    unsigned char flags; char pad11[0x2b]; int time; int f40;
} ScriptEvent;
typedef struct ScriptStep {
    struct ScriptStep* next; int id; char* text;
    ScriptEvent* ev0c; ScriptEvent* ev10;
} ScriptStep;

extern Icon* g_info_icon_a; /* 0x007fdfdc */
extern Icon* g_info_icon_b; /* 0x007fdfc0 */
extern Icon* g_info_icon_d; /* 0x007fdea8 */
extern Icon* g_info_icon_e; /* 0x007fe000 */
extern Icon* g_info_icon_f; /* 0x007fdfe0 */
extern Icon* g_info_icon_g; /* 0x007fdea4 */
extern Icon* g_info_icon_h; /* 0x007fdfcc */
extern Sprite* g_pu_ok; /* 0x00668938 */
extern Sprite* g_cb_close; /* 0x0066893c */
extern Sprite* g_pu_close_on; /* 0x0066891c */
extern Sprite* g_pu_delete_on; /* 0x00668914 */
extern int g_info_fa4; /* 0x007fdfa4 */
extern int g_pending_state; /* 0x00832ba0 */
extern char g_level_end_sequence1[]; /* 0x00832998 */
extern char g_level_end_sequence2[]; /* 0x00832a98 */
extern int g_help_target; /* 0x004b9f8c; polymorphic id/object pointer */
extern int g_help_changed; /* 0x004b9f88 */
extern unsigned long g_help_hover_start; /* 0x007fe920 */
extern int g_help_face_state; /* 0x006687a4 */
extern int g_help_requested; /* 0x006687a8 */

extern void SetIconSprite(Icon*, Sprite*); /* 0x0046d680 */
extern void GetIconBounds(Icon*, ClipRect*); /* 0x0046de50 */
extern void GetIconHitBounds(Icon*, ClipRect*); /* 0x0046de90 */
extern int PointInIconRect(Pos*, ClipRect*); /* 0x0046f300 */
extern void SetClipping(ClipRect*); /* 0x0048a5c0 */
extern void* HeapAlloc_w(unsigned int); /* 0x0049e4ff */
extern void RunLevelEndSequence(char*); /* 0x00459710 */
extern void ClosePopUpIcons(void); /* 0x00471610 */
extern void ClearNewObjectMarkers(void); /* 0x004714e0 */
extern void ResetInfoStruct(void); /* 0x00471510 */
extern void DisablePopUpInputs(void); /* 0x00471470 */
extern int IsNarrationPlaying(void); /* 0x00498cf0; speech state == playing */
extern void ResetHelpKeyCursor(void); /* 0x004735b0 */
extern __declspec(dllimport) unsigned long __stdcall GetTickCount(void); /* IAT 0x004ab1f8 */
extern char PU_CloseInput(Icon*, int, short, short); /* 0x004730f0 */
extern char PU_DeleteInput(Icon*, int, short, short); /* 0x004731a0 */
extern char PU_GardenerInput(Icon*, int, short, short); /* 0x004733f0 */
extern char PU_MechInput(Icon*, int, short, short); /* 0x00473460 */
extern char PU_Delete2Input(Icon*, int, short, short); /* 0x004734d0 */
extern Sprite* g_pu_prev_on; /* 0x0066892c */
extern Sprite* g_pu_next_on; /* 0x00668924 */
extern Sprite* g_cb_close_on; /* 0x00668940 */
extern int g_new_obj_index; /* 0x007fdf78 */
extern int g_new_obj_count; /* 0x007fdf74 */
extern int g_clock_frozen; /* 0x0079a890 */
extern int g_clock_held; /* 0x0079a894 */
extern int g_clock_base; /* 0x0079a898 */
extern int g_clock_aux_held; /* 0x0079a89c */
extern int g_clock_aux_base; /* 0x0079a8a0 */
extern int g_detail; /* 0x008119a4; raw auxiliary counter read retained */
extern int g_script_step_started; /* 0x00668790 */
extern int g_map_dirty; /* 0x00668610 */
extern int g_script_tick; /* 0x00668794 */
extern unsigned int g_event_dirty_mask[]; /* 0x004b9e5c */
extern int g_help_force; /* 0x006687ac */
extern Icon* g_focussed_icon; /* 0x006687d0 */
extern Icon* g_side_icons; /* 0x006687c8 */
extern Icon* g_side_icon_tail; /* 0x004ba87c */
extern Icon* g_side_icon_tail2; /* 0x004ba880 */
extern ScriptEvent* g_goal_list; /* 0x00668728 */
extern ScriptEvent* g_object_help; /* 0x00668724 */
extern ScriptEvent* g_script_event; /* 0x00668784 */
extern ScriptStep* g_script_steps; /* 0x00668798 */
extern ScriptStep* g_script_cur; /* 0x0066879c */
extern void UpdateHelpBar(void); /* 0x0046d110 */
extern void FreeIcon(Icon*); /* 0x0046d3c0 */
extern void FreeScriptEventList(ScriptEvent*); /* 0x00468970 */
extern void FreeScriptStepList(ScriptStep*); /* 0x0046b560 */
extern void HeapFree_w(void*); /* 0x0049e4d0 */
extern unsigned char g_report_state[0xa0]; /* 0x00665ff8 */
extern int g_instant_appraisal; /* 0x00666098 */
extern int SaveGameWrite(const void*, unsigned int); /* 0x0047d760 */
extern int SaveGameRead(void*, unsigned int); /* 0x0047d730 */
extern int GetGameTimer(void); /* 0x00499430 */
typedef struct KeyMapEntry { unsigned char dik; char ch; } KeyMapEntry;
extern KeyMapEntry g_key_map[59]; /* 0x004bad58 */
extern unsigned char g_key_state[256]; /* 0x007fdda0 */
extern char g_typed_key_prev[59]; /* 0x00668de4; separate from GetInputChar */
extern ScriptEvent* NewScriptEvent(int kind, int param); /* 0x00468910 */
extern void SetScriptEventText(ScriptEvent*, const char*, int copy); /* 0x00468b40 */
extern void EnqueueObjectHelp(ScriptEvent*); /* 0x00468b00 */
extern int g_last_hint; /* 0x00668614 */
typedef struct Cell { char pad00[4]; unsigned char x, y; } Cell;
extern Cell* g_popup_cell; /* 0x007fdf84 */
extern int g_gardener_count; /* 0x0079a8bc */
extern int g_mechanic_count; /* 0x0079a8cc */
extern Sprite* g_pu_gardener_on; /* 0x00668944 */
extern Sprite* g_pu_mech_on; /* 0x0066894c */
extern int CanHireGardener(void); /* 0x0049a120 */
extern int CanHireMechanic(void); /* 0x0049a160 */
#ifndef LEGOLAND_PORTABLE
extern void GenerateGardener(Pos*, int); /* 0x0049a1a0 */
#else
extern int GenerateGardener(Pos*, int); /* 0x0049a1a0 */
#endif
#ifndef LEGOLAND_PORTABLE
extern void GenerateMechanic(Pos*, int); /* 0x0049a340 */
#else
extern int GenerateMechanic(Pos*, int); /* 0x0049a340 */
#endif
extern Sprite* g_backdrop; /* 0x00810148 */
extern int g_cur_screen; /* 0x0080ff84 */
extern void* g_icon_handler1; /* 0x006687bc */
extern void* g_icon_handler2; /* 0x006687c0 */
#ifndef LEGOLAND_PORTABLE
extern void KillSprite(Sprite*); /* 0x00497bd0 */
#else
extern int KillSprite(Sprite*); /* 0x00497bd0 */
#endif
extern void RemoveIconGroup(int); /* 0x0046d520 */
extern void KillSaveScreenSprites(void); /* 0x0048dbc0 */
extern void KillTitleScreenSprites(void); /* 0x0048faa0 */
extern void DeleteSavedGameList(void); /* 0x0048e160 */
extern void CleanUpFreePlay(void); /* 0x0048b540 */
extern void RestoreCurrentProfileFromList(void); /* 0x0048d230 */
#ifndef LEGOLAND_PORTABLE
extern void UpdateSoundVols(void); /* 0x00495a90 */
#else
extern int UpdateSoundVols(void); /* 0x00495a90 */
#endif
extern void DeleteProfileList(void); /* 0x00491b50 */
extern void KillListProfileSprite(void); /* 0x0048ca40 */
extern Sprite* g_rep_next_lit; /* 0x0081c034 */
extern Icon* g_rep_next_icon; /* 0x007cb2e4 */
extern Icon* g_rep_prev_icon; /* 0x007cb2e0 */
extern void* g_snd_click; /* 0x004b92c0 */
extern int g_rep_page; /* 0x004bf670; 1-based starting line, not page ordinal */
extern int g_rep_line_count; /* 0x0079887c */
#ifndef LEGOLAND_PORTABLE
extern void PlayInstanceOfSample(void*, int, int, void*); /* 0x00496d20 */
#else
extern int PlayInstanceOfSample(void*, int, int, void*); /* 0x00496d20 */
#endif
extern char ReportAcceptInput(Icon*, int, short, short); /* 0x00490970 */
extern void UpdateReportPageIcons(void); /* 0x00490aa0 */
extern void PlayReportPageNarration(int); /* 0x00490a20 */
typedef struct FPEntry { int id; char* name; int cost, chosen; } FPEntry;
extern int FreePlayItemUpdate(const char*, FPEntry**); /* 0x0048a840 */
extern int g_fp_bulk_update; /* 0x00798648 */
extern Icon* g_fp_accept_icon; /* 0x0079864c */
extern int g_frontend_checkbox_closed; /* 0x004bef9c */
extern unsigned char g_save_type; /* 0x0080ffe5 */
extern int g_6687b0; /* 0x006687b0 */
extern int g_game_mode; /* 0x008119b4 */
extern void SetWaitSpriteRect(int, int); /* 0x00466360 */
extern int PauseCurrentTrack(void); /* 0x00498920; destroys speech */
extern void StartFreePlayPark(void); /* 0x0048abb0 */
extern void InitGameInterface(int); /* 0x004749d0 */
extern void SetInGameIconHandlers(void); /* 0x00474880 */
extern int g_icons2_mode; /* 0x00668e38 */
extern char g_report_movie[256]; /* 0x00798778 */
extern const char kEmpty[]; /* 0x004d8bb0 */
extern void KillReportScreenSprites(void); /* 0x004908b0 */
extern void FreeHelpTextBuffer(void); /* 0x00490850 */
extern void FreeReportHintBuffer(void); /* 0x00490880 */
extern void ShowInfoPanel(int); /* 0x00490600 */
#ifndef LEGOLAND_PORTABLE
extern void SetPointer(int); /* 0x00463850 */
#else
extern int SetPointer(int); /* 0x00463850 */
#endif
#ifndef LEGOLAND_PORTABLE
extern void PlayMovie(const char*, int, int); /* 0x004771f0 */
#else
extern int PlayMovie(const char*, int, int); /* 0x004771f0 */
#endif
extern void SetReportMovie(const char*); /* 0x00490610 */
extern void ResumePausedSamples(void); /* 0x00492850 */
extern void KillAdvisorHelp(void); /* 0x0046ce20 */
#ifndef LEGOLAND_PORTABLE
extern void RestoreScriptStepHelp(void); /* 0x0046b760 */
#else
extern int RestoreScriptStepHelp(void); /* 0x0046b760 */
#endif

// FUNCTION: LEGOLAND 0x004714a0
void ResetInfoPopUp(void)
{
    g_info_icon_a->input = PU_DeleteInput;
    g_info_icon_b->input = PU_CloseInput;
    g_info_icon_f->input = PU_GardenerInput;
    g_info_icon_g->input = PU_MechInput;
    g_info_icon_h->input = PU_Delete2Input;
}

// FUNCTION: LEGOLAND 0x00471d60
void ResetToolIcons(void)
{
    SetIconSprite(g_info_icon_d, g_pu_ok);
    SetIconSprite(g_info_icon_e, g_cb_close);
}

// FUNCTION: LEGOLAND 0x0046df30
int ClipToIcon(Icon* p)
{
    ClipRect r;
    GetIconBounds(p, &r);
    SetClipping(&r);
    return 0;
}

// FUNCTION: LEGOLAND 0x0046f330
int IconHitTest(Pos* pt, Icon* p)
{
    ClipRect r;
    GetIconHitBounds(p, &r);
    return PointInIconRect(pt, &r);
}

// FUNCTION: LEGOLAND 0x0046b4f0
ScriptStep* NewScriptStep(int id)
{
    ScriptStep* s = (ScriptStep*)HeapAlloc_w(sizeof(ScriptStep));
    if (s) {
        s->text = 0;
        s->id = id;
        s->ev0c = 0;
        s->ev10 = 0;
        s->next = 0;
    }
    return s;
}

// FUNCTION: LEGOLAND 0x00459820
void EndLevel(int result)
{
    g_pending_state = result;
    if (result == 1) RunLevelEndSequence(g_level_end_sequence1);
    else if (result == 2) RunLevelEndSequence(g_level_end_sequence2);
}

#ifdef LEGOLAND_PORTABLE
/* PORT-M3: the Icon +0x2c input slot is called with four arguments
 * (fpui.c CheckFocussedIcon, uimisc.c RestoreFreePlaySelections) and so
 * are g_icon_handler1/2; this body reads only the first two, which x86
 * cdecl tolerates and a wasm call_indirect does not. The matched body is
 * renamed for the portable build only and a twin of the slot's shape is
 * exported over it. VC6 compiles the #ifndef world unchanged. */
#define IndicatorInput IndicatorInput_vc6_body
#endif
// FUNCTION: LEGOLAND 0x0046fbc0
char IndicatorInput(Icon* p, int ev)
{
    Indicator* ind = (Indicator*)p->widget;
    if (ev & 5) {
        if (ind->flags & 4) ind->callback(ind);
        return 2;
    }
    return 1;
}
#ifdef LEGOLAND_PORTABLE
#undef IndicatorInput
char IndicatorInput(Icon* p, int ev, int ll_dx, int ll_dy)
{
    (void)ll_dx;
    (void)ll_dy;
    return IndicatorInput_vc6_body(p, ev);
}
#endif

// FUNCTION: LEGOLAND 0x004730f0
char PU_CloseInput(Icon* p, int ev, short dx, short dy)
{
    ClosePopUpIcons();
    if (p) SetIconSprite(p, g_pu_close_on);
    if (ev & 2) {
        ClearNewObjectMarkers();
        ResetInfoStruct();
    }
    return 1;
}

// FUNCTION: LEGOLAND 0x0046d340
void ShowObjectHelp(void* obj)
{
    if (IsNarrationPlaying()) goto requested;
    if (!obj) return;
    if ((int)obj != g_help_target) {
        g_help_target = (int)obj;
        g_help_hover_start = GetTickCount();
        g_help_face_state = 2;
        g_help_changed = 1;
    }
requested:
    g_help_requested = 1;
}

// FUNCTION: LEGOLAND 0x0046d230
void ShowIdHelp(int id)
{
    if (IsNarrationPlaying()) goto requested;
    if (id == -1) return;
    if (id != g_help_target) {
        g_help_target = id;
        g_help_hover_start = GetTickCount();
        g_help_face_state = 0;
        g_help_changed = 1;
        ResetHelpKeyCursor();
    }
requested:
    g_help_requested = 1;
}

// FUNCTION: LEGOLAND 0x004731a0
char PU_DeleteInput(Icon* p, int ev, short dx, short dy)
{
    ClosePopUpIcons();
    SetIconSprite(p, g_pu_delete_on);
    if (ev & 2) {
        DisablePopUpInputs();
        g_info_fa4 = 1;
    }
    return 1;
}

#ifdef LEGOLAND_PORTABLE
/* PORT-M3: the Icon +0x2c input slot is called with four arguments
 * (fpui.c CheckFocussedIcon, uimisc.c RestoreFreePlaySelections) and so
 * are g_icon_handler1/2; this body reads only the first two, which x86
 * cdecl tolerates and a wasm call_indirect does not. The matched body is
 * renamed for the portable build only and a twin of the slot's shape is
 * exported over it. VC6 compiles the #ifndef world unchanged. */
#define PU_PrevInput PU_PrevInput_vc6_body
#endif
// FUNCTION: LEGOLAND 0x004733b0
char PU_PrevInput(Icon* p, int ev)
{
    ResetToolIcons();
    SetIconSprite(p, g_pu_prev_on);
    if ((ev & 2) && g_new_obj_index > 0) --g_new_obj_index;
    return 1;
}
#ifdef LEGOLAND_PORTABLE
#undef PU_PrevInput
char PU_PrevInput(Icon* p, int ev, int ll_dx, int ll_dy)
{
    (void)ll_dx;
    (void)ll_dy;
    return PU_PrevInput_vc6_body(p, ev);
}
#endif

// FUNCTION: LEGOLAND 0x0046de50
void GetIconBounds(Icon* p, ClipRect* r)
{
    r->left = p->x;
    r->top = p->y;
    r->right = p->w + p->x;
    r->bottom = p->h + p->y;
}

// FUNCTION: LEGOLAND 0x004993c0
void ThawGameClock(void)
{
    if (g_clock_frozen) {
        g_clock_frozen = 0;
        g_clock_base += GetTickCount() - g_clock_held;
        g_clock_aux_base += g_detail - g_clock_aux_held;
    }
}

// FUNCTION: LEGOLAND 0x0046b200
int ScriptEventDue(ScriptEvent* e)
{
    if (g_script_step_started || (g_event_dirty_mask[e->kind] & g_map_dirty)
        || g_script_tick - e->time > 5000) return 1;
    return 0;
}

// FUNCTION: LEGOLAND 0x00473310
char PU_ToolA(Icon* p, int ev)
{
    ResetToolIcons();
    SetIconSprite(p, g_cb_close_on);
    if (ev & 2) {
        g_info_icon_d->flags |= 0x400;
        g_info_icon_e->flags |= 0x400;
        ResetInfoPopUp();
        g_info_fa4 = 0;
    }
    return 1;
}

// FUNCTION: LEGOLAND 0x0046d280
int ShowHelpPopup(int id)
{
    if (IsNarrationPlaying() || id == -1 || id == g_help_target) return 0;
    g_help_target = id;
    g_help_hover_start = GetTickCount();
    g_help_face_state = 0;
    g_help_changed = 1;
    g_help_force = 1;
    ResetHelpKeyCursor();
    g_help_requested = 1;
    UpdateHelpBar();
    return 1;
}

/* Original clears focus when it equals the SUCCESSOR, not the removed icon.
 * FreeIcon separately clears focus on the removed record. */
// FUNCTION: LEGOLAND 0x0046d460
void UnlinkIcon(Icon** link)
{
    Icon* next;
    if (g_focussed_icon == (*link)->next) g_focussed_icon = 0;
    if (g_side_icon_tail == *link) g_side_icon_tail = (Icon*)link;
    next = (*link)->next;
    FreeIcon(*link);
    *link = next;
}

// FUNCTION: LEGOLAND 0x0046d4a0
void UnlinkIcon2(Icon** link)
{
    Icon* next;
    if (g_focussed_icon == (*link)->next) g_focussed_icon = 0;
    if (g_side_icon_tail2 == *link) g_side_icon_tail2 = (Icon*)link;
    next = (*link)->next;
    FreeIcon(*link);
    *link = next;
}

// FUNCTION: LEGOLAND 0x0046c5c0
void KillHelpText(void)
{
    FreeScriptEventList(g_goal_list);
    g_goal_list = 0;
    FreeScriptEventList(g_object_help);
    g_object_help = 0;
    FreeScriptEventList(g_script_event);
    g_script_event = 0;
    FreeScriptStepList(g_script_steps);
    g_script_steps = 0;
    g_script_cur = 0;
}

// FUNCTION: LEGOLAND 0x00473360
char PU_NextInput(Icon* p, int ev)
{
    ResetToolIcons();
    if (p) SetIconSprite(p, g_pu_next_on);
    if (ev & 2) {
        if (g_new_obj_index < g_new_obj_count - 1) ++g_new_obj_index;
        else ResetInfoStruct();
    }
    return 1;
}

// FUNCTION: LEGOLAND 0x0046d4e0
void DeleteIcon(Icon* p)
{
    if (g_side_icons == p) UnlinkIcon(&g_side_icons);
    else {
        Icon* prev = g_side_icons;
        while (prev) {
            Icon* next = prev->next;
            if (next == p) break;
            prev = next;
        }
        if (prev) UnlinkIcon(&prev->next);
    }
}

// FUNCTION: LEGOLAND 0x0046b520
void FreeScriptStep(ScriptStep* s)
{
    if (s->ev0c) FreeScriptEventList(s->ev0c);
    if (s->ev10) FreeScriptEventList(s->ev10);
    if (s->text) HeapFree_w(s->text);
    HeapFree_w(s);
}

// FUNCTION: LEGOLAND 0x00444200
int SaveReport(void)
{
    int remaining, ok;
    ok = SaveGameWrite(g_report_state, sizeof(g_report_state));
    if (!ok) return ok;
    if (!g_instant_appraisal) remaining = -1;
    else remaining = g_instant_appraisal - GetGameTimer();
    return SaveGameWrite(&remaining, 4) != 0;
}

// FUNCTION: LEGOLAND 0x00444260
int LoadReport(void)
{
    int remaining, ok;
    ok = SaveGameRead(g_report_state, sizeof(g_report_state));
    if (!ok) return ok;
    ok = SaveGameRead(&remaining, 4);
    if (!ok) return ok;
    if (remaining == -1) g_instant_appraisal = 0;
    else g_instant_appraisal = GetGameTimer() + remaining;
    return 1;
}

// FUNCTION: LEGOLAND 0x00474130
char GetTypedChar(void)
{
    int result = 0, i, prev;
    unsigned char cur;
    for (i = 0; i < 59; i++) {
        cur = g_key_state[g_key_map[i].dik];
        prev = (unsigned)(g_typed_key_prev[i] & 0x80) >> 7;
        g_typed_key_prev[i] = cur;
        if ((cur & 0x80) && !prev) result = g_key_map[i].ch;
    }
    if (result == -10) result = ':';
    return result;
}

// FUNCTION: LEGOLAND 0x0046b6b0
void ShowScriptStepText(ScriptStep* s, int copy)
{
    if (s->text) {
        ScriptEvent* e = NewScriptEvent(1, 2);
        SetScriptEventText(e, s->text, copy);
        if (!copy) {
            s->text = 0;
            e->flags = 0x20;
        }
        EnqueueObjectHelp(e);
        g_last_hint = 0;
    }
}

// FUNCTION: LEGOLAND 0x004733f0
char PU_GardenerInput(Icon* p, int ev, short dx, short dy)
{
    Pos pos;
    ClosePopUpIcons();
    if (g_gardener_count < 15) {
        SetIconSprite(p, g_pu_gardener_on);
        if (ev & 2) {
            pos.x = g_popup_cell->x;
            pos.y = g_popup_cell->y;
            if (CanHireGardener()) GenerateGardener(&pos, 1);
        }
    }
    return 1;
}

// FUNCTION: LEGOLAND 0x00473460
char PU_MechInput(Icon* p, int ev, short dx, short dy)
{
    Pos pos;
    ClosePopUpIcons();
    if (g_mechanic_count < 15) {
        SetIconSprite(p, g_pu_mech_on);
        if (ev & 2) {
            pos.x = g_popup_cell->x;
            pos.y = g_popup_cell->y;
            if (CanHireMechanic()) GenerateMechanic(&pos, 1);
        }
    }
    return 1;
}

// FUNCTION: LEGOLAND 0x004585c0
void KillCurrentScreen(void)
{
    if (g_backdrop) {
        KillSprite(g_backdrop);
        g_backdrop = 0;
    }
    RemoveIconGroup(7);
    if (g_cur_screen != -1) {
        switch (g_cur_screen) {
        case 0:
            RestoreCurrentProfileFromList();
            UpdateSoundVols();
            DeleteProfileList();
            KillListProfileSprite();
            break;
        case 3:
            CleanUpFreePlay();
            break;
        case 4:
            KillSaveScreenSprites();
            KillTitleScreenSprites();
            DeleteSavedGameList();
            break;
        }
        g_icon_handler1 = 0;
        g_icon_handler2 = 0;
    }
}

#ifdef LEGOLAND_PORTABLE
/* PORT-M3: the Icon +0x2c input slot is called with four arguments
 * (fpui.c CheckFocussedIcon, uimisc.c RestoreFreePlaySelections) and so
 * are g_icon_handler1/2; this body reads only the first two, which x86
 * cdecl tolerates and a wasm call_indirect does not. The matched body is
 * renamed for the portable build only and a twin of the slot's shape is
 * exported over it. VC6 compiles the #ifndef world unchanged. */
#define ReportNextPageInput ReportNextPageInput_vc6_body
#endif
// FUNCTION: LEGOLAND 0x00490b20
char ReportNextPageInput(Icon* p, int ev)
{
    SetIconSprite(g_rep_next_icon, g_rep_next_lit);
    if (ev & 2) {
        if (g_rep_next_icon->flags & 0x400)
            return ReportAcceptInput(0, ev, 0, 0);
        PlayInstanceOfSample(g_snd_click, 0, 1, 0);
        g_rep_page += 14;
        UpdateReportPageIcons();
    }
    return 1;
}
#ifdef LEGOLAND_PORTABLE
#undef ReportNextPageInput
char ReportNextPageInput(Icon* p, int ev, int ll_dx, int ll_dy)
{
    (void)ll_dx;
    (void)ll_dy;
    return ReportNextPageInput_vc6_body(p, ev);
}
#endif

/* Replay selected class icons while suppressing their individual click sounds.
 * Original assumes every qualifying icon resolves to a non-null table row. */
// FUNCTION: LEGOLAND 0x0048a790
void RestoreFreePlaySelections(void)
{
    Icon* p = g_side_icons;
    FPEntry* entry;
    g_fp_bulk_update = 1;
    while (p) {
        if (p->group == 200 || p->group == 500 || p->group == 400 || p->group == 300) {
            FreePlayItemUpdate(p->text, &entry);
            if (entry->chosen) p->input(p, 2, 0, 0);
        }
        p = p->next;
    }
    g_fp_bulk_update = 0;
}

#ifdef LEGOLAND_PORTABLE
/* PORT-M3: the Icon +0x2c input slot is called with four arguments
 * (fpui.c CheckFocussedIcon, uimisc.c RestoreFreePlaySelections) and so
 * are g_icon_handler1/2; this body reads only the first two, which x86
 * cdecl tolerates and a wasm call_indirect does not. The matched body is
 * renamed for the portable build only and a twin of the slot's shape is
 * exported over it. VC6 compiles the #ifndef world unchanged. */
#define FreePlayAcceptInput FreePlayAcceptInput_vc6_body
#endif
// FUNCTION: LEGOLAND 0x0048ac60
char FreePlayAcceptInput(Icon* p, int ev)
{
    if (g_fp_accept_icon && (g_fp_accept_icon->flags & 0x400)) return 0;
    if (g_frontend_checkbox_closed && (ev & 2)) {
        g_save_type = 2;
        SetWaitSpriteRect(0x127, 0x170);
        PauseCurrentTrack();
        g_6687b0 = 4;
        PlayInstanceOfSample(g_snd_click, 0, 1, 0);
        StartFreePlayPark();
        CleanUpFreePlay();
        KillTitleScreenSprites();
        RemoveIconGroup(7);
        g_game_mode = 3;
        InitGameInterface(1);
        SetInGameIconHandlers();
        g_save_type = 2;
    }
    return 1;
}
#ifdef LEGOLAND_PORTABLE
#undef FreePlayAcceptInput
char FreePlayAcceptInput(Icon* p, int ev, int ll_dx, int ll_dy)
{
    (void)ll_dx;
    (void)ll_dy;
    return FreePlayAcceptInput_vc6_body(p, ev);
}
#endif

// FUNCTION: LEGOLAND 0x00490aa0
void UpdateReportPageIcons(void)
{
    if (g_rep_page <= 1) {
        g_rep_page = 1;
        g_rep_prev_icon->flags |= 0x400;
    } else g_rep_prev_icon->flags &= ~0x400;
    if (g_rep_page + 14 > g_rep_line_count) g_rep_next_icon->flags |= 0x400;
    else g_rep_next_icon->flags &= ~0x400;
    PlayReportPageNarration(g_rep_page / 14);
}

// FUNCTION: LEGOLAND 0x00490970
char ReportAcceptInput(Icon* p, int ev, short dx, short dy)
{
    if (ev & 2) {
        PlayInstanceOfSample(g_snd_click, 0, 1, 0);
        g_icons2_mode = 0;
        KillReportScreenSprites();
        g_game_mode = 3;
        SetInGameIconHandlers();
        FreeHelpTextBuffer();
        FreeReportHintBuffer();
        ShowInfoPanel(0);
        if (g_report_movie[0]) {
            SetPointer(0);
            PlayMovie(g_report_movie, 1, 1);
            SetPointer(6);
            SetReportMovie(kEmpty);
        }
        PauseCurrentTrack();
        g_6687b0 = 4;
        ThawGameClock();
        ResumePausedSamples();
        KillAdvisorHelp();
        RestoreScriptStepHelp();
    }
    return 1;
}

#ifdef LEGOLAND_PORTABLE
/* PU_NextInput is called with 0 argument(s) the original ignores: the body
 * at this address never reads them, and in cdecl the caller cleans them up.
 * On wasm the argument count is part of the function type, so the exported
 * name is this forwarder and the matched body keeps its own.  */
#undef PU_NextInput
char PU_NextInput(Icon* ll_p, int ll_ev, short ll_a, short ll_b) {  return PU_NextInput_vc6_body(ll_p, ll_ev); }
#endif
