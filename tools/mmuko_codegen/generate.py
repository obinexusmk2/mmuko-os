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


def _parse_functions(text: str) -> list[str]:
    return re.findall(r"^FUNC\s+([a-zA-Z0-9_]+)", text, re.MULTILINE)


def _parse_constants(text: str) -> list[tuple[str, str]]:
    results: list[tuple[str, str]] = []
    for name, value in re.findall(r"^CONST\s+([A-Z0-9_]+)\s*=\s*(.+)$", text, re.MULTILINE):
        results.append((name.strip(), value.strip()))
    return results


def _support_manifest(paths: list[Path], primary: Path) -> list[str]:
    manifest: list[str] = []
    for path in sorted(paths):
        role = "primary boot model" if path == primary else "supporting pseudocode context"
        manifest.append(f"{path.as_posix()} :: {role}")
    return manifest


def _write(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def generate(root: Path, spec_path: Path, primary: Path, pseudocode_dir: Path) -> None:
    psc_files = sorted(pseudocode_dir.glob("*.psc"))
    if primary not in psc_files:
        raise SystemExit(f"Primary pseudocode file not found in {pseudocode_dir}: {primary}")

    primary_text = primary.read_text(encoding="utf-8")
    spec_text = spec_path.read_text(encoding="utf-8")
    parsed_functions = _parse_functions(primary_text)
    parsed_constants = _parse_constants(primary_text)
    support_manifest = _support_manifest(psc_files, primary)

    source_list = ",\n".join(f'    "{entry}"' for entry in support_manifest)
    function_list = "\n".join(f" *   - {name}" for name in parsed_functions)
    constant_rows = "\n".join(f" *   - {name} = {value}" for name, value in parsed_constants[:8])
    phase_rows = "\n".join(
        f"    {{ \"{phase}\", \"{title}\", \"{summary}\" }}," for phase, title, summary in PHASES
    )
    spec_excerpt = " ".join(spec_text.splitlines()[:6]).replace('"', '\\"')

    asm = dedent(
        f"""\
        ; -----------------------------------------------------------------------------
        ; Generated file. Do not edit by hand.
        ; Authoritative input: {spec_path.as_posix()}
        ; Primary pseudocode: {primary.as_posix()}
        ; Supporting pseudocode count: {len(psc_files)}
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

            mov si, boot_banner
            call print_string
            mov si, boot_stage1
            call print_string
            mov si, boot_stage2
            call print_string
            mov si, boot_ready
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

        boot_banner db 13,10, "MMUKO-OS generated stage-1", 13,10, 0
        boot_stage1 db "Spec: MMUKO-OS.txt", 13,10, 0
        boot_stage2 db "Stage-2 handoff: kernel/mmuko_stage2_loader.c", 13,10, 0
        boot_ready  db "BOOTSTRAP_READY", 13,10, 0

        times 510-($-$$) db 0
        dw 0xAA55
        """
    )

    header = dedent(
        f"""\
        /* Generated file. Do not edit by hand.
         * Authoritative input: {spec_path.as_posix()}
         * Primary pseudocode: {primary.as_posix()}
         */
        #ifndef MMUKO_CODEGEN_H
        #define MMUKO_CODEGEN_H

        #include <stddef.h>

        #ifdef __cplusplus
        extern \"C\" {{
        #endif

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

        #ifdef __cplusplus
        }}
        #endif

        #endif /* MMUKO_CODEGEN_H */
        """
    )

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
        #include \"mmuko_codegen.h\"

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
            return \"{spec_excerpt}\";
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
        """
    )

    stage2_cpp = dedent(
        f"""\
        // Generated file. Do not edit by hand.
        // Authoritative input: {spec_path.as_posix()}
        // Primary pseudocode: {primary.as_posix()}
        #include \"mmuko_codegen.h\"

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
            report << \"Authoritative input: {spec_path.as_posix()}\\n\";
            report << \"Primary pseudocode: {primary.as_posix()}\\n\";
            report << \"Phase count: \" << mmuko_stage2_phase_count() << \"\\n\";

            const auto *phases = mmuko_stage2_phases();
            for (size_t index = 0; index < mmuko_stage2_phase_count(); ++index) {{
                report << phases[index].phase_id << \" => \" << phases[index].title
                       << \" :: \" << phases[index].summary << \"\\n\";
            }}
            return report.str();
        }}

        }} // namespace mmuko::generated
        """
    )

    pxd = dedent(
        """\
        cdef extern from \"mmuko_codegen.h\":
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
