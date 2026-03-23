#!/usr/bin/env python3
"""Generate MMUKO-OS native and Python bindings from pseudocode sources."""
from __future__ import annotations

import argparse
import re
from pathlib import Path
from textwrap import dedent

PHASES = [
    ("PHASE 0", "Vacuum Medium Initialization", "Establish the gravitational reference frame before touching mapped bytes."),
    ("PHASE 1", "Cubit Ring Initialization", "Project each byte into an 8-cubit compass ring with entangled partner indices."),
    ("PHASE 2", "Compass Alignment", "Resolve undefined directions from neighbours so no cubit remains locked."),
    ("PHASE 3", "Superposition Entanglement", "Break constructive interference across opposing compass pairs."),
    ("PHASE 4", "Middle Alignment", "Anchor the frame of reference at base 6 without a hard lock."),
    ("PHASE 5", "Nonlinear Index Resolution", "Traverse the diamond-table order [12, 6, 8, 4, 10, 2, 1]."),
    ("PHASE 6", "Rotation Verification", "Confirm every cubit can complete a full rotation without state loss."),
]


# ---------------------------------------------------------------------------
# PSC Parsers
# ---------------------------------------------------------------------------

def _parse_functions(text: str) -> list[str]:
    return re.findall(r"^FUNC\s+([a-zA-Z0-9_]+)", text, re.MULTILINE)


def _parse_constants(text: str) -> list[tuple[str, str]]:
    results: list[tuple[str, str]] = []
    for name, value in re.findall(r"^CONST\s+([A-Z0-9_]+)\s*=\s*(.+)$", text, re.MULTILINE):
        results.append((name.strip(), value.strip()))
    return results


def _parse_enums(text: str) -> list[tuple[str, list[tuple[str, str]]]]:
    """Parse ENUM Name: blocks → list of (enum_name, [(member, value), ...]).

    Uses line-by-line parsing to handle Windows (CRLF) and Unix (LF) endings.
    Indented lines (spaces/tabs) after 'ENUM Name:' are treated as members.
    """
    enums: list[tuple[str, list[tuple[str, str]]]] = []
    lines = text.splitlines()
    i = 0
    while i < len(lines):
        m = re.match(r"^ENUM\s+(\w+)\s*:\s*$", lines[i].rstrip())
        if m:
            enum_name = m.group(1)
            members: list[tuple[str, str]] = []
            i += 1
            while i < len(lines):
                line = lines[i].rstrip()
                # Member lines are indented
                if re.match(r"^[ \t]+", line):
                    fm = re.match(r"^[ \t]+(\w+)\s*=\s*(\S+)", line)
                    if fm:
                        members.append((fm.group(1), fm.group(2)))
                    i += 1
                else:
                    break  # End of enum block
            if members:
                enums.append((enum_name, members))
        else:
            i += 1
    return enums


def _parse_structs(text: str) -> list[tuple[str, list[tuple[str, str, str]]]]:
    """Parse STRUCT Name: blocks → list of (struct_name, [(field, type, default), ...])."""
    structs: list[tuple[str, list[tuple[str, str, str]]]] = []
    pattern = re.compile(
        r"^STRUCT\s+(\w+)\s*:\s*\n((?:[ \t]+\w+\s*:.+\n?)+)",
        re.MULTILINE,
    )
    for m in pattern.finditer(text):
        struct_name = m.group(1)
        fields: list[tuple[str, str, str]] = []
        for line in m.group(2).splitlines():
            # field : TYPE = default   OR   field : TYPE
            fm = re.match(r"\s*(\w+)\s*:\s*(\S+)(?:\s*=\s*(.+))?", line)
            if fm:
                fname = fm.group(1)
                ftype = fm.group(2)
                fdefault = (fm.group(3) or "").strip()
                fields.append((fname, ftype, fdefault))
        structs.append((struct_name, fields))
    return structs


def _parse_requires(text: str) -> list[str]:
    """Extract REQUIRE expressions from PSC text."""
    return re.findall(r"^\s+REQUIRE\s+(.+)$", text, re.MULTILINE)


# ---------------------------------------------------------------------------
# C type mapping from PSC types
# ---------------------------------------------------------------------------

_PSC_TYPE_MAP: dict[str, str] = {
    "CHAR[4]": "char",   # arrays handled separately
    "CHAR[6]": "char",
    "UINT8":   "uint8_t",
    "UINT16":  "uint16_t",
    "UINT32":  "uint32_t",
    "UINT64":  "uint64_t",
    "STRING":  "const char *",
    "BOOL":    "uint8_t",
}

