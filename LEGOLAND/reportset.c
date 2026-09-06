/* LEGOLAND -- scope Y: the 25 per-report setters.
 *
 * The level file's REPORT keyword (levelkw3.c) looks its first word up in
 * g_report_names[25] and hands the index and the line's two numbers to
 * SetReportMode (0x0046a140, eventtick.c), which calls
 * g_report_set[index](a, b) -- the table of 25 function pointers at
 * 0x004b7e38 whose targets are this file, in the same order.
 *
 * Every setter is the same shape: both arguments zero turns the report off
 * (its bit is cleared in the flags word), anything else stores the numbers
 * in the report's slot(s) and sets the bit. The five ride reports keep a
 * two-bit mode taken from the second argument instead of a single bit, and
 * store only the first. HAPPPY_VIS and HUNGRY_VIS share one slot pair and
 * differ only in their bit -- setting either overwrites the other's numbers,
 * as shipped.
 *
 * The bit is set BEFORE the slot stores in every body on purpose: written
 * that way VC6 hoists the flags load above the stores, keeps the `or` on
 * the full word and sinks the flags store below them, which is the
 * original's schedule. Written after the stores it narrows the `or` to a
 * byte and drops the interleave (11 of 14). The clear arm is the plain
 * `&= ~BIT` either way, and VC6 picks its own width for that one -- a byte
 * `and al,0xfe` for the low bits, a full `and dword ptr [..],0xfffbffff`
 * once the constant no longer fits in a byte.
 *
 * VC6 SP3 /O2 /Gy /Gd. Types describe only the fields used here; names are
 * ours (the brief's readings, which are the report keywords), addresses are
 * load-bearing.
 */

/* ---- the report state block (uimisc.c's g_report_state, saved whole) ----- */

/* A report's two REPORT-keyword numbers. */
typedef struct ReportArgs { int a; int b; } ReportArgs;

typedef struct ReportState {
    unsigned int flags;             /* +0x00  one bit per report, two per ride */
    int          zone_ll;           /* +0x04 */
    int          zone_adv;          /* +0x08 */
    int          zone_med;          /* +0x0c */
    int          zone_wes;          /* +0x10 */
    int          coaster;           /* +0x14 */
    int          dschool;           /* +0x18 */
    int          lflume;            /* +0x1c */
    int          bschool;           /* +0x20 */
    int          jcruise;           /* +0x24 */
    ReportArgs   num_attractions;   /* +0x28 */
    ReportArgs   var_attractions;   /* +0x30 */
    ReportArgs   ride_access;       /* +0x38 */
    ReportArgs   num_vis;           /* +0x40 */
    ReportArgs   num_food;          /* +0x48 */
    ReportArgs   var_food;          /* +0x50 */
    ReportArgs   vis;               /* +0x58  HAPPPY_VIS and HUNGRY_VIS */
    ReportArgs   power;             /* +0x60 */
    ReportArgs   working_rides;     /* +0x68 */
    ReportArgs   studded;           /* +0x70 */
    ReportArgs   num_scenery;       /* +0x78 */
    ReportArgs   var_scenery;       /* +0x80 */
    ReportArgs   cov_scenery;       /* +0x88 */
    ReportArgs   num_shops;         /* +0x90 */
    ReportArgs   var_shops;         /* +0x98 */
} ReportState;                      /* 0xa0 */

extern ReportState g_report_state;  /* 0x00665ff8 */

/* ---- the zone reports: one number, one bit ------------------------------- */

// FUNCTION: LEGOLAND 0x004443b0
void SetReport_ZONE_LL(int a, int b)
{
    if (a | b) {
        g_report_state.flags |= 1;
        g_report_state.zone_ll = a;
    } else {
        g_report_state.flags &= ~1;
    }
}

// FUNCTION: LEGOLAND 0x004443e0
void SetReport_ZONE_ADV(int a, int b)
{
    if (a | b) {
        g_report_state.flags |= 2;
        g_report_state.zone_adv = a;
    } else {
        g_report_state.flags &= ~2;
    }
}

