/**
 * firmware/bios_interface_cpp.h
 * MMUKO-OS BIOS Firmware Interface — C++17 RAII wrappers
 * OBINexus Computing | Nnamdi Michael Okpala
 * Version: 0.1-DRAFT | 29 March 2026
 *
 * Namespace: OBINexus::BIOS  (mirrors existing OBINexus::NSIGII)
 *
 * Classes:
 *   DateTimeService  — reads host/BIOS RTC, formats date/time string
 *   SpinSystem       — wraps SpinPair, drives attraction simulation
 *   MosaicMemory     — 64-entry color-coded hex memory store
 *   Phase1UI         — combines all three, renders to stdout
 *
 * C-linkage exports (for Python ctypes / .NET P/Invoke):
 *   bios_cpp_datetime_format() → const char*
 *   bios_cpp_spin_step()       → void
 *   bios_cpp_mosaic_display()  → void
 */

#ifndef BIOS_INTERFACE_CPP_H
#define BIOS_INTERFACE_CPP_H

#include <string>
#include <array>
#include <cstdio>
#include <cstring>

extern "C" {
#include "bios_interface.h"
}

namespace OBINexus::BIOS {

/* =========================================================================
 * DateTimeService — RAII, reads RTC once per call
 * ========================================================================= */
class DateTimeService {
public:
    /** Read current date/time (calls bios_datetime_now each time). */
    BIOSDateTime read() const {
        return bios_datetime_now();
    }

    /** Format as "CCYY-MM-DD HH:MM:SS". */
    std::string format_string() const {
        char buf[24];
        BIOSDateTime dt = read();
        bios_datetime_format(&dt, buf, static_cast<int>(sizeof(buf)));
        return std::string(buf);
    }
};

/* =========================================================================
 * SpinSystem — wraps SpinPair, drives magnetic attraction simulation
 * ========================================================================= */
class SpinSystem {
    SpinPair m_pair;
public:
    SpinSystem()  : m_pair(spin_pair_init()) {}

    void     step()          { spin_pair_step(&m_pair); }
    double   work_done()   const { return m_pair.work_done; }
    bool     is_coherent() const { return spin_valid_state(&m_pair) != 0; }
    uint32_t steps()       const { return m_pair.step_count; }

    /** One-line status: "alpha x=…  orient=…  |  bravo x=…  orient=…  |  work=…" */
    std::string display() const {
        char buf[256];
        std::snprintf(buf, sizeof(buf),
            "alpha x=%-5d orient=%3d | bravo x=%-5d orient=%3d | work=%.6f",
            m_pair.alpha_magnet.position_x,
            m_pair.alpha_magnet.orientation_x100,
            m_pair.bravo_magnet.position_x,
            m_pair.bravo_magnet.orientation_x100,
            m_pair.work_done);
        return std::string(buf);
    }

    const SpinPair& raw() const { return m_pair; }
    void reset() { m_pair = spin_pair_init(); }
};

/* =========================================================================
 * MosaicMemory — 64-entry color-coded hexadecimal memory store
 * ========================================================================= */
class MosaicMemory {
    std::array<MosaicEntry, 64> m_entries{};
    int m_count = 0;
public:
    /** Encode addr[3] + data + state → append to internal store. */
    void encode(const uint8_t addr[3], uint8_t data, TrinaryState state) {
        if (m_count < 64)
            m_entries[static_cast<std::size_t>(m_count++)] = mosaic_encode(addr, data, state);
    }

    /** Multi-line ANSI-colored display string. */
    std::string display() const {
        std::string out;
        char line[160];
        for (int i = 0; i < m_count; i++) {
            const MosaicEntry &e = m_entries[static_cast<std::size_t>(i)];
            std::snprintf(line, sizeof(line),
                "  %s#%02X%02X%02X\033[0m [color=%d] state=%-9s data=0x%02X\n",
                mosaic_color_for(e.color_code),
                e.address[0], e.address[1], e.address[2],
                e.color_code,
                e.spin_state == TRINARY_YES      ? "YES"      :
                e.spin_state == TRINARY_MAYBE    ? "MAYBE"    :
                e.spin_state == TRINARY_MAYBE_NOT? "MAYBE_NOT": "NO",
                e.data);
            out += line;
        }
        return out;
    }

    /** Return ANSI escape for entry at idx. */
    const char *get_color_ansi(int idx) const {
        if (idx < 0 || idx >= m_count) return "\033[0m";
        return mosaic_color_for(m_entries[static_cast<std::size_t>(idx)].color_code);
    }

    int count() const { return m_count; }
};

/* =========================================================================
 * Phase1UI — combines DateTimeService + SpinSystem + MosaicMemory,
 *             renders the complete Phase 1 firmware UI to stdout.
 * ========================================================================= */
class Phase1UI {
    DateTimeService m_dt;
    SpinSystem      m_spin;
    MosaicMemory    m_mosaic;
    bool            m_active = false;

public:
    Phase1UI() {
        /* Pre-populate mosaic with 8 canonical entries */
        static const uint8_t addrs[8][3] = {
            {0xBC,0x5F,0xA9}, {0x1E,0x2F,0x3C}, {0xDE,0xAD,0xBE}, {0xCA,0xFE,0x00},
            {0xFF,0x00,0x11}, {0x7F,0x80,0x90}, {0x42,0x66,0xAA}, {0x0B,0xA5,0xE5},
        };
        TrinaryState states[8] = {
            TRINARY_YES, TRINARY_YES, TRINARY_YES,
            TRINARY_MAYBE, TRINARY_MAYBE, TRINARY_MAYBE,
            TRINARY_NO, TRINARY_NO,
        };
        for (int i = 0; i < 8; i++)
            m_mosaic.encode(addrs[i], static_cast<uint8_t>(0xA0 + i), states[i]);
    }

    void init() {
        m_active = true;
        std::printf("\033[2J\033[H\033[?25l");   /* clear screen, hide cursor */
        std::fflush(stdout);
    }

    void step()  { m_spin.step(); }
    void reset() { m_spin.reset(); }

    void cleanup() {
        std::printf("\033[?25h\033[0m\n");        /* show cursor, reset color */
        std::fflush(stdout);
        m_active = false;
    }

    void render() const {
        std::printf("\033[2J\033[H");
        std::printf("\033[1m+====================================================+\n");
        std::printf("|  OBINexus MMUKO-OS | Phase 1 BIOS Firmware UI      |\n");
        std::printf("+====================================================+\033[0m\n");
        std::printf("\033[1;36m  %s\033[0m\n\n", m_dt.format_string().c_str());
        std::printf("SPIN: %s\n", m_spin.display().c_str());
        std::printf("Coherent: %s\n\n",
                    m_spin.is_coherent() ? "\033[1;32m[YES]\033[0m"
                                         : "\033[1;31m[NO]\033[0m");
        std::printf("MOSAIC:\n%s\n", m_mosaic.display().c_str());
        std::fflush(stdout);
    }

    void handle_input(char c) {
        switch (c) {
            case 's': case 'S': step();  break;
            case 'c': case 'C': reset(); break;
            default: break;
        }
    }

    bool is_active() const { return m_active; }
};

} /* namespace OBINexus::BIOS */

/* =========================================================================
 * C-linkage exports (static global instance, thread-local not needed here)
 * ========================================================================= */
extern "C" {
const char* bios_cpp_datetime_format(void);
void        bios_cpp_spin_step(void);
void        bios_cpp_mosaic_display(void);
}

#endif /* BIOS_INTERFACE_CPP_H */
