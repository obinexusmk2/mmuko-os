/**
 * firmware/bios_interface.c
 * MMUKO-OS BIOS Firmware Interface — implementation
 * OBINexus Computing | Nnamdi Michael Okpala
 * Version: 0.1-DRAFT | 29 March 2026
 */

#include "bios_interface.h"   /* found via same directory */

#include <time.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* =========================================================================
 * ANSI color table (color_code 0–7)
 * 0 = reset, 1 = red, 2 = green, 3 = yellow,
 * 4 = blue, 5 = magenta, 6 = cyan, 7 = white
 * ========================================================================= */
static const char *s_ansi_colors[8] = {
    "\033[0m",    /* 0: reset    */
    "\033[31m",   /* 1: red      */
    "\033[32m",   /* 2: green    */
    "\033[33m",   /* 3: yellow   */
    "\033[34m",   /* 4: blue     */
    "\033[35m",   /* 5: magenta  */
    "\033[36m",   /* 6: cyan     */
    "\033[37m",   /* 7: white    */
};

/* =========================================================================
 * bios_datetime_now
 *
 * On the host, reads the real clock via time()+localtime().
 * On bare metal, this would be replaced with BIOS INT 0x1A calls:
 *   AH=4 → date:  CH=century, CL=year, DH=month, DL=day  (BCD)
 *   AH=2 → time:  CH=hours,   CL=minutes, DH=seconds      (BCD)
 * ========================================================================= */
BIOSDateTime bios_datetime_now(void) {
    time_t t = time(NULL);
    const struct tm *lt = localtime(&t);
    BIOSDateTime dt;
    int full_year = lt->tm_year + 1900;
    dt.century = (uint8_t)(full_year / 100);
    dt.year    = (uint8_t)(full_year % 100);
    dt.month   = (uint8_t)(lt->tm_mon + 1);
    dt.day     = (uint8_t)(lt->tm_mday);
    dt.hour    = (uint8_t)(lt->tm_hour);
    dt.min     = (uint8_t)(lt->tm_min);
    dt.sec     = (uint8_t)(lt->tm_sec);
    return dt;
}

/* =========================================================================
 * bios_datetime_format
 * Writes "CCYY-MM-DD HH:MM:SS\0" into buf (needs >= 20 bytes).
 * ========================================================================= */
void bios_datetime_format(const BIOSDateTime *dt, char *buf, int bufsz) {
    snprintf(buf, (size_t)bufsz, "%02u%02u-%02u-%02u %02u:%02u:%02u",
             dt->century, dt->year, dt->month, dt->day,
             dt->hour, dt->min, dt->sec);
}

/* =========================================================================
 * spin_pair_init
 *
 * Alpha (north/+1) at x=-10, bravo (south/-1) at x=+10.
 * All trinary states start as MAYBE (uncertain) except alpha of each magnet
 * which starts as YES (the primary spin is confirmed).
 * ========================================================================= */
SpinPair spin_pair_init(void) {
    SpinPair p;
    memset(&p, 0, sizeof(p));

    /* Alpha magnet — north pole */
    p.alpha_magnet.position_x       = -10;
    p.alpha_magnet.polarity          =  1;
    p.alpha_magnet.alpha             = TRINARY_YES;
    p.alpha_magnet.beta              = TRINARY_MAYBE;
    p.alpha_magnet.gamma             = TRINARY_MAYBE;
    p.alpha_magnet.orientation_x100  = 0;
    p.alpha_magnet.elevation         = 0;

    /* Bravo magnet — south pole */
    p.bravo_magnet.position_x       = 10;
    p.bravo_magnet.polarity          = -1;
    p.bravo_magnet.alpha             = TRINARY_YES;
    p.bravo_magnet.beta              = TRINARY_MAYBE;
    p.bravo_magnet.gamma             = TRINARY_MAYBE;
    p.bravo_magnet.orientation_x100  = 0;
    p.bravo_magnet.elevation         = 0;

    p.step_count = 0;
    p.work_done  = 0.0;
    return p;
}