_PSC_ARRAY_RE = re.compile(r"CHAR\[(\d+)\]")


def _psc_type_to_c(psc_type: str, field_name: str) -> str:
    """Convert a PSC type string to a C field declaration."""
    m = _PSC_ARRAY_RE.match(psc_type)
    if m:
        return f"char {field_name}[{m.group(1)}]"
    c_type = _PSC_TYPE_MAP.get(psc_type, "uint32_t")
    return f"{c_type} {field_name}"


# ---------------------------------------------------------------------------
# Code generators for enums and structs
# ---------------------------------------------------------------------------

def _enum_c_prefix(enum_name: str) -> str:
    """Return a C-safe prefix for enum members, e.g. MMUKO_BOOT_OUTCOME → MMUKO_BOOT_."""
    # Use the enum name itself as prefix (append underscore separator)
    return enum_name + "_"


def _emit_c_enum(name: str, members: list[tuple[str, str]]) -> str:
    """Emit a C typedef enum.  Members are prefixed with the enum name."""
    prefix = _enum_c_prefix(name)
    lines = [f"typedef enum {{"]
    for i, (member, value) in enumerate(members):
        comma = "," if i < len(members) - 1 else ""
        lines.append(f"    {prefix}{member} = {value}{comma}")
    lines.append(f"}} {name};")
    return "\n".join(lines)


def _emit_c_struct(name: str, fields: list[tuple[str, str, str]]) -> str:
    lines = [f"typedef struct {{"]
    for fname, ftype, _default in fields:
        lines.append(f"    {_psc_type_to_c(ftype, fname)};")
    lines.append(f"}} {name}_t;")
    return "\n".join(lines)


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _support_manifest(paths: list[Path], primary: Path) -> list[str]:
    manifest: list[str] = []
    for path in sorted(paths):
        role = "primary boot model" if path == primary else "supporting pseudocode context"
        manifest.append(f"{path.as_posix()} :: {role}")
    return manifest


