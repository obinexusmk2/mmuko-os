# cython: language_level=3
# python/mmuko_os/bios_firmware.pxd
# MMUKO-OS BIOS Firmware Interface — Cython declarations
# OBINexus Computing | Nnamdi Michael Okpala | 29 March 2026
#
# Import TrinaryState from the existing firmware .pxd to avoid
# redeclaring the same C enum and causing type-conflict errors.

from libc.stdint cimport uint8_t, uint32_t, int8_t, int32_t
from libc.stddef cimport size_t

# Reuse TrinaryState from the sibling .pxd (same package, same C header chain)
from .firmware cimport TrinaryState

cdef extern from "firmware/bios_interface.h":

    # ------------------------------------------------------------------
    # BIOSDateTime — mirrors BIOS INT 0x1A register layout
    # ------------------------------------------------------------------
    ctypedef struct BIOSDateTime:
        uint8_t century
        uint8_t year
        uint8_t month
        uint8_t day
        uint8_t hour
        uint8_t min
        uint8_t sec

    # ------------------------------------------------------------------
    # GPSLockState — from transcript: half-locked / locked / not-locked
    # ------------------------------------------------------------------
    ctypedef enum GPSLockState:
        GPS_HALF_LOCKED
        GPS_LOCKED
        GPS_NOT_LOCKED

    # ------------------------------------------------------------------
    # SpinOperator — single magnetic dipole
    # ------------------------------------------------------------------
    ctypedef struct SpinOperator:
        TrinaryState alpha
        TrinaryState beta
        TrinaryState gamma
        int32_t      orientation_x100
        int32_t      elevation
        int32_t      position_x
        int8_t       polarity

    # ------------------------------------------------------------------
    # SpinPair — alpha(north) + bravo(south) coupled magnet system
    # ------------------------------------------------------------------
    ctypedef struct SpinPair:
        SpinOperator alpha_magnet
        SpinOperator bravo_magnet
        uint32_t     step_count
        double       work_done

    # ------------------------------------------------------------------
    # MosaicEntry — color-coded hexadecimal memory cell
    # ------------------------------------------------------------------
    ctypedef struct MosaicEntry:
        uint8_t      address[3]
        uint8_t      color_code
        uint8_t      data
        TrinaryState spin_state

    # ------------------------------------------------------------------
    # Function declarations
    # ------------------------------------------------------------------
    BIOSDateTime bios_datetime_now()                                  nogil
    void         bios_datetime_format(const BIOSDateTime *dt,
                                      char *buf, int bufsz)           nogil
    SpinPair     spin_pair_init()                                      nogil
    void         spin_pair_step(SpinPair *pair)                       nogil
    bint         spin_valid_state(const SpinPair *pair)               nogil
    MosaicEntry  mosaic_encode(const uint8_t *addr,
                               uint8_t data,
                               TrinaryState state)                    nogil
    const char*  mosaic_color_for(uint8_t color_code)                 nogil
