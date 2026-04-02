/**
 * ui/phase1_ui.c
 * MMUKO-OS Phase 1 BIOS Firmware Terminal UI — implementation + main()
 * OBINexus Computing | Nnamdi Michael Okpala
 * Version: 0.1-DRAFT | 29 March 2026
 *
 * Build (standalone): gcc -std=c11 -I. -o build/phase1_ui
 *                         ui/phase1_ui.c firmware/bios_interface.c -lm
 * Run:                ./build/phase1_ui
 *   [s] step spin   [c] calibrate   [n] next NSIGII phase   [q] quit
 */

#include "phase1_ui.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* =========================================================================
 * Platform keyboard input
 * ========================================================================= */
#ifdef _WIN32
#  include <conio.h>
#  define UI_GETCH()  _getch()
static void ui_raw_on(void)  { /* no-op on Windows — _getch() is already raw */ }
static void ui_raw_off(void) { /* no-op */ }
#else
#  include <termios.h>
#  include <unistd.h>
static struct termios g_saved_termios;

static void ui_raw_on(void) {
    struct termios raw;
    tcgetattr(STDIN_FILENO, &g_saved_termios);
    raw = g_saved_termios;
    raw.c_lflag &= (tcflag_t)~(ICANON | ECHO);
    raw.c_cc[VMIN]  = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}

static void ui_raw_off(void) {
    tcsetattr(STDIN_FILENO, TCSANOW, &g_saved_termios);
}

#  define UI_GETCH()  getchar()
#endif

/* =========================================================================
 * ANSI helpers
 * ========================================================================= */
#define A_CLEAR   "\033[2J\033[H"
#define A_RESET   "\033[0m"
#define A_BOLD    "\033[1m"
#define A_GREEN   "\033[1;32m"
#define A_YELLOW  "\033[1;33m"
#define A_RED     "\033[1;31m"
#define A_CYAN    "\033[1;36m"
#define A_HIDE_CUR "\033[?25l"
#define A_SHOW_CUR "\033[?25h"

/* =========================================================================
 * Phase name strings (indices 1–6)
 * ========================================================================= */
static const char *s_phase_names[7] = {
    "",
    "NEED_STATE_INIT",
    "SAFETY_SCAN",
    "IDENTITY_CAL",
    "GOVERNANCE",
    "INTERNAL_PROBE",
    "INTEGRITY_VERIFY",
};

/* NSIGII letters indexed 1–6 */
static const char s_nsigii_letters[] = "NSIGII";

/* Trinary state display strings (padded to 9 chars) */
static const char *trinary_label(TrinaryState s) {
    switch (s) {
        case TRINARY_YES:       return "YES      ";
        case TRINARY_NO:        return "NO       ";
        case TRINARY_MAYBE:     return "MAYBE    ";
        case TRINARY_MAYBE_NOT: return "MAYBE_NOT";
        default:                return "UNKNOWN  ";
    }
}

/* =========================================================================
 * phase1_ui_init
 * ========================================================================= */
void phase1_ui_init(Phase1UIState *s) {
    static const uint8_t addrs[8][3] = {
        {0xBC, 0x5F, 0xA9},
        {0x1E, 0x2F, 0x3C},
        {0xDE, 0xAD, 0xBE},
        {0xCA, 0xFE, 0x00},
        {0xFF, 0x00, 0x11},
        {0x7F, 0x80, 0x90},
        {0x42, 0x66, 0xAA},
        {0x0B, 0xA5, 0xE5},
    };
    static const TrinaryState states[8] = {
        TRINARY_YES,   TRINARY_YES,   TRINARY_YES,
        TRINARY_MAYBE, TRINARY_MAYBE, TRINARY_MAYBE,
        TRINARY_NO,    TRINARY_NO,
    };

    memset(s, 0, sizeof(*s));
    s->spin          = spin_pair_init();
    s->dt            = bios_datetime_now();
    s->current_phase = 1;
    s->running       = 1;

    for (int i = 0; i < 8; i++)
        s->mosaic[i] = mosaic_encode(addrs[i], (uint8_t)(0xA0 + i), states[i]);

    ui_raw_on();
    printf(A_HIDE_CUR);
    fflush(stdout);
}

/* =========================================================================
 * phase1_ui_cleanup
 * ========================================================================= */
void phase1_ui_cleanup(void) {
    ui_raw_off();
    printf(A_SHOW_CUR A_RESET "\n");
    fflush(stdout);
}

/* =========================================================================
 * phase1_ui_render
 * ========================================================================= */
