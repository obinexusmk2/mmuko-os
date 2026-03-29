cdef extern from "mmuko_codegen.h":
    ctypedef struct mmuko_phase_descriptor:
        const char *phase_id
        const char *title
        const char *summary

    size_t mmuko_stage2_phase_count()
    const mmuko_phase_descriptor *mmuko_stage2_phases()
    const char *mmuko_stage2_boot_summary()
    size_t mmuko_pseudocode_source_count()
    const char *mmuko_pseudocode_source(size_t index)
