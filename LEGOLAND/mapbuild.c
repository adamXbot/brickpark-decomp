/* LEGOLAND — map build-time helpers (called by LoadBaseMap).
 *
 * The build-stat accumulators at 0x00667ce0..0x00667cfc are the running
 * footprint/power tallies that PutObjOnMap advances as each object is placed;
 * LoadBaseMap resets them (via ResetBuildStats) before it walks the .MAP. */
#include "legoland.h"

extern int g_area_total;   /* 0x00667ce0 */
extern int g_area_type1;   /* 0x00667ce4 */
extern int g_area_type4;   /* 0x00667ce8 */
extern int g_area_type5;   /* 0x00667cec */
extern int g_area_type3;   /* 0x00667cf0 */
extern int g_count_env;    /* 0x00667cf4 */
extern int g_area_type2;   /* 0x00667cf8 */
extern int g_stat_cfc;     /* 0x00667cfc */

// FUNCTION: LEGOLAND 0x00459880
void ResetBuildStats(void)
{
    g_area_total = 0;
    g_area_type1 = 0;
    g_area_type4 = 0;
    g_area_type5 = 0;
    g_area_type3 = 0;
    g_count_env = 0;
    g_area_type2 = 0;
    g_stat_cfc = 0;
}