void phase1_ui_render(const Phase1UIState *s) {
    char dtbuf[24];
    bios_datetime_format(&s->dt, dtbuf, (int)sizeof(dtbuf));

    printf(A_CLEAR);

    /* Header */
    printf(A_BOLD "+====================================================+\n" A_RESET);
    printf(A_BOLD "|  OBINexus MMUKO-OS | Phase 1 BIOS Firmware UI      |\n" A_RESET);
    printf(A_CYAN  "|  %-48s|\n" A_RESET, dtbuf);
    printf(A_BOLD "+====================================================+\n\n" A_RESET);

    /* NSIGII phase bar */
    printf("NSIGII: ");
    for (int p = 1; p <= 6; p++) {
        const char *col = (p < (int)s->current_phase) ? A_GREEN :
                          (p == (int)s->current_phase) ? A_YELLOW : A_RESET;
        printf("%s[%c:%d]" A_RESET " ",
               col, s_nsigii_letters[p - 1], p);
    }
    printf("\nPhase : %u  (%s)\n\n",
           s->current_phase,
           s->current_phase <= 6 ? s_phase_names[s->current_phase] : "?");

    /* Spin simulation */
    printf("SPIN SIMULATION:\n");
    printf("  Alpha (North/+1): x=%-5d  orient=%3d  elev=%-6d  spin=%s\n",
           s->spin.alpha_magnet.position_x,
           s->spin.alpha_magnet.orientation_x100,
           s->spin.alpha_magnet.elevation,
           trinary_label(s->spin.alpha_magnet.alpha));
    printf("  Bravo (South/-1): x=%-5d  orient=%3d  elev=%-6d  spin=%s\n",
           s->spin.bravo_magnet.position_x,
           s->spin.bravo_magnet.orientation_x100,
           s->spin.bravo_magnet.elevation,
           trinary_label(s->spin.bravo_magnet.alpha));

    bool coherent = spin_valid_state(&s->spin);
    printf("  Steps: %-4u  Work: %-14.6f  Coherent: %s\n\n",
           s->spin.step_count,
           s->spin.work_done,
           coherent ? A_GREEN "[YES]" A_RESET : A_RED "[NO]" A_RESET);

    /* Membrane outcome */
    bool phase_passed   = s->current_phase > 1;
    const char *outcome = (coherent && phase_passed) ? "PASS" : "HOLD";
    const char *ocol    = (coherent && phase_passed) ? A_GREEN : A_YELLOW;
    printf("MEMBRANE OUTCOME: %s[%s]%s\n\n", ocol, outcome, A_RESET);

    /* Mosaic memory */
    printf("MOSAIC MEMORY:\n");
    for (int i = 0; i < 8; i++) {
        const MosaicEntry *me = &s->mosaic[i];
        printf("  %s#%02X%02X%02X%s [color=%d]  state=%-9s  data=0x%02X\n",
               mosaic_color_for(me->color_code),
               me->address[0], me->address[1], me->address[2],
               A_RESET,
               me->color_code,
               me->spin_state == TRINARY_YES      ? "YES"      :
               me->spin_state == TRINARY_MAYBE    ? "MAYBE"    :
               me->spin_state == TRINARY_MAYBE_NOT? "MAYBE_NOT": "NO",
               me->data);
    }

    printf("\n" A_BOLD "Commands: [s]tep  [c]alibrate  [n]ext-phase  [q]uit\n" A_RESET);
    fflush(stdout);
}

/* =========================================================================
 * State-mutation helpers
 * ========================================================================= */
void phase1_ui_step(Phase1UIState *s) {
    spin_pair_step(&s->spin);
    s->dt = bios_datetime_now();
}

void phase1_ui_next_phase(Phase1UIState *s) {
    s->current_phase = (s->current_phase < 6) ? s->current_phase + 1 : 1;
    s->dt = bios_datetime_now();
}

void phase1_ui_calibrate(Phase1UIState *s) {
    s->spin          = spin_pair_init();
    s->current_phase = 1;
    s->dt            = bios_datetime_now();
}

/* =========================================================================
 * main — standalone test entry point
 * ========================================================================= */
int main(void) {
    Phase1UIState state;
    phase1_ui_init(&state);

    while (state.running) {
        phase1_ui_render(&state);
        int c = UI_GETCH();
        switch (c) {
            case 's': case 'S': phase1_ui_step(&state);      break;
            case 'c': case 'C': phase1_ui_calibrate(&state); break;
            case 'n': case 'N': phase1_ui_next_phase(&state);break;
            case 'q': case 'Q': state.running = 0;           break;
            default: break;
        }
    }

    phase1_ui_cleanup();
    printf("MMUKO-OS Phase 1 UI terminated.\n");
    return 0;
}