// FUNCTION: LEGOLAND 0x00444410
void SetReport_ZONE_MED(int a, int b)
{
    if (a | b) {
        g_report_state.flags |= 4;
        g_report_state.zone_med = a;
    } else {
        g_report_state.flags &= ~4;
    }
}

// FUNCTION: LEGOLAND 0x00444440
void SetReport_ZONE_WES(int a, int b)
{
    if (a | b) {
        g_report_state.flags |= 8;
        g_report_state.zone_wes = a;
    } else {
        g_report_state.flags &= ~8;
    }
}

/* ---- the five rides: one number, a two-bit mode from the second ---------- */

// FUNCTION: LEGOLAND 0x00444470
void SetReport_COASTER(int a, int b)
{
    if (a | b) {
        g_report_state.flags |= (b & 3) << 4;
        g_report_state.coaster = a;
    } else {
        g_report_state.flags &= ~0x30;
    }
}

// FUNCTION: LEGOLAND 0x004444b0
void SetReport_DSCHOOL(int a, int b)
{
    if (a | b) {
        g_report_state.flags |= (b & 3) << 6;
        g_report_state.dschool = a;
    } else {
        g_report_state.flags &= ~0xc0;
    }
}

// FUNCTION: LEGOLAND 0x004444f0
void SetReport_LFLUME(int a, int b)
{
    if (a | b) {
        g_report_state.flags |= (b & 3) << 8;
        g_report_state.lflume = a;
    } else {
        g_report_state.flags &= ~0x300;
    }
}

// FUNCTION: LEGOLAND 0x00444530
void SetReport_BSCHOOL(int a, int b)
{
    if (a | b) {
        g_report_state.flags |= (b & 3) << 10;
        g_report_state.bschool = a;
    } else {
        g_report_state.flags &= ~0xc00;
    }
}

// FUNCTION: LEGOLAND 0x00444570
void SetReport_JCRUISE(int a, int b)
{
    if (a | b) {
        g_report_state.flags |= (b & 3) << 12;
        g_report_state.jcruise = a;
    } else {
        g_report_state.flags &= ~0x3000;
    }
}

/* ---- the counted reports: both numbers, one bit -------------------------- */

// FUNCTION: LEGOLAND 0x004445b0
void SetReport_NUM_ATTRACTIONS(int a, int b)
{
    if (a | b) {
        g_report_state.flags |= 0x4000;
        g_report_state.num_attractions.a = a;
        g_report_state.num_attractions.b = b;
    } else {
        g_report_state.flags &= ~0x4000;
    }
}

// FUNCTION: LEGOLAND 0x004445f0
void SetReport_VAR_ATTRACTIONS(int a, int b)
{
    if (a | b) {
        g_report_state.flags |= 0x8000;
        g_report_state.var_attractions.a = a;
        g_report_state.var_attractions.b = b;
    } else {
        g_report_state.flags &= ~0x8000;
    }
}

// FUNCTION: LEGOLAND 0x00444630
void SetReport_RIDE_ACCESS(int a, int b)
{
    if (a | b) {
        g_report_state.flags |= 0x40000;
        g_report_state.ride_access.a = a;
        g_report_state.ride_access.b = b;
    } else {
        g_report_state.flags &= ~0x40000;
    }
}

// FUNCTION: LEGOLAND 0x00444670
void SetReport_NUM_SCENERY(int a, int b)
{
    if (a | b) {
        g_report_state.flags |= 0x8000000;
        g_report_state.num_scenery.a = a;
        g_report_state.num_scenery.b = b;
    } else {
        g_report_state.flags &= ~0x8000000;
    }
}

// FUNCTION: LEGOLAND 0x004446b0
void SetReport_VAR_SCENERY(int a, int b)
{
    if (a | b) {
        g_report_state.flags |= 0x10000000;
        g_report_state.var_scenery.a = a;
        g_report_state.var_scenery.b = b;
    } else {
        g_report_state.flags &= ~0x10000000;
    }
}

