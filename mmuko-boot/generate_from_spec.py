#!/usr/bin/env python3
"""Generate MMUKO boot skeletons and native headers from MMUKO-OS.txt."""

from __future__ import annotations

import json
import pathlib
import re
import sys
from typing import Dict, List

ROOT = pathlib.Path(__file__).resolve().parents[1]
SPEC_PATH = ROOT / "MMUKO-OS.txt"
OUTPUT_DIR = ROOT / "mmuko-boot" / "generated"
INCLUDE_DIR = ROOT / "mmuko-boot" / "include"

SECTION_RE = re.compile(r"^\[(.+)\]$")
PHASE_RE = re.compile(r"^(\d+)\s*\|\s*([^|]+?)\s*\|\s*([^|]+?)\s*\|\s*([^|]+?)\s*\|\s*(.+)$")
KEYVAL_RE = re.compile(r"^([^=]+?)\s*=\s*(.+)$")

REQUIRED_SECTIONS = [
    "Boot Phases",
    "Filesystem Target",
    "Kernel Handoff Contract",
    "NSIGII Firmware Requirements",
    "Artifact Names",
]


def parse_spec(text: str) -> Dict[str, List[str]]:
    sections: Dict[str, List[str]] = {}
    current = None
    for raw_line in text.splitlines():
        line = raw_line.strip()
        if not line:
            continue
        match = SECTION_RE.match(line)
        if match:
            current = match.group(1)
            sections[current] = []
            continue
        if current:
            sections[current].append(line)
    missing = [name for name in REQUIRED_SECTIONS if name not in sections]
    if missing:
        raise ValueError(f"Missing required sections: {', '.join(missing)}")
    return sections


def parse_phase_rows(lines: List[str]) -> List[Dict[str, str]]:
    phases = []
    for line in lines:
        match = PHASE_RE.match(line)
        if not match:
            raise ValueError(f"Invalid phase row: {line}")
        order, phase_id, letter, title, description = match.groups()
        phases.append(
            {
                "order": int(order),
                "phase_id": phase_id.strip(),
                "letter": letter.strip(),
                "title": title.strip(),
                "description": description.strip(),
            }
        )
    if len(phases) != 6:
        raise ValueError(f"Expected 6 boot phases, found {len(phases)}")
    return phases


def parse_keyvals(lines: List[str]) -> Dict[str, str]:
    values: Dict[str, str] = {}
    for line in lines:
        match = KEYVAL_RE.match(line)
        if not match:
            raise ValueError(f"Invalid key/value row: {line}")
        key, value = match.groups()
        values[key.strip()] = value.strip()
    return values


def c_string(value: str) -> str:
    return value.replace('\\', '\\\\').replace('"', '\\"')


def generate_boot_asm(phases: List[Dict[str, str]], fs: Dict[str, str], contract: Dict[str, str]) -> str:
    lines = [
        "; AUTO-GENERATED from MMUKO-OS.txt — do not edit by hand",
        "; Skeleton only: map canonical phases and handoff fields into the boot sector implementation",
        f"; Filesystem target: {fs['layout']} image {fs['image_name']}",
        f"; Handoff structure: {contract['struct_name']} magic={contract['magic']} revision={contract['revision']}",
        "",
        "BITS 16",
        "ORG 0x7C00",
        "",
        "; Canonical phase order",
    ]
    for phase in phases:
        lines.append(f";   {phase['order']}. {phase['phase_id']} ({phase['letter']}) — {phase['title']}")
    lines += [
        "",
        "; Required handoff fields to populate before kernel transfer:",
        f";   {contract['completed_phase_field']}",
        f";   {contract['last_phase_field']}",
        f";   {contract['filesystem_field']}",
        f";   {contract['kernel_field']}",
        f";   {contract['manifest_field']}",
        f";   {contract['config_field']}",
        f";   {contract['entry_segment_field']}",
        f";   {contract['entry_offset_field']}",
        f";   {contract['flags_field']}",
        f";   {contract['checksum_field']}",
        "",
        "start:",
        "    ; TODO: implement canonical phase execution and populate MMUKO_BOOT_HANDOFF",
        "    cli",
        "    hlt",
    ]
    return "\n".join(lines) + "\n"


