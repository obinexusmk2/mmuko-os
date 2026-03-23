#ifndef MMUKO_RUNTIME_INTERFACE_H
#define MMUKO_RUNTIME_INTERFACE_H

#include <stdbool.h>
#include <stdint.h>

#include "mmuko_boot_spec.h"

typedef struct {
    const MMUKO_BOOT_HANDOFF *handoff;
    uint32_t expected_revision;
    uint8_t required_phase_count;
} MMUKO_RUNTIME_INTERFACE;

static inline bool mmuko_runtime_can_enter(const MMUKO_RUNTIME_INTERFACE *runtime) {
    return runtime != 0 &&
           runtime->handoff != 0 &&
           runtime->handoff->outcome == MMUKO_BOOT_OUTCOME_PASS &&
           runtime->handoff->completed_phases == runtime->required_phase_count &&
           runtime->handoff->revision == runtime->expected_revision;
}

static inline const char *mmuko_runtime_kernel_path(const MMUKO_RUNTIME_INTERFACE *runtime) {
    return (runtime != 0 && runtime->handoff != 0) ? runtime->handoff->kernel_path : "";
}

#endif
