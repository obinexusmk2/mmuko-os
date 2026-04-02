/**
 * firmware/bios_interface.cpp
 * MMUKO-OS BIOS Firmware Interface — C++ implementation + C-linkage exports
 * OBINexus Computing | Nnamdi Michael Okpala
 * Version: 0.1-DRAFT | 29 March 2026
 *
 * All class logic lives in bios_interface_cpp.h (inline methods).
 * This file provides the static global instance and C-linkage export stubs.
 */

#include "bios_interface_cpp.h"
#include <string>

/* =========================================================================
 * Static global Phase1UI instance (used by C-linkage exports)
 * Pattern mirrors nsigii_cpp_wrapper.cpp g_membrane pattern.
 * ========================================================================= */
static OBINexus::BIOS::Phase1UI  g_phase1_ui;
static std::string               g_datetime_buf;

extern "C" {

/**
 * bios_cpp_datetime_format — return current date/time as a C string.
 * The returned pointer is valid until the next call.
 */
const char* bios_cpp_datetime_format(void) {
    OBINexus::BIOS::DateTimeService svc;
    g_datetime_buf = svc.format_string();
    return g_datetime_buf.c_str();
}

/**
 * bios_cpp_spin_step — advance the global spin simulation one step.
 */
void bios_cpp_spin_step(void) {
    g_phase1_ui.step();
}

/**
 * bios_cpp_mosaic_display — render the global Phase1UI state to stdout.
 */
void bios_cpp_mosaic_display(void) {
    g_phase1_ui.render();
}

} /* extern "C" */