def generate_boot_c(phases: List[Dict[str, str]], fs: Dict[str, str], contract: Dict[str, str], artifacts: Dict[str, str]) -> str:
    phase_cases = "\n".join(
        f"        case {phase['phase_id']}:\n"
        f"            /* {phase['title']} */\n"
        f"            return true;"
        for phase in phases
    )
    return f'''/* AUTO-GENERATED from MMUKO-OS.txt — do not edit by hand */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "../include/mmuko_boot_spec.h"
#include "../include/mmuko_runtime_interface.h"

static void mmuko_boot_handoff_defaults(MMUKO_BOOT_HANDOFF *handoff) {{
    memset(handoff, 0, sizeof(*handoff));
    memcpy(handoff->magic, "{c_string(contract['magic'])}", 4);
    handoff->revision = {contract['revision']};
    handoff->outcome = MMUKO_BOOT_OUTCOME_HOLD;
    handoff->phase_count = {contract['phase_count']};
    strncpy(handoff->filesystem_target, "{c_string(fs['layout'])}:{c_string(fs['image_name'])}", sizeof(handoff->filesystem_target) - 1U);
    strncpy(handoff->kernel_path, "{c_string(fs['root_artifact'])}", sizeof(handoff->kernel_path) - 1U);
    strncpy(handoff->artifact_manifest_path, "{c_string(fs['manifest_artifact'])}", sizeof(handoff->artifact_manifest_path) - 1U);
    strncpy(handoff->config_path, "{c_string(fs['config_artifact'])}", sizeof(handoff->config_path) - 1U);
}}

uint32_t mmuko_boot_checksum(const MMUKO_BOOT_HANDOFF *handoff) {{
    const unsigned char *bytes = (const unsigned char *)handoff;
    uint32_t checksum = 2166136261u;

    for (size_t i = 0; i < sizeof(*handoff) - sizeof(handoff->handoff_checksum); ++i) {{
        checksum ^= bytes[i];
        checksum *= 16777619u;
    }}
    return checksum;
}}

bool mmuko_boot_validate(const MMUKO_BOOT_HANDOFF *handoff) {{
    if (handoff == NULL) {{
        return false;
    }}
    return memcmp(handoff->magic, MMUKO_BOOT_MAGIC, 4) == 0 &&
           handoff->revision == MMUKO_BOOT_REVISION &&
           handoff->completed_phases == MMUKO_BOOT_PHASE_COUNT &&
           handoff->outcome == MMUKO_BOOT_OUTCOME_PASS &&
           handoff->handoff_checksum == mmuko_boot_checksum(handoff);
}}

static bool mmuko_run_phase(MMUKO_BOOT_PHASE phase, MMUKO_BOOT_HANDOFF *handoff) {{
    (void)handoff;
    switch (phase) {{
{phase_cases}
        default:
            return false;
    }}
}}

bool mmuko_boot_from_spec(MMUKO_BOOT_HANDOFF *handoff) {{
    static const MMUKO_BOOT_PHASE canonical_order[] = {{
        {', '.join(phase['phase_id'] for phase in phases)}
    }};

    mmuko_boot_handoff_defaults(handoff);
    for (size_t i = 0; i < sizeof(canonical_order) / sizeof(canonical_order[0]); ++i) {{
        if (!mmuko_run_phase(canonical_order[i], handoff)) {{
            handoff->outcome = MMUKO_BOOT_OUTCOME_ALERT;
            return false;
        }}
        handoff->completed_phases += 1U;
        handoff->last_completed_phase = canonical_order[i];
        handoff->validation_flags |= (1UL << i);
    }}

    handoff->outcome = MMUKO_BOOT_OUTCOME_PASS;
    handoff->kernel_entry_segment = 0x1000;
    handoff->kernel_entry_offset = 0x0000;
    handoff->handoff_checksum = mmuko_boot_checksum(handoff);
    return true;
}}
'''


