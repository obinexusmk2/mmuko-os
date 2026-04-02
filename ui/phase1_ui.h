/**
 * ui/phase1_ui.h
 * MMUKO-OS Phase 1 BIOS Firmware Terminal UI — header
 * OBINexus Computing | Nnamdi Michael Okpala
 * Version: 0.1-DRAFT | 29 March 2026
 *
 * Pure ANSI escape codes; no ncurses dependency.
 * POSIX: uses termios for raw keyboard input.
 * Windows (MSYS2/Git Bash): uses _getch() from <conio.h>.
 *
 * Render layout:
 *   +====================================================+
 *   |  OBINexus MMUKO-OS | Phase 1 BIOS Firmware UI      |
 *   |  CCYY-MM-DD HH:MM:SS                               |
 *   +====================================================+
 *
 *   NSIGII: [N:1] [S:2] [I:3] [G:4] [I:5] [I:6]
 *   Phase: 1 (NEED_STATE_INIT)
 *
 *   SPIN SIMULATION:
 *     Alpha (North/+1): x=…  orient=…  elev=…  state=…
 *     Bravo (South/-1): x=…  orient=…  elev=…  state=…
 *     Steps: …  Work: …  Coherent: [YES/NO]
 *
 *   MEMBRANE OUTCOME: [PASS/HOLD]
 *
 *   MOSAIC MEMORY:
 *     #BC5FA9 [color=4] state=YES    data=0xA0
 *     …
 *
 *   Commands: [s]tep  [c]alibrate  [n]ext-phase  [q]uit
 */

#ifndef PHASE1_UI_H
#define PHASE1_UI_H

#include <stdint.h>
#include "firmware/bios_interface.h"  /* BIOSDateTime, SpinPair, MosaicEntry */

/* =========================================================================
 * Phase1UIState — complete UI state snapshot
 * ========================================================================= */
typedef struct {
    SpinPair     spin;             /* current spin simulation state          */
    BIOSDateTime dt;               /* last datetime read                     */
    uint32_t     current_phase;    /* NSIGII phase 1–6                       */
    int          running;          /* 0 = quit requested                     */
    MosaicEntry  mosaic[8];        /* 8 mosaic memory entries                */
} Phase1UIState;

/* =========================================================================
 * C API
 * ========================================================================= */
#ifdef __cplusplus
extern "C" {
#endif

/** Initialise state, populate mosaic, enable raw terminal input. */
void phase1_ui_init(Phase1UIState *s);

/** Render the full UI to stdout (clears screen first). */
void phase1_ui_render(const Phase1UIState *s);

/** Advance spin simulation one step and refresh datetime. */
void phase1_ui_step(Phase1UIState *s);

/** Cycle current_phase 1→2→…→6→1 and refresh datetime. */
void phase1_ui_next_phase(Phase1UIState *s);

/** Reset spin to initial state (calibrate). */
void phase1_ui_calibrate(Phase1UIState *s);

/** Restore terminal settings, show cursor, reset colors. */
void phase1_ui_cleanup(void);

#ifdef __cplusplus
}  /* extern "C" */

/* =========================================================================
 * C++ convenience wrapper (declaration only; implemented in phase1_ui.cpp)
 * ========================================================================= */
namespace OBINexus {

class Phase1UIWrapper {
    Phase1UIState m_state;
public:
    Phase1UIWrapper();
    ~Phase1UIWrapper();
    void step();
    void next_phase();
    void calibrate();
    void render() const;
    void run_loop();
    bool is_running()    const { return m_state.running != 0; }
    int  current_phase() const { return static_cast<int>(m_state.current_phase); }
};

} /* namespace OBINexus */
#endif /* __cplusplus */

#endif /* PHASE1_UI_H */
