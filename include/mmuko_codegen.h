/* Generated file. Do not edit by hand.
 * Authoritative input: MMUKO-OS.txt
 * Primary pseudocode: mmuko-boot/pseudocode/mmuko-boot.psc
 */
#ifndef MMUKO_CODEGEN_H
#define MMUKO_CODEGEN_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mmuko_phase_descriptor {
    const char *phase_id;
    const char *title;
    const char *summary;
} mmuko_phase_descriptor;

size_t mmuko_stage2_phase_count(void);
const mmuko_phase_descriptor *mmuko_stage2_phases(void);
const char *mmuko_stage2_boot_summary(void);
size_t mmuko_pseudocode_source_count(void);
const char *mmuko_pseudocode_source(size_t index);

#ifdef __cplusplus
}
#endif

#endif /* MMUKO_CODEGEN_H */