def generate_spec_header(phases: List[Dict[str, str]], fs: Dict[str, str], contract: Dict[str, str]) -> str:
    enum_rows = "\n".join(
        f"    {phase['phase_id']} = {phase['order']}, /* {phase['title']} */"
        for phase in phases
    )
    return f'''#ifndef MMUKO_BOOT_SPEC_H
#define MMUKO_BOOT_SPEC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MMUKO_BOOT_MAGIC "{c_string(contract['magic'])}"
#define MMUKO_BOOT_REVISION {contract['revision']}
#define MMUKO_BOOT_FILESYSTEM_TARGET "{c_string(fs['layout'])}:{c_string(fs['image_name'])}"
#define MMUKO_BOOT_KERNEL_PATH "{c_string(fs['root_artifact'])}"
#define MMUKO_BOOT_FALLBACK_PATH "{c_string(fs['fallback_artifact'])}"
#define MMUKO_BOOT_CONFIG_PATH "{c_string(fs['config_artifact'])}"
#define MMUKO_BOOT_MANIFEST_PATH "{c_string(fs['manifest_artifact'])}"
#define MMUKO_BOOT_SIGNATURE {fs['boot_signature']}
#define MMUKO_BOOT_SECTOR_BYTES {fs['boot_sector_bytes']}
#define MMUKO_BOOT_PHASE_COUNT {contract['phase_count']}

typedef enum {{
    MMUKO_BOOT_OUTCOME_PASS = 0xAA,
    MMUKO_BOOT_OUTCOME_HOLD = 0xBB,
    MMUKO_BOOT_OUTCOME_ALERT = 0xCC
}} MMUKO_BOOT_OUTCOME;

typedef enum {{
{enum_rows}
}} MMUKO_BOOT_PHASE;

typedef struct {{
    char magic[4];
    uint16_t revision;
    uint8_t outcome;
    uint8_t completed_phases;
    uint8_t phase_count;
    uint8_t last_completed_phase;
    char filesystem_target[32];
    char kernel_path[64];
    char artifact_manifest_path[64];
    char config_path[64];
    uint16_t kernel_entry_segment;
    uint16_t kernel_entry_offset;
    uint32_t validation_flags;
    uint32_t handoff_checksum;
}} MMUKO_BOOT_HANDOFF;

uint32_t mmuko_boot_checksum(const MMUKO_BOOT_HANDOFF *handoff);
bool mmuko_boot_validate(const MMUKO_BOOT_HANDOFF *handoff);

#endif
'''


def generate_runtime_header(contract: Dict[str, str]) -> str:
    return f'''#ifndef MMUKO_RUNTIME_INTERFACE_H
#define MMUKO_RUNTIME_INTERFACE_H

#include <stdbool.h>
#include <stdint.h>

#include "mmuko_boot_spec.h"

typedef struct {{
    const MMUKO_BOOT_HANDOFF *handoff;
    uint32_t expected_revision;
    uint8_t required_phase_count;
}} MMUKO_RUNTIME_INTERFACE;

static inline bool mmuko_runtime_can_enter(const MMUKO_RUNTIME_INTERFACE *runtime) {{
    return runtime != 0 &&
           runtime->handoff != 0 &&
           runtime->handoff->outcome == MMUKO_BOOT_OUTCOME_PASS &&
           runtime->handoff->completed_phases == runtime->required_phase_count &&
           runtime->handoff->revision == runtime->expected_revision;
}}

static inline const char *mmuko_runtime_kernel_path(const MMUKO_RUNTIME_INTERFACE *runtime) {{
    return (runtime != 0 && runtime->handoff != 0) ? runtime->handoff->kernel_path : "";
}}

#endif
'''


def generate_validation_json(phases, fs, contract, firmware, artifacts) -> str:
    payload = {
        "boot_phases": phases,
        "filesystem_target": fs,
        "kernel_handoff_contract": contract,
        "nsigii_firmware_requirements": firmware,
        "artifact_names": artifacts,
    }
    return json.dumps(payload, indent=2) + "\n"


def main() -> int:
    text = SPEC_PATH.read_text(encoding="utf-8")
    sections = parse_spec(text)
    phases = parse_phase_rows(sections["Boot Phases"])
    fs = parse_keyvals(sections["Filesystem Target"])
    contract = parse_keyvals(sections["Kernel Handoff Contract"])
    firmware = parse_keyvals(sections["NSIGII Firmware Requirements"])
    artifacts = parse_keyvals(sections["Artifact Names"])

    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    INCLUDE_DIR.mkdir(parents=True, exist_ok=True)

    (OUTPUT_DIR / "boot.asm").write_text(generate_boot_asm(phases, fs, contract), encoding="utf-8")
    (OUTPUT_DIR / "mmuko-boot.c").write_text(generate_boot_c(phases, fs, contract, artifacts), encoding="utf-8")
    (INCLUDE_DIR / "mmuko_boot_spec.h").write_text(generate_spec_header(phases, fs, contract), encoding="utf-8")
    (INCLUDE_DIR / "mmuko_runtime_interface.h").write_text(generate_runtime_header(contract), encoding="utf-8")
    (OUTPUT_DIR / "spec-validation.json").write_text(
        generate_validation_json(phases, fs, contract, firmware, artifacts),
        encoding="utf-8",
    )

    print(f"Generated: {OUTPUT_DIR / 'boot.asm'}")
    print(f"Generated: {OUTPUT_DIR / 'mmuko-boot.c'}")
    print(f"Generated: {INCLUDE_DIR / 'mmuko_boot_spec.h'}")
    print(f"Generated: {INCLUDE_DIR / 'mmuko_runtime_interface.h'}")
    print(f"Generated: {OUTPUT_DIR / 'spec-validation.json'}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