/* =========================================================================
 * spin_pair_step
 *
 * Implements one simulation step of the magnetic attraction model.
 *
 * Transcript: "opposite polarity => attract … work = force × displacement"
 * Transcript: "orientation += 5 (wrap at 628 ≡ 2π×100)"
 * Transcript: "elevation += distance (path length)"
 *
 * Force model: F = 1/d² (Coulomb-style inverse square law, unit charges)
 * Work per step: W += F × 1  (displacement = 1 unit per step)
 *
 * Trinary state promotion:
 *   distance ≤ 4 → beta = YES   (proximity threshold)
 *   distance ≤ 2 → gamma = YES  (coherence threshold)
 * ========================================================================= */
void spin_pair_step(SpinPair *pair) {
    int32_t dist = pair->bravo_magnet.position_x - pair->alpha_magnet.position_x;
    int32_t abs_dist = dist >= 0 ? dist : -dist;
    if (abs_dist == 0) abs_dist = 1;   /* prevent division by zero */

    double force = 1.0 / ((double)abs_dist * (double)abs_dist);

    /* Attract: alpha moves right (+1), bravo moves left (-1) */
    pair->alpha_magnet.position_x++;
    pair->bravo_magnet.position_x--;

    /* Orientation: advance 5 units (2π×100 / 125 steps ≈ one full rotation) */
    pair->alpha_magnet.orientation_x100 = (pair->alpha_magnet.orientation_x100 + 5) % 628;
    pair->bravo_magnet.orientation_x100 = (pair->bravo_magnet.orientation_x100 + 5) % 628;

    /* Elevation accumulates total path length */
    pair->alpha_magnet.elevation += abs_dist;
    pair->bravo_magnet.elevation += abs_dist;

    /* Work = Σ force × unit displacement */
    pair->work_done += force;
    pair->step_count++;

    /* Promote trinary states based on proximity */
    if (abs_dist <= 4) {
        pair->alpha_magnet.beta = TRINARY_YES;
        pair->bravo_magnet.beta = TRINARY_YES;
    }
    if (abs_dist <= 2) {
        pair->alpha_magnet.gamma = TRINARY_YES;
        pair->bravo_magnet.gamma = TRINARY_YES;
    }
}

/* =========================================================================
 * spin_valid_state — 2/3 + 2/3 + 1/4 coherence check
 *
 * From transcript: "2/3 2/3 one quarter spin state … 2/3 and 2/3 start …
 * you have three operator and three states … valid spin state"
 *
 * Scoring:
 *   alpha_score = 2 × (alpha.alpha==YES) + 1 × (alpha.beta==YES)  → need ≥ 2
 *   bravo_score = 2 × (bravo.alpha==YES) + 1 × (bravo.beta==YES)  → need ≥ 2
 *   gamma_ok    = (alpha.gamma == YES)                              → need 1
 *
 * Interpretation: at minimum the primary spin (α) must be YES for each magnet.
 * ========================================================================= */
bool spin_valid_state(const SpinPair *pair) {
    int alpha_score = (pair->alpha_magnet.alpha == TRINARY_YES ? 2 : 0)
                    + (pair->alpha_magnet.beta  == TRINARY_YES ? 1 : 0);
    int bravo_score = (pair->bravo_magnet.alpha == TRINARY_YES ? 2 : 0)
                    + (pair->bravo_magnet.beta  == TRINARY_YES ? 1 : 0);
    int gamma_ok    = (pair->alpha_magnet.gamma == TRINARY_YES ? 1 : 0);
    return alpha_score >= 2 && bravo_score >= 2 && gamma_ok >= 1;
}

/* =========================================================================
 * mosaic_encode
 *
 * Encode a 3-byte address into a MosaicEntry.
 * color_code = addr[0] % 8 (deterministic, reproducible from the address).
 * ========================================================================= */
MosaicEntry mosaic_encode(const uint8_t addr[3], uint8_t data, TrinaryState state) {
    MosaicEntry e;
    e.address[0]  = addr[0];
    e.address[1]  = addr[1];
    e.address[2]  = addr[2];
    e.color_code  = (uint8_t)(addr[0] % 8);
    e.data        = data;
    e.spin_state  = state;
    return e;
}

/* =========================================================================
 * mosaic_color_for
 * Returns the ANSI escape string for color_code 0–7.
 * ========================================================================= */
const char *mosaic_color_for(uint8_t color_code) {
    if (color_code > 7) color_code = 0;
    return s_ansi_colors[color_code];
}
