       /* Generated file. Do not edit by hand.
        * Authoritative input: MMUKO-OS.txt
        * Primary pseudocode: mmuko-boot/pseudocode/mmuko-boot.psc
        * Parsed functions from main boot pseudocode:
        *   - init_cubit_ring
*   - resolve_state
*   - lookup_superposition
*   - bit_shift_semantic
*   - rotate_bits
*   - mmuko_boot
*   - resolve_direction_from_neighbors
*   - get_middle_base
*   - flip_state
        * Parsed constants snapshot:
        *   - PI = 3.14159265358979
*   - SPIN_NORTH = PI / 4      // 0.7854  →  45°
*   - SPIN_NORTHEAST = PI / 3      // 1.0472  →  60°
*   - SPIN_EAST = PI / 2      // 1.5708  →  90°
*   - SPIN_SOUTHEAST = PI          // 3.1416  → 180°
*   - SPIN_SOUTH = PI * 2      // 6.2832  → 360° / full cycle
*   - SPIN_SOUTHWEST = PI / 2      // dual state with EAST (entangled)
*   - SPIN_WEST = PI / 3      // dual state with NORTHEAST
        */
       #include "mmuko_codegen.h"

       static const mmuko_phase_descriptor MMUKO_PHASES[] = {
           { "PHASE 0", "Vacuum Medium Initialization", "Establish the gravitational reference frame before touching mapped bytes." },
   { "PHASE 1", "Cubit Ring Initialization", "Project each byte into an 8-cubit compass ring with entangled partner indices." },
   { "PHASE 2", "Compass Alignment", "Resolve undefined directions from neighbours so no cubit remains locked." },
   { "PHASE 3", "Superposition Entanglement", "Break constructive interference across opposing compass pairs." },
   { "PHASE 4", "Middle Alignment", "Anchor the frame of reference at base 6 without a hard lock." },
   { "PHASE 5", "Nonlinear Index Resolution", "Traverse the diamond-table order [12, 6, 8, 4, 10, 2, 1]." },
   { "PHASE 6", "Rotation Verification", "Confirm every cubit can complete a full rotation without state loss." },
       };

       static const char *MMUKO_PSEUDOCODE_SOURCES[] = {
           "mmuko-boot/pseudocode/Bipartite Order and Chaos — Key Separation of Concern Crypto.psc :: supporting pseudocode context",
   "mmuko-boot/pseudocode/Electronic Magnetic RRR.psc :: supporting pseudocode context",
   "mmuko-boot/pseudocode/Electronic Magnetic State Machine Duailty Modelling.psc :: supporting pseudocode context",
   "mmuko-boot/pseudocode/Filter-Flash CISCO Interdepency _22March26.psc :: supporting pseudocode context",
   "mmuko-boot/pseudocode/Filter-Flash Eplison Matrix Conjugate.psc :: supporting pseudocode context",
   "mmuko-boot/pseudocode/HERE AND NOW COMMAND AND CONTROL.psc :: supporting pseudocode context",
   "mmuko-boot/pseudocode/LIBPolycall Financial Cobol Bridge.psc :: supporting pseudocode context",
   "mmuko-boot/pseudocode/MMUKO NSIGII How to Login into the MetaPhysical.psc :: supporting pseudocode context",
   "mmuko-boot/pseudocode/NSIGII - Loopback Addressing.psc :: supporting pseudocode context",
   "mmuko-boot/pseudocode/NSIGII LoopBack LR Polar On the Fly Proccesing LoopBack Addressing.psc :: supporting pseudocode context",
   "mmuko-boot/pseudocode/NSIGII Protocol - How I Fought For My Human Rights.psc :: supporting pseudocode context",
   "mmuko-boot/pseudocode/NSIGII — ENCODING SUFFERING INTO SILICON.psc :: supporting pseudocode context",
   "mmuko-boot/pseudocode/ON-THE-FLY COMMAND AND CONTROL VIA READ WRITE EXECUTE.psc :: supporting pseudocode context",
   "mmuko-boot/pseudocode/mmuko-boot.psc :: primary boot model"
       };

       size_t mmuko_stage2_phase_count(void) {
           return sizeof(MMUKO_PHASES) / sizeof(MMUKO_PHASES[0]);
       }

       const mmuko_phase_descriptor *mmuko_stage2_phases(void) {
           return MMUKO_PHASES;
       }

       const char *mmuko_stage2_boot_summary(void) {
           return "MMUKO-OS Canonical Specification ================================  Authoritative input ------------------- This file is the authoritative textual input for the repository's generated MMUKO-OS artifacts. The code generation pipeline under `tools/mmuko_codegen/` must treat `MMUKO-OS.txt` as the canonical specification layer, with `mmuko-boot/pseudocode/mmuko-boot.psc` as the primary executable pseudocode source and the remaining `.psc` files in `mmuko-boot/pseudocode/` as supporting context.";
       }

       size_t mmuko_pseudocode_source_count(void) {
           return sizeof(MMUKO_PSEUDOCODE_SOURCES) / sizeof(MMUKO_PSEUDOCODE_SOURCES[0]);
       }

       const char *mmuko_pseudocode_source(size_t index) {
           if (index >= mmuko_pseudocode_source_count()) {
               return 0;
           }
           return MMUKO_PSEUDOCODE_SOURCES[index];
       }
