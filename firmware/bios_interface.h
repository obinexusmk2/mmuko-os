/**
 * firmware/bios_interface.h
 * MMUKO-OS BIOS Firmware Interface — Spin Operators, Mosaic Memory, RTC
 * OBINexus Computing | Nnamdi Michael Okpala
 * Version: 0.1-DRAFT | 29 March 2026
 *
 * Models the magnetic spin/polarization system from the transcript:
 *   - SpinOperator: three-state (alpha/beta/gamma) magnetic dipole
 *   - SpinPair:     alpha(north) + bravo(south) coupled attraction system
 *   - MosaicEntry:  color-coded hexadecimal memory cell (BC 5F A9 …)
 *   - BIOSDateTime: mirrors BIOS INT 0x1A RTC register layout (BCD)
 *   - GPSLockState: GPS half-lock / locked / not-locked (from transcript)
 *
 * 2/3 + 2/3 + 1/4 coherence rule (from transcript):
 *   - alpha_score >= 2 (2 pts for .alpha==YES, 1 pt for .beta==YES)
 *   - bravo_score >= 2 (same scoring)
 *   - gamma == YES (1/4 quarter gate)
 *
 * Host build: gcc -std=c11 -I. -Iinclude -c firmware/bios_interface.c
 */

#ifndef BIOS_INTERFACE_H
#define BIOS_INTERFACE_H

#include <stdint.h>
#include <stdbool.h>
#include "heartfull_firmware.h"   /* TrinaryState, MembraneOutcome */

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * BIOSDateTime — mirrors BIOS INT 0x1A register layout (BCD-decoded)
 *   INT 0x1A AH=4: CH=century, CL=year, DH=month, DL=day
 *   INT 0x1A AH=2: CH=hours,   CL=minutes, DH=seconds
 * ========================================================================= */
typedef struct {
    uint8_t century;   /* e.g. 0x20 for year 2026 */
    uint8_t year;      /* e.g. 0x26 for year 2026 */
    uint8_t month;     /* 01–12 */
    uint8_t day;       /* 01–31 */
    uint8_t hour;      /* 00–23 */
    uint8_t min;       /* 00–59 */
    uint8_t sec;       /* 00–59 */
} BIOSDateTime;

/* =========================================================================
 * GPSLockState — from transcript: "GPS is half locked in half locked in"
 * Represents three states of positional certainty in the system
 * ========================================================================= */
typedef enum {
    GPS_HALF_LOCKED =  0,   /* uncertain — MAYBE state                      */
    GPS_LOCKED      =  1,   /* fully locked — YES state                     */
    GPS_NOT_LOCKED  = -1    /* not locked — NO state                        */
} GPSLockState;

/* =========================================================================
 * SpinOperator — single magnetic dipole with three-state spin vector
 *
 * From transcript: "if you have a spin you have north east south west …
 * you have a kind of here … you're going to have the angle"
 *
 * orientation_x100: integer-scaled angle (0–628 ≡ 0–2π×100)
 *   Each step advances by 5 units; wraps at 628.
 *   Avoids float in the struct while preserving precision.
 * ========================================================================= */
typedef struct {
    TrinaryState alpha;            /* primary   spin trinary (YES/NO/MAYBE)  */
    TrinaryState beta;             /* secondary spin trinary                 */
    TrinaryState gamma;            /* tertiary  spin trinary                 */
    int32_t      orientation_x100; /* 0–628 (2π × 100 scaled integer)       */
    int32_t      elevation;        /* accumulated path length (work proxy)   */
    int32_t      position_x;       /* 1-D horizontal position               */
    int8_t       polarity;         /* +1 = north pole, -1 = south pole       */
} SpinOperator;

/* =========================================================================
 * SpinPair — alpha(north) + bravo(south) coupled magnet system
 *
 * From transcript: "you have two magnets … alpha and bravo …
 * opposite polarity => attract … work = force × displacement"
 *
 * Alpha starts at x = -10 (north, +1 polarity)
 * Bravo starts at x = +10 (south, -1 polarity)
 * Each step: alpha.x++, bravo.x-- (attraction)
 * Work accumulates as Σ force_i = Σ 1/dist²
 * ========================================================================= */
typedef struct {
    SpinOperator alpha_magnet;   /* north pole — starts at x=-10            */
    SpinOperator bravo_magnet;   /* south pole — starts at x=+10            */
    uint32_t     step_count;     /* steps taken so far                       */
    double       work_done;      /* accumulated work (sum of 1/dist²)        */
} SpinPair;

/* =========================================================================
 * MosaicEntry — one cell in the mosaic (color-coded hex memory)
 *
 * From transcript: "mosaic is basically about … color-coded numbers …
 * BC B 5F A9 represents a memory region"
 *
 * color_code = address[0] % 8 → deterministic ANSI color assignment
 * ========================================================================= */
typedef struct {
    uint8_t      address[3];     /* 3-byte hex address, e.g. {0xBC,0x5F,0xA9} */
    uint8_t      color_code;     /* 0–7 (ANSI color index, addr[0] % 8)       */
    uint8_t      data;           /* byte value stored at this address          */
    TrinaryState spin_state;     /* trinary state of this memory cell          */
} MosaicEntry;

/* =========================================================================
 * FUNCTION DECLARATIONS
 * ========================================================================= */

/**
 * bios_datetime_now — simulate BIOS INT 0x1A via host time()+localtime()
 * On bare metal this would call INT 0x1A AH=4 (date) and AH=2 (time).
 */
BIOSDateTime  bios_datetime_now(void);

/**
 * bios_datetime_format — format as "CCYY-MM-DD HH:MM:SS"
 * buf must be at least 20 bytes.
 */
void          bios_datetime_format(const BIOSDateTime *dt, char *buf, int bufsz);

/**
 * spin_pair_init — create a fresh SpinPair in the initial attract configuration.
 * alpha at x=-10 (north/+1), bravo at x=+10 (south/-1), all orientations 0.
 */
SpinPair      spin_pair_init(void);

/**
 * spin_pair_step — advance the simulation one step:
 *   1. Compute drift = bravo.x - alpha.x (distance)
 *   2. Opposite polarity → attract: alpha.x++, bravo.x--
 *   3. orientation_x100 += 5 (wrap at 628 ≡ 2π×100)
 *   4. elevation += |distance|
 *   5. work_done += 1/distance²  (force × unit displacement)
 *   6. Update beta/gamma to YES when distance ≤ 4 / ≤ 2
 */
void          spin_pair_step(SpinPair *pair);

/**
 * spin_valid_state — 2/3 + 2/3 + 1/4 coherence check (from transcript).
 * Returns true when the spin pair has reached a valid superposition state.
 */
bool          spin_valid_state(const SpinPair *pair);

/**
 * mosaic_encode — encode a 3-byte address into a MosaicEntry.
 * color_code = addr[0] % 8 (deterministic ANSI color mapping).
 */
MosaicEntry   mosaic_encode(const uint8_t addr[3], uint8_t data, TrinaryState state);

/**
 * mosaic_color_for — return ANSI escape string for color_code 0–7.
 * Returns "\033[0m" for 0 (reset) and "\033[3Xm" for 1–7.
 */
const char *  mosaic_color_for(uint8_t color_code);

#ifdef __cplusplus
}
#endif

#endif /* BIOS_INTERFACE_H */
