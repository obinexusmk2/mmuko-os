# Generated file. Do not edit by hand.
<<<<<<< HEAD
<<<<<<< HEAD
# Authoritative input: MMUKO-OS.txt
=======
# Authoritative input: pseudocode/MMUKO-OS.txt
>>>>>>> 87ae7ecdbcb6bce4c98cef97219e0ed08854e7c7
=======
# Authoritative input: pseudocode/MMUKO-OS.txt
>>>>>>> 87ae7ecdbcb6bce4c98cef97219e0ed08854e7c7
# distutils: language = c
from libc.string cimport strlen
cimport mmuko_codegen

def boot_summary():
    cdef const char *value = mmuko_codegen.mmuko_stage2_boot_summary()
    return value[:strlen(value)].decode("utf-8")

def phases():
    cdef size_t total = mmuko_codegen.mmuko_stage2_phase_count()
    cdef const mmuko_codegen.mmuko_phase_descriptor *items = mmuko_codegen.mmuko_stage2_phases()
    return [
        {
            "phase": items[index].phase_id[:strlen(items[index].phase_id)].decode("utf-8"),
            "title": items[index].title[:strlen(items[index].title)].decode("utf-8"),
            "summary": items[index].summary[:strlen(items[index].summary)].decode("utf-8"),
        }
        for index in range(total)
    ]

def pseudocode_sources():
    cdef size_t total = mmuko_codegen.mmuko_pseudocode_source_count()
    return [
        mmuko_codegen.mmuko_pseudocode_source(index)[:strlen(mmuko_codegen.mmuko_pseudocode_source(index))].decode("utf-8")
        for index in range(total)
    ]
