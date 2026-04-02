/**
 * ui/phase1_ui.cpp
 * MMUKO-OS Phase 1 UI — OBINexus::Phase1UIWrapper C++ implementation
 * OBINexus Computing | Nnamdi Michael Okpala
 * Version: 0.1-DRAFT | 29 March 2026
 *
 * Thin RAII wrapper around the C phase1_ui functions.
 * Useful for embedding the Phase 1 UI into larger C++ applications.
 *
 * Build: g++ -std=c++17 -I. -c ui/phase1_ui.cpp -o build/obj/ui/phase1_ui_cpp.o
 */

#include "phase1_ui.h"
#include <cstdio>
#ifdef _WIN32
#  include <conio.h>
#endif

namespace OBINexus {

Phase1UIWrapper::Phase1UIWrapper() {
    phase1_ui_init(&m_state);
}

Phase1UIWrapper::~Phase1UIWrapper() {
    if (m_state.running)
        phase1_ui_cleanup();
}

void Phase1UIWrapper::step() {
    phase1_ui_step(&m_state);
}

void Phase1UIWrapper::next_phase() {
    phase1_ui_next_phase(&m_state);
}

void Phase1UIWrapper::calibrate() {
    phase1_ui_calibrate(&m_state);
}

void Phase1UIWrapper::render() const {
    phase1_ui_render(&m_state);
}

void Phase1UIWrapper::run_loop() {
    while (m_state.running) {
        render();
#ifdef _WIN32
        int c = _getch();
#else
        int c = std::getchar();
#endif
        switch (c) {
            case 's': case 'S': step();        break;
            case 'c': case 'C': calibrate();   break;
            case 'n': case 'N': next_phase();  break;
            case 'q': case 'Q': m_state.running = 0; break;
            default: break;
        }
    }
}

} /* namespace OBINexus */