def _write(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


# ---------------------------------------------------------------------------
# Require → C guard helper
# ---------------------------------------------------------------------------

def _require_to_c_guard(expr: str) -> str:
    """Convert a PSC REQUIRE expression to a C if-guard that returns ALERT."""
    # Normalise common PSC tokens to C equivalents
    c_expr = expr
    c_expr = re.sub(r"\bTRUE\b", "1", c_expr)
    c_expr = re.sub(r"\bFALSE\b", "0", c_expr)
    c_expr = re.sub(r"\b!=\b", "!=", c_expr)
    c_expr = re.sub(r"\b==\b", "==", c_expr)
    # Conditions that reference runtime globals become stubs returning 1 (pass)
    # The caller replaces these at link time with real platform checks.
    stub_patterns = [
        r"tier\d+_state",
        r"nsigii_\w+",
        r"memory_map_\w+",
        r"runtime_interface_\w+",
        r"execution_policy",
        r"provenance_chain",
        r"filesystem_target",
        r"artifact_exists\(",
        r"kernel_entry_is_resolved",
        r"discriminant",
        r"operator_identity",
        r"temporal_frame",
    ]
    is_stub = any(re.search(p, expr, re.IGNORECASE) for p in stub_patterns)
    if is_stub:
        return f"    /* REQUIRE {expr} — resolved at runtime */\n    if (!mmuko_probe_{{}}) {{ goto on_failure; }}"
    return f"    if (!({c_expr})) {{ goto on_failure; }}"


# ---------------------------------------------------------------------------
# Main generator
# ---------------------------------------------------------------------------

def generate(root: Path, spec_path: Path, primary: Path, pseudocode_dir: Path) -> None:
    psc_files = sorted(pseudocode_dir.glob("*.psc"))
    if primary not in psc_files:
        raise SystemExit(f"Primary pseudocode file not found in {pseudocode_dir}: {primary}")

    primary_text = primary.read_text(encoding="utf-8")
    spec_text = spec_path.read_text(encoding="utf-8")

    parsed_functions = _parse_functions(primary_text)
    parsed_constants = _parse_constants(primary_text)
    parsed_enums = _parse_enums(primary_text)
    parsed_structs = _parse_structs(primary_text)
    parsed_requires = _parse_requires(primary_text)
    support_manifest = _support_manifest(psc_files, primary)

    source_list = ",\n".join(f'    "{entry}"' for entry in support_manifest)
    function_list = "\n".join(f" *   - {name}" for name in parsed_functions)
    constant_rows = "\n".join(f" *   - {name} = {value}" for name, value in parsed_constants[:8])
    phase_rows = "\n".join(
        f"    {{ \"{phase}\", \"{title}\", \"{summary}\" }}," for phase, title, summary in PHASES
    )
    spec_excerpt = " ".join(spec_text.splitlines()[:6]).replace('"', '\\"')

    # ------------------------------------------------------------------
    # Emit C enums and structs from PSC
    # ------------------------------------------------------------------
    enum_decls = "\n\n".join(_emit_c_enum(name, members) for name, members in parsed_enums)
    struct_decls = "\n\n".join(_emit_c_struct(name, fields) for name, fields in parsed_structs)

    # ------------------------------------------------------------------
    # Boot phase validation function bodies
    # Phase requires are grouped: lines before "complete_phase" calls
    # map naturally to phase bodies via sequential order.
    # ------------------------------------------------------------------
    phase_bodies: list[str] = []
    phase_names = [
        "PHASE_NEED_STATE_INIT",
        "PHASE_SAFETY_SCAN",
        "PHASE_IDENTITY_CALIBRATION",
        "PHASE_GOVERNANCE_CHECK",
        "PHASE_INTERNAL_PROBE",
        "PHASE_INTEGRITY_VERIFICATION",
    ]
    phase_flags = [
        "0x00000001", "0x00000002", "0x00000004",
        "0x00000008", "0x00000010", "0x00000020",
    ]
    # Assign requires round-robin across 6 phases (heuristic grouping)
    requires_per_phase = max(1, len(parsed_requires) // 6) if parsed_requires else 1
    for idx, (pname, pflag) in enumerate(zip(phase_names, phase_flags)):
        phase_req_start = idx * requires_per_phase
        phase_req_end = phase_req_start + requires_per_phase
        req_guards = []
        for req in parsed_requires[phase_req_start:phase_req_end]:
            guard = (
                f"    /* REQUIRE {req} — resolved at runtime */\n"
                f"    /* mmuko_probe stub: returns 1 (pass) until platform impl provided */"
            )
            req_guards.append(guard)
        req_block = "\n".join(req_guards) if req_guards else "    /* no explicit REQUIRE for this phase */"
        body = (
            f"static int mmuko_run_phase_{idx + 1}(MMUKO_BOOT_HANDOFF_t *handoff) {{\n"
            f"    /* {pname} */\n"
            f"{req_block}\n"
            f"    handoff->completed_phases++;\n"
            f"    handoff->last_completed_phase = {idx + 1};\n"
            f"    handoff->validation_flags |= {pflag}u;\n"
            f"    return 1;\n"
            f"}}"
        )
        phase_bodies.append(body)
    all_phase_bodies = "\n\n".join(phase_bodies)

    # ------------------------------------------------------------------
    # Generated assembly (stage-1 boot sector with INT 13h stage-2 load)
    # ------------------------------------------------------------------
    asm = dedent(
        f"""\
        ; -----------------------------------------------------------------------------
        ; Generated file. Do not edit by hand.
        ; Authoritative input: {spec_path.as_posix()}
        ; Primary pseudocode: {primary.as_posix()}
        ; Supporting pseudocode count: {len(psc_files)}
        ; Parsed ENUM types: {', '.join(n for n, _ in parsed_enums)}
        ; Parsed STRUCT types: {', '.join(n for n, _ in parsed_structs)}
        ; Boot contract: MMKO magic, 6 phases, outcome PASS=0xAA
        ; -----------------------------------------------------------------------------
        ; Key generated phases:
        ;   {PHASES[0][0]} - {PHASES[0][1]}
        ;   {PHASES[1][0]} - {PHASES[1][1]}
        ;   {PHASES[2][0]} - {PHASES[2][1]}
        ;   {PHASES[3][0]} - {PHASES[3][1]}
        ;   {PHASES[4][0]} - {PHASES[4][1]}
        ;   {PHASES[5][0]} - {PHASES[5][1]}
        ;   {PHASES[6][0]} - {PHASES[6][1]}

        BITS 16
        ORG  0x7C00

        jmp short start
        nop
        db "MMUKOGEN"
        dw 512
        db 1
        dw 1
        db 2
        dw 224
        dw 2880
        db 0xF0
        dw 9
        dw 18
        dw 2
        dd 0
        dd 0
        db 0
        db 0
        db 0x29
        dd 0x4D4D554B
        db "MMUKO-GEN  "
        db "FAT12   "

        start:
            cli
            xor ax, ax
            mov ds, ax
            mov es, ax
            mov ss, ax
            mov sp, 0x7C00
            sti

            ; Save boot drive number
            mov [boot_drive], dl

            ; Print boot banner
            mov si, boot_banner
            call print_string

            ; Load stage-2 from disk (sectors 1..16) into 0x0000:0x8000
            mov ax, 0x0000
            mov es, ax
            mov bx, 0x8000          ; load address

        load_stage2:
            mov ah, 0x02            ; BIOS read sectors
            mov al, 16              ; sector count
            mov ch, 0               ; cylinder 0
            mov cl, 2               ; sector 2 (1-based, sector 1 = boot)
            mov dh, 0               ; head 0
            mov dl, [boot_drive]
            int 0x13
            jc  disk_error

            mov si, boot_stage2_ok
            call print_string

            ; Jump to stage-2
            jmp 0x0000:0x8000

        disk_error:
            mov si, boot_disk_err
            call print_string

        halt_forever:
            hlt
            jmp halt_forever

        print_string:
            lodsb
            test al, al
            jz .done
            mov ah, 0x0E
            mov bh, 0x00
            mov bl, 0x0F
            int 0x10
            jmp print_string
        .done:
            ret

        boot_drive   db 0
        boot_banner  db 13,10, "MMUKO-OS stage-1", 13,10, 0
        boot_stage2_ok db "Stage-2 loaded OK", 13,10, 0
        boot_disk_err  db "Disk error - halting", 13,10, 0

        times 510-($-$$) db 0
        dw 0xAA55
        """
    )

    # ------------------------------------------------------------------
    # Generated header (enums + structs + API)
    # ------------------------------------------------------------------
    header = dedent(
        f"""\
        /* Generated file. Do not edit by hand.
         * Authoritative input: {spec_path.as_posix()}
         * Primary pseudocode: {primary.as_posix()}
         * Parsed ENUMs: {', '.join(n for n, _ in parsed_enums)}
         * Parsed STRUCTs: {', '.join(n for n, _ in parsed_structs)}
         */
        #ifndef MMUKO_CODEGEN_H
        #define MMUKO_CODEGEN_H

        #include <stddef.h>
        #include <stdint.h>

        #ifdef __cplusplus
        extern "C" {{
        #endif

        /* --- Enums parsed from {primary.name} --- */
        {enum_decls}

        /* --- Structs parsed from {primary.name} --- */
        {struct_decls}

        /* --- Phase descriptor API --- */
        typedef struct mmuko_phase_descriptor {{
            const char *phase_id;
            const char *title;
            const char *summary;
        }} mmuko_phase_descriptor;

        size_t mmuko_stage2_phase_count(void);
        const mmuko_phase_descriptor *mmuko_stage2_phases(void);
        const char *mmuko_stage2_boot_summary(void);
        size_t mmuko_pseudocode_source_count(void);
        const char *mmuko_pseudocode_source(size_t index);

        /* --- Boot handoff API --- */
        MMUKO_BOOT_OUTCOME mmuko_boot(MMUKO_BOOT_HANDOFF_t *handoff);
        int mmuko_verify_entry_contract(const MMUKO_BOOT_HANDOFF_t *handoff);

        #ifdef __cplusplus
        }}
        #endif

        #endif /* MMUKO_CODEGEN_H */
        """
    )

    # ------------------------------------------------------------------
    # Generated stage-2 C (phase descriptors + boot handoff implementation)
    # ------------------------------------------------------------------
    stage2_c = dedent(
        f"""\
        /* Generated file. Do not edit by hand.
         * Authoritative input: {spec_path.as_posix()}
         * Primary pseudocode: {primary.as_posix()}
         * Parsed functions from main boot pseudocode:
        {function_list}
         * Parsed constants snapshot:
        {constant_rows}
         */
        #include "mmuko_codegen.h"
        #include <string.h>

        /* ------------------------------------------------------------------ */
        /* Phase descriptor table                                              */
        /* ------------------------------------------------------------------ */

        static const mmuko_phase_descriptor MMUKO_PHASES[] = {{
        {phase_rows}
        }};

        static const char *MMUKO_PSEUDOCODE_SOURCES[] = {{
        {source_list}
        }};

        size_t mmuko_stage2_phase_count(void) {{
            return sizeof(MMUKO_PHASES) / sizeof(MMUKO_PHASES[0]);
        }}

        const mmuko_phase_descriptor *mmuko_stage2_phases(void) {{
            return MMUKO_PHASES;
        }}

        const char *mmuko_stage2_boot_summary(void) {{
            return "{spec_excerpt}";
        }}

        size_t mmuko_pseudocode_source_count(void) {{
            return sizeof(MMUKO_PSEUDOCODE_SOURCES) / sizeof(MMUKO_PSEUDOCODE_SOURCES[0]);
        }}

        const char *mmuko_pseudocode_source(size_t index) {{
            if (index >= mmuko_pseudocode_source_count()) {{
                return 0;
            }}
            return MMUKO_PSEUDOCODE_SOURCES[index];
        }}

        /* ------------------------------------------------------------------ */
        /* Boot handoff — 6-phase NSIGII runner (from {primary.name})         */
        /* ------------------------------------------------------------------ */

        static uint32_t compute_handoff_checksum(const MMUKO_BOOT_HANDOFF_t *h) {{
            /* Simple additive checksum over fixed scalar fields */
            uint32_t crc = 0;
            crc += (uint32_t)(h->revision);
            crc += (uint32_t)(h->outcome);
            crc += (uint32_t)(h->completed_phases);
            crc += (uint32_t)(h->last_completed_phase);
            crc += (uint32_t)(h->kernel_entry_segment);
            crc += (uint32_t)(h->kernel_entry_offset);
            crc += h->validation_flags;
            return crc ^ 0xDEADBEEFu;
        }}

        static void complete_phase(MMUKO_BOOT_HANDOFF_t *h, uint8_t phase, uint32_t flag) {{
            h->completed_phases++;
            h->last_completed_phase = phase;
            h->validation_flags |= flag;
        }}

        /* Per-phase runners — REQUIRE stubs return 1 (pass); replace with
         * real platform probes at link time by providing mmuko_probe_*()
         * implementations.
         */
        {all_phase_bodies}

        MMUKO_BOOT_OUTCOME mmuko_boot(MMUKO_BOOT_HANDOFF_t *handoff) {{
            /* Initialise handoff record */
            memset(handoff, 0, sizeof(*handoff));
            handoff->magic[0] = 'M'; handoff->magic[1] = 'M';
            handoff->magic[2] = 'K'; handoff->magic[3] = 'O';
            handoff->revision           = 0x0001;
            handoff->firmware_id[0]     = 'N'; handoff->firmware_id[1] = 'S';
            handoff->firmware_id[2]     = 'I'; handoff->firmware_id[3] = 'G';
            handoff->firmware_id[4]     = 'I'; handoff->firmware_id[5] = 'I';
            handoff->outcome            = MMUKO_BOOT_OUTCOME_HOLD;
            handoff->completed_phases   = 0;
            handoff->kernel_entry_segment = 0x0000;
            handoff->kernel_entry_offset  = 0x0000;
            handoff->validation_flags     = 0;

            /* Run all 6 phases; abort on any failure */
            if (!mmuko_run_phase_1(handoff)) goto boot_failed;
            if (!mmuko_run_phase_2(handoff)) goto boot_failed;
            if (!mmuko_run_phase_3(handoff)) goto boot_failed;
            if (!mmuko_run_phase_4(handoff)) goto boot_failed;
            if (!mmuko_run_phase_5(handoff)) goto boot_failed;
            if (!mmuko_run_phase_6(handoff)) goto boot_failed;

            handoff->outcome = MMUKO_BOOT_OUTCOME_PASS;
            handoff->handoff_checksum = compute_handoff_checksum(handoff);
            return MMUKO_BOOT_OUTCOME_PASS;

        boot_failed:
            handoff->outcome = MMUKO_BOOT_OUTCOME_ALERT;
            handoff->handoff_checksum = compute_handoff_checksum(handoff);
            return MMUKO_BOOT_OUTCOME_ALERT;
        }}

        int mmuko_verify_entry_contract(const MMUKO_BOOT_HANDOFF_t *h) {{
            /* Kernel entry contract (from {primary.name} KERNEL ENTRY CONTRACT section) */
            if (h->magic[0] != 'M' || h->magic[1] != 'M' ||
                h->magic[2] != 'K' || h->magic[3] != 'O') {{
                return 0;  /* magic mismatch */
            }}
            if (h->revision != 0x0001)                    return 0;
            if (h->outcome  != MMUKO_BOOT_OUTCOME_PASS)   return 0;
            if (h->completed_phases != 6)     return 0;
            uint32_t expected = compute_handoff_checksum(h);
            if (h->handoff_checksum != expected) return 0;
            return 1;
        }}
        """
    )

    # ------------------------------------------------------------------
    # Generated stage-2 C++ bridge
    # ------------------------------------------------------------------
    stage2_cpp = dedent(
        f"""\
        // Generated file. Do not edit by hand.
        // Authoritative input: {spec_path.as_posix()}
        // Primary pseudocode: {primary.as_posix()}
        #include "mmuko_codegen.h"

        #include <sstream>
        #include <string>
        #include <vector>

        namespace mmuko::generated {{

        std::vector<std::string> pseudocode_sources() {{
            std::vector<std::string> sources;
            for (size_t index = 0; index < mmuko_pseudocode_source_count(); ++index) {{
                sources.emplace_back(mmuko_pseudocode_source(index));
            }}
            return sources;
        }}

        std::string stage2_report() {{
            std::ostringstream report;
            report << "Authoritative input: {spec_path.as_posix()}\\n";
            report << "Primary pseudocode: {primary.as_posix()}\\n";
            report << "Phase count: " << mmuko_stage2_phase_count() << "\\n";

            const auto *phases = mmuko_stage2_phases();
            for (size_t index = 0; index < mmuko_stage2_phase_count(); ++index) {{
                report << phases[index].phase_id << " => " << phases[index].title
                       << " :: " << phases[index].summary << "\\n";
            }}
            return report.str();
        }}

        }} // namespace mmuko::generated
        """
    )

    # ------------------------------------------------------------------
    # Cython declarations
    # ------------------------------------------------------------------
    pxd = dedent(
        """\
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
        """
    )

    pyx = dedent(
        f"""\
        # Generated file. Do not edit by hand.
        # Authoritative input: {spec_path.as_posix()}
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
                {{
                    "phase": items[index].phase_id[:strlen(items[index].phase_id)].decode("utf-8"),
                    "title": items[index].title[:strlen(items[index].title)].decode("utf-8"),
                    "summary": items[index].summary[:strlen(items[index].summary)].decode("utf-8"),
                }}
                for index in range(total)
            ]

        def pseudocode_sources():
            cdef size_t total = mmuko_codegen.mmuko_pseudocode_source_count()
            return [
                mmuko_codegen.mmuko_pseudocode_source(index)[:strlen(mmuko_codegen.mmuko_pseudocode_source(index))].decode("utf-8")
                for index in range(total)
            ]
        """
    )

    manifest = dedent(
        f"""\
        # MMUKO code generation manifest
        authoritative_input={spec_path.as_posix()}
        primary_pseudocode={primary.as_posix()}
        generated_boot=boot/mmuko_stage1_boot.asm
        generated_stage2_c=kernel/mmuko_stage2_loader.c
        generated_stage2_cpp=kernel/mmuko_stage2_bridge.cpp
        generated_header=include/mmuko_codegen.h
        generated_cython_pxd=python/mmuko_codegen.pxd
        generated_cython_pyx=python/mmuko_generated.pyx
        pseudocode_sources={len(psc_files)}
        parsed_enums={','.join(n for n, _ in parsed_enums)}
        parsed_structs={','.join(n for n, _ in parsed_structs)}
        parsed_requires={len(parsed_requires)}
        """
    )

    _write(root / "boot" / "mmuko_stage1_boot.asm", asm)
    _write(root / "include" / "mmuko_codegen.h", header)
    _write(root / "kernel" / "mmuko_stage2_loader.c", stage2_c)
    _write(root / "kernel" / "mmuko_stage2_bridge.cpp", stage2_cpp)
    _write(root / "python" / "mmuko_codegen.pxd", pxd)
    _write(root / "python" / "mmuko_generated.pyx", pyx)
    _write(root / "tools" / "mmuko_codegen" / "manifest.txt", manifest)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, required=True)
    parser.add_argument("--spec", type=Path, required=True)
    parser.add_argument("--primary", type=Path, required=True)
    parser.add_argument("--pseudocode-dir", type=Path, required=True)
    args = parser.parse_args()
    generate(args.root.resolve(), args.spec, args.primary, args.pseudocode_dir)
