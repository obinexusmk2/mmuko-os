#!/usr/bin/env python3
"""Generate MMUKO-OS native and Python bindings from pseudocode sources."""
from __future__ import annotations

import argparse
from dataclasses import dataclass
import re
from pathlib import Path
from textwrap import dedent



@dataclass(frozen=True)
class PhaseBlock:
    phase_number: int
    enum_name: str
    requirements: list[str]
    binds: list[str]
    completion_flag: str


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


def _psc_type_to_c(
    psc_type: str,
    field_name: str,
    enum_type_names: set[str] | None = None,
) -> str:
    """Convert a PSC type string to a C field declaration."""
    m = _PSC_ARRAY_RE.fullmatch(psc_type)
    if m:
        return f"char {field_name}[{m.group(1)}]"
    if enum_type_names and psc_type in enum_type_names:
        return f"{psc_type} {field_name}"
    if psc_type in _PSC_TYPE_MAP:
        return f"{_PSC_TYPE_MAP[psc_type]} {field_name}"
    raise SystemExit(f"Unsupported PSC type for {field_name}: {psc_type}")


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


def _emit_c_struct(
    name: str,
    fields: list[tuple[str, str, str]],
    enum_type_names: set[str] | None = None,
) -> str:
    lines = [f"typedef struct {{"]
    for fname, ftype, _default in fields:
        lines.append(f"    {_psc_type_to_c(ftype, fname, enum_type_names)};")
    lines.append(f"}} {name}_t;")
    return "\n".join(lines)


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _write(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def _resolve_input(root: Path, path: Path) -> Path:
    """Resolve CLI input paths relative to the repository root."""
    return path.resolve() if path.is_absolute() else (root / path).resolve()


def _repo_rel(root: Path, path: Path) -> str:
    """Return a deterministic POSIX repo-relative path when possible."""
    root_resolved = root.resolve()
    path_resolved = path.resolve() if path.is_absolute() else (root_resolved / path).resolve()
    try:
        return path_resolved.relative_to(root_resolved).as_posix()
    except ValueError:
        return path_resolved.as_posix()


def _support_manifest(paths: list[Path], primary: Path, root: Path) -> list[str]:
    manifest: list[str] = []
    primary_resolved = _resolve_input(root, primary)
    for path in sorted(paths, key=lambda item: _repo_rel(root, item)):
        path_resolved = _resolve_input(root, path)
        role = "primary boot model" if path_resolved == primary_resolved else "supporting pseudocode context"
        manifest.append(f"{_repo_rel(root, path)} :: {role}")
    return manifest


def _parse_handoff_checksum_fields(text: str) -> list[str]:
    """Parse the ordered handoff field list passed to CRC32(...)."""
    match = re.search(
        r"^FUNC\s+compute_handoff_checksum\s*\([^)]*\)\s*->\s*UINT32\s*:\s*\n\s*RETURN\s+CRC32\((.*?)\)",
        text,
        re.MULTILINE | re.DOTALL,
    )
    if not match:
        return []
    fields: list[str] = []
    for raw_arg in match.group(1).split(","):
        arg = raw_arg.strip()
        field_match = re.fullmatch(r"handoff\.(\w+)", arg)
        if not field_match:
            raise SystemExit(
                "compute_handoff_checksum CRC32 arguments must be explicit handoff fields; "
                f"unsupported argument: {arg}"
            )
        fields.append(field_match.group(1))
    return fields


def _crc32_standard(data: bytes, seed: int = 0) -> int:
    """Compute standard reflected CRC32 (polynomial 0xEDB88320)."""
    crc = seed ^ 0xFFFFFFFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            mask = -(crc & 1) & 0xFFFFFFFF
            crc = ((crc >> 1) ^ (0xEDB88320 & mask)) & 0xFFFFFFFF
    return crc ^ 0xFFFFFFFF


def _c_string_literal(value: str) -> str:
    """Return a C string literal for a PSC string default."""
    if len(value) >= 2 and value[0] == '"' and value[-1] == '"':
        value = value[1:-1]
    return '"' + value.replace('\\', '\\\\').replace('"', '\\"') + '"'


def _handoff_struct_fields(structs: list[tuple[str, list[tuple[str, str, str]]]]) -> list[tuple[str, str, str]]:
    for struct_name, fields in structs:
        if struct_name == "MMUKO_BOOT_HANDOFF":
            return fields
    raise SystemExit("MMUKO_BOOT_HANDOFF struct not found in primary pseudocode")


def _emit_handoff_string_initializers(fields: list[tuple[str, str, str]]) -> str:
    lines = []
    for fname, ftype, default in fields:
        array_match = _PSC_ARRAY_RE.fullmatch(ftype)
        if array_match and default:
            array_size = int(array_match.group(1))
            value = default[1:-1] if len(default) >= 2 and default[0] == '"' and default[-1] == '"' else default
            encoded = value.encode("ascii")
            if len(encoded) > array_size:
                raise SystemExit(f"Default for {fname} exceeds {ftype} capacity")
            for index in range(array_size):
                byte = encoded[index] if index < len(encoded) else 0
                lines.append(f"            handoff->{fname}[{index}] = (char)0x{byte:02X};")
        elif ftype == "STRING" and default:
            lines.append(f"            handoff->{fname} = {_c_string_literal(default)};")
    return "\n".join(lines)


def _emit_crc32_field_update(field_name: str, field_type: str) -> str:
    if field_type.startswith("CHAR["):
        return f"            crc = mmuko_crc32_update(crc, h->{field_name}, sizeof(h->{field_name}));"
    if field_type == "STRING":
        return (
            f"            if (h->{field_name} == 0) {{ return 0; }}\n"
            f"            crc = mmuko_crc32_update(crc, h->{field_name}, strlen(h->{field_name}) + 1u);"
        )
    if field_type == "UINT8":
        return f"            crc = mmuko_crc32_update_u8(crc, h->{field_name});"
    if field_type == "UINT16":
        return f"            crc = mmuko_crc32_update_u16le(crc, h->{field_name});"
    # PSC enums are emitted as integer-backed C fields by _psc_type_to_c.
    return f"            crc = mmuko_crc32_update_u32le(crc, (uint32_t)h->{field_name});"


def _emit_handoff_crc32_body(
    checksum_fields: list[str],
    handoff_fields: list[tuple[str, str, str]],
    primary_display: str,
) -> str:
    fields_by_name = {fname: ftype for fname, ftype, _default in handoff_fields}
    if not checksum_fields:
        checksum_fields = [fname for fname, _ftype, _default in handoff_fields if fname != "handoff_checksum"]
    if "handoff_checksum" in checksum_fields:
        raise SystemExit("compute_handoff_checksum must not include handoff.handoff_checksum in its CRC32 input")
    missing = [field for field in checksum_fields if field not in fields_by_name]
    if missing:
        raise SystemExit(
            f"{primary_display} compute_handoff_checksum references unknown handoff field(s): "
            + ", ".join(missing)
        )
    updates = "\n".join(_emit_crc32_field_update(field, fields_by_name[field]) for field in checksum_fields)
    return f"""\
        static uint32_t mmuko_crc32_update(uint32_t crc, const void *data, size_t len) {{
            const uint8_t *bytes = (const uint8_t *)data;
            crc ^= 0xFFFFFFFFu;
            for (size_t i = 0; i < len; ++i) {{
                crc ^= (uint32_t)bytes[i];
                for (unsigned bit = 0; bit < 8; ++bit) {{
                    uint32_t mask = 0u - (crc & 1u);
                    crc = (crc >> 1) ^ (0xEDB88320u & mask);
                }}
            }}
            return crc ^ 0xFFFFFFFFu;
        }}

        static uint32_t mmuko_crc32_update_u8(uint32_t crc, uint8_t value) {{
            return mmuko_crc32_update(crc, &value, sizeof(value));
        }}

        static uint32_t mmuko_crc32_update_u16le(uint32_t crc, uint16_t value) {{
            uint8_t bytes[2] = {{
                (uint8_t)(value & 0xFFu),
                (uint8_t)((value >> 8) & 0xFFu),
            }};
            return mmuko_crc32_update(crc, bytes, sizeof(bytes));
        }}

        static uint32_t mmuko_crc32_update_u32le(uint32_t crc, uint32_t value) {{
            uint8_t bytes[4] = {{
                (uint8_t)(value & 0xFFu),
                (uint8_t)((value >> 8) & 0xFFu),
                (uint8_t)((value >> 16) & 0xFFu),
                (uint8_t)((value >> 24) & 0xFFu),
            }};
            return mmuko_crc32_update(crc, bytes, sizeof(bytes));
        }}

        static uint32_t compute_handoff_checksum(const MMUKO_BOOT_HANDOFF_t *h) {{
            /* Standard CRC32 over the pseudocode-listed serialized handoff fields.
             * handoff_checksum is intentionally excluded. STRING values are
             * serialized as their NUL-terminated byte sequences, not pointers.
             */
            uint32_t crc = 0;
{updates}
            return crc;
        }}
"""

def _spec_declares_raw_boot_path(spec_text: str) -> bool:
    """Return True when the canonical spec declares a raw fixed-sector BIOS path."""
    normalized = " ".join(spec_text.lower().split())
    return (
        "raw" in normalized
        and "fixed" in normalized
        and "lba 0" in normalized
        and "lba 1-16" in normalized
        and "not fat12" in normalized
    )


def _validate_boot_filesystem_contract(spec_text: str, primary_text: str, primary_display: str) -> None:
    """Reject FAT-oriented boot pseudocode when the spec declares the raw layout."""
    if not _spec_declares_raw_boot_path(spec_text):
        return

    fat_matches = [
        line.strip()
        for line in primary_text.splitlines()
        if "filesystem_target" in line and re.search(r"FAT(?:12|32|64)", line)
    ]
    if fat_matches:
        details = "; ".join(fat_matches)
        raise SystemExit(
            f"{primary_display} conflicts with MMUKO-OS.txt: canonical BIOS boot "
            f"path is the raw fixed-sector layout (LBA 0 stage-1, LBA 1-16 "
            f"stage-2, LBA 17-48 runtime), but FAT-oriented filesystem_target "
            f"pseudocode was found: {details}"
        )


def _require_comment_text(expr: str) -> str:
    """Represent a PSC REQUIRE expression for generated comments."""
    return expr

def _parse_phase_blocks(text: str) -> list[PhaseBlock]:
    """Parse structured phase blocks from ``FUNC mmuko_boot``.

    A phase starts at a comment such as ``// Phase 1 — PHASE_NEED_STATE_INIT``
    and ends at the matching ``complete_phase(handoff, PHASE_..., flag)`` call.
    Requirements from ``KERNEL ENTRY CONTRACT`` intentionally remain outside the
    returned phase data so phase runners only reflect boot-phase requirements.
    """
    phases: list[PhaseBlock] = []
    in_boot = False
    in_phase = False
    phase_number = 0
    phase_enum = ""
    current_requires: list[str] = []
    current_binds: list[str] = []

    phase_comment_re = re.compile(r"^//\s*Phase\s+(\d+)\s*(?:[-—]\s*(\w+))?\s*$")
    complete_re = re.compile(
        r"^complete_phase\(\s*handoff\s*,\s*(\w+)\s*,\s*(0x[0-9A-Fa-f]+|\d+)\s*\)"
    )

    for raw_line in text.splitlines():
        line = raw_line.strip()
        if re.match(r"^FUNC\s+mmuko_boot\b", line):
            in_boot = True
            continue
        if not in_boot:
            continue
        if line.startswith("ON FAILURE:") or line.startswith("KERNEL ENTRY CONTRACT:"):
            break

        phase_match = phase_comment_re.match(line)
        if phase_match:
            if in_phase:
                raise ValueError("phase block ended before complete_phase call")
            in_phase = True
            phase_number = int(phase_match.group(1))
            phase_enum = phase_match.group(2) or ""
            current_requires = []
            current_binds = []
            continue

        if not in_phase:
            continue

        require_match = re.match(r"^REQUIRE\s+(.+)$", line)
        if require_match:
            current_requires.append(require_match.group(1).strip())
            continue

        bind_match = re.match(r"^BIND\s+(.+?)\s+INTO\s+handoff\s*$", line)
        if bind_match:
            current_binds.append(bind_match.group(1).strip())
            continue

        complete_match = complete_re.match(line)
        if complete_match:
            completed_enum = complete_match.group(1)
            if phase_enum and completed_enum != phase_enum:
                raise ValueError(
                    f"phase {phase_number} comment names {phase_enum}, "
                    f"but complete_phase uses {completed_enum}"
                )
            phases.append(
                PhaseBlock(
                    phase_number=phase_number,
                    enum_name=completed_enum,
                    requirements=current_requires,
                    binds=current_binds,
                    completion_flag=complete_match.group(2),
                )
            )
            in_phase = False
            phase_number = 0
            phase_enum = ""
            current_requires = []
            current_binds = []

    if in_phase:
        raise ValueError("unterminated phase block in mmuko_boot")
    return phases


def _parse_boot_phase_requires(text: str) -> list[tuple[str, str, list[str]]]:
    """Compatibility wrapper returning phase enum, flag, and REQUIREs."""
    return [
        (phase.enum_name, phase.completion_flag, phase.requirements)
        for phase in _parse_phase_blocks(text)
    ]


def _validate_handoff_binds(
    phases: list[PhaseBlock],
    handoff_fields: list[tuple[str, str, str]],
    primary_display: str,
) -> dict[str, str]:
    """Ensure every ``BIND <name> INTO handoff`` has an explicit handoff field.

    The boot pseudocode binds identity/time values into the handoff record.
    Treating a phase with unrepresented BINDs as successful would silently drop
    that semantic input, so generation fails unless each BIND target is present
    in ``MMUKO_BOOT_HANDOFF``.
    """
    field_names = {fname for fname, _ftype, _default in handoff_fields}
    bound_names = [bind for phase in phases for bind in phase.binds]
    missing = sorted({name for name in bound_names if name not in field_names})
    if missing:
        details = ", ".join(f"BIND {name} INTO handoff" for name in missing)
        raise SystemExit(
            f"{primary_display} BIND target has no handoff representation: {details}. "
            "Add explicit MMUKO_BOOT_HANDOFF field(s), map the BIND to documented "
            "generated metadata, or remove/clarify the pseudocode binding."
        )
    return {name: name for name in bound_names}


def _emit_bind_comments(phase: PhaseBlock, bind_map: dict[str, str]) -> str:
    if not phase.binds:
        return ""
    return "\n".join(
        f"    /* BIND {name} INTO handoff — represented by MMUKO_BOOT_HANDOFF.{bind_map[name]} */"
        for name in phase.binds
    )


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
    root = root.resolve()
    spec_file = _resolve_input(root, spec_path)
    primary_file = _resolve_input(root, primary)
    pseudocode_path = _resolve_input(root, pseudocode_dir)

    psc_files = sorted(pseudocode_path.glob("*.psc"), key=lambda item: _repo_rel(root, item))
    if primary_file.resolve() not in {path.resolve() for path in psc_files}:
        raise SystemExit(f"Primary pseudocode file not found in {pseudocode_path}: {primary_file}")

    primary_text = primary_file.read_text(encoding="utf-8")
    spec_text = spec_file.read_text(encoding="utf-8")

    spec_display = _repo_rel(root, spec_file)
    primary_display = _repo_rel(root, primary_file)
    _validate_boot_filesystem_contract(spec_text, primary_text, primary_display)

    parsed_functions = _parse_functions(primary_text)
    parsed_constants = _parse_constants(primary_text)
    parsed_enums = _parse_enums(primary_text)
    parsed_structs = _parse_structs(primary_text)
    parsed_requires = _parse_requires(primary_text)
    checksum_fields = _parse_handoff_checksum_fields(primary_text)
    handoff_fields = _handoff_struct_fields(parsed_structs)
    handoff_crc32_body = _emit_handoff_crc32_body(checksum_fields, handoff_fields, primary_display)
    string_initializers = _emit_handoff_string_initializers(handoff_fields)
    boot_phase_blocks = _parse_phase_blocks(primary_text)
    if len(boot_phase_blocks) != 6:
        raise SystemExit(f"Expected 6 boot phase blocks in {primary_display}, found {len(boot_phase_blocks)}")
    bind_handoff_fields = _validate_handoff_binds(boot_phase_blocks, handoff_fields, primary_display)
    support_manifest = _support_manifest(psc_files, primary_file, root)

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
    enum_type_names = {name for name, _members in parsed_enums}
    enum_decls = "\n\n".join(_emit_c_enum(name, members) for name, members in parsed_enums)
    struct_decls = "\n\n".join(
        _emit_c_struct(name, fields, enum_type_names) for name, fields in parsed_structs
    )

    # ------------------------------------------------------------------
    # Boot phase validation function bodies
    # Phase requires are grouped: lines before "complete_phase" calls
    # map naturally to phase bodies via sequential order.
    # ------------------------------------------------------------------
    phase_bodies: list[str] = []
    for idx, phase in enumerate(boot_phase_blocks):
        pname = phase.enum_name
        pflag = phase.completion_flag
        phase_requires = phase.requirements
        req_guards = []
        for req in phase_requires:
            guard = (
                f"    /* REQUIRE {_require_comment_text(req)} — represented from {pname}; resolved at runtime */\n"
                f"    /* mmuko_probe stub: returns 1 (pass) until platform impl provided */"
            )
            req_guards.append(guard)
        req_block = "\n".join(req_guards) if req_guards else "    /* no explicit REQUIRE for this phase */"
        bind_block = _emit_bind_comments(phase, bind_handoff_fields)
        semantic_lines = req_block if not bind_block else f"{req_block}\n{bind_block}"
        body = (
            f"static int mmuko_run_phase_{idx + 1}(MMUKO_BOOT_HANDOFF_t *handoff) {{\n"
            f"    /* {pname} */\n"
            f"{semantic_lines}\n"
            f"    handoff->completed_phases++;\n"
            f"    handoff->last_completed_phase = {idx + 1};\n"
            f"    handoff->validation_flags |= {pflag}u;\n"
            f"{req_block}\n"
            f"    complete_phase(handoff, MMUKO_BOOT_PHASE_{pname}, {pflag}u);\n"
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
        ; Authoritative input: {spec_display}
        ; Primary pseudocode: {primary_display}
        ; Supporting pseudocode count: {len(psc_files)}
        ; Parsed ENUM types: {', '.join(n for n, _ in parsed_enums)}
        ; Parsed STRUCT types: {', '.join(n for n, _ in parsed_structs)}
        ; Boot contract: MMUKO magic, 6 phases, outcome PASS=0xAA
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

        ; Raw fixed-sector MMUKO boot layout. This reserved metadata is not a
        ; BIOS Parameter Block and intentionally carries no filesystem label.
        mmuko_layout_magic  db "MMUKORAW"
        mmuko_stage2_lba    dw 1
        mmuko_stage2_count  dw 16
        mmuko_runtime_lba   dw 17
        mmuko_runtime_count dw 32
        mmuko_reserved      times 8 db 0

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
         * Authoritative input: {spec_display}
         * Primary pseudocode: {primary_display}
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

        /* --- Enums parsed from {primary_file.name} --- */
        {enum_decls}

        /* --- Structs parsed from {primary_file.name} --- */
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
         * Authoritative input: {spec_display}
         * Primary pseudocode: {primary_display}
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
        /* Boot handoff — 6-phase NSIGII runner (from {primary_file.name})         */
        /* ------------------------------------------------------------------ */

{handoff_crc32_body}
        static void complete_phase(MMUKO_BOOT_HANDOFF_t *h, MMUKO_BOOT_PHASE phase, uint32_t flag) {{
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
            handoff->magic[2] = 'U'; handoff->magic[3] = 'K';
            handoff->magic[4] = 'O';
            handoff->revision           = 0x0001;
            handoff->firmware_id[0]     = 'N'; handoff->firmware_id[1] = 'S';
            handoff->firmware_id[2]     = 'I'; handoff->firmware_id[3] = 'G';
            handoff->firmware_id[4]     = 'I'; handoff->firmware_id[5] = 'I';
            handoff->outcome            = MMUKO_BOOT_OUTCOME_HOLD;
            handoff->completed_phases   = 0;
            handoff->kernel_entry_segment = 0x0000;
            handoff->kernel_entry_offset  = 0x0000;
            handoff->validation_flags     = 0;
{string_initializers}

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
            /* Kernel entry contract (from {primary_file.name} KERNEL ENTRY CONTRACT section) */
            if (h->magic[0] != 'M' || h->magic[1] != 'M' ||
                h->magic[2] != 'U' || h->magic[3] != 'K' ||
                h->magic[4] != 'O') {{
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
        // Authoritative input: {spec_display}
        // Primary pseudocode: {primary_display}
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
            report << "Authoritative input: {spec_display}\\n";
            report << "Primary pseudocode: {primary_display}\\n";
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
        # Authoritative input: {spec_display}
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
        authoritative_input={spec_display}
        primary_pseudocode={primary_display}
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