// FUNCTION: LEGOLAND 0x004446f0
void SetReport_COV_SCENERY(int a, int b)
{
    if (a | b) {
        g_report_state.flags |= 0x20000000;
        g_report_state.cov_scenery.a = a;
        g_report_state.cov_scenery.b = b;
    } else {
        g_report_state.flags &= ~0x20000000;
    }
}

// FUNCTION: LEGOLAND 0x00444730
void SetReport_NUM_FOOD(int a, int b)
{
    if (a | b) {
        g_report_state.flags |= 0x10000;
        g_report_state.num_food.a = a;
        g_report_state.num_food.b = b;
    } else {
        g_report_state.flags &= ~0x10000;
    }
}

// FUNCTION: LEGOLAND 0x00444770
void SetReport_VAR_FOOD(int a, int b)
{
    if (a | b) {
        g_report_state.flags |= 0x20000;
        g_report_state.var_food.a = a;
        g_report_state.var_food.b = b;
    } else {
        g_report_state.flags &= ~0x20000;
    }
}

// FUNCTION: LEGOLAND 0x004447b0
void SetReport_NUM_SHOPS(int a, int b)
{
    if (a | b) {
        g_report_state.flags |= 0x40000000;
        g_report_state.num_shops.a = a;
        g_report_state.num_shops.b = b;
    } else {
        g_report_state.flags &= ~0x40000000;
    }
}

// FUNCTION: LEGOLAND 0x004447f0
void SetReport_VAR_SHOPS(int a, int b)
{
    if (a | b) {
        g_report_state.flags |= 0x80000000;
        g_report_state.var_shops.a = a;
        g_report_state.var_shops.b = b;
    } else {
        g_report_state.flags &= ~0x80000000;
    }
}

// FUNCTION: LEGOLAND 0x00444830
void SetReport_NUM_VIS(int a, int b)
{
    if (a | b) {
        g_report_state.flags |= 0x80000;
        g_report_state.num_vis.a = a;
        g_report_state.num_vis.b = b;
    } else {
        g_report_state.flags &= ~0x80000;
    }
}

/* HAPPPY_VIS (the table's spelling) and HUNGRY_VIS share the slot pair. */
// FUNCTION: LEGOLAND 0x00444870
void SetReport_HAPPPY_VIS(int a, int b)
{
    if (a | b) {
        g_report_state.flags |= 0x1000000;
        g_report_state.vis.a = a;
        g_report_state.vis.b = b;
    } else {
        g_report_state.flags &= ~0x1000000;
    }
}

// FUNCTION: LEGOLAND 0x004448b0
void SetReport_HUNGRY_VIS(int a, int b)
{
    if (a | b) {
        g_report_state.flags |= 0x4000000;
        g_report_state.vis.a = a;
        g_report_state.vis.b = b;
    } else {
        g_report_state.flags &= ~0x4000000;
    }
}

// FUNCTION: LEGOLAND 0x004448f0
void SetReport_POWER(int a, int b)
{
    if (a | b) {
        g_report_state.flags |= 0x200000;
        g_report_state.power.a = a;
        g_report_state.power.b = b;
    } else {
        g_report_state.flags &= ~0x200000;
    }
}

// FUNCTION: LEGOLAND 0x00444930
void SetReport_WORKING_RIDES(int a, int b)
{
    if (a | b) {
        g_report_state.flags |= 0x400000;
        g_report_state.working_rides.a = a;
        g_report_state.working_rides.b = b;
    } else {
        g_report_state.flags &= ~0x400000;
    }
}

// FUNCTION: LEGOLAND 0x00444970
void SetReport_STUDDED(int a, int b)
{
    if (a | b) {
        g_report_state.flags |= 0x800000;
        g_report_state.studded.a = a;
        g_report_state.studded.b = b;
    } else {
        g_report_state.flags &= ~0x800000;
    }
}
