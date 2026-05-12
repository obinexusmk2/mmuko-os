# /AGENTS.md

# MMUKO-OS Agent Instructions

This repository contains generated artifacts and low-level boot/runtime code.
Agents must treat the specification and pseudocode as the primary authority.

## Source-of-truth order

Follow this order strictly:

1. `MMUKO-OS.txt`
   - Canonical, authoritative specification.
2. `mmuko-boot/pseudocode/mmuko-boot.psc`
   - Primary boot behavioral pseudocode.
3. Other `.psc` files under `mmuko-boot/pseudocode/`
   - Supporting context only.
4. Generated outputs under:
   - `boot/`
   - `kernel/`
   - `include/`
   - `python/`
   These are derived artifacts, not primary sources.

If any generated file conflicts with `MMUKO-OS.txt`, the spec wins.

## Edit policy

Prefer minimal, surgical changes.

Do not hand-edit generated files unless the task is explicitly to repair or inspect generated output.
When changing generated output behavior, modify the generator first, then regenerate artifacts.

Primary generator area:
- `tools/mmuko_codegen/`

Generated outputs:
- `boot/mmuko_stage1_boot.asm`
- `kernel/mmuko_stage2_loader.c`
- `kernel/mmuko_stage2_bridge.cpp`
- `include/mmuko_codegen.h`
- `python/mmuko_codegen.pxd`
- `python/mmuko_generated.pyx`
- `tools/mmuko_codegen/manifest.txt`

## Boot contract constraints

The canonical BIOS boot path is a raw fixed-sector layout.

Do not introduce or preserve filesystem assumptions in the BIOS boot chain unless the spec explicitly changes.

Current canonical boot layout:
- LBA 0: stage-1 boot sector
- LBA 1-16: stage-2 loader
- LBA 17-48: runtime image
- LBA 49+: reserved

The BIOS boot path is not FAT12, FAT32, or "FAT64" unless a future spec explicitly defines such a format.

Do not emit FAT-identifying metadata in generated boot code if it contradicts the canonical raw-layout spec.

## `.som` container constraints

`.som` is a custom MMUKO container format.

Do not treat `.som` as an alias for:
- `.so`
- `.dll`

Respect the header contract defined in `MMUKO-OS.txt`, including:
- fixed 64-byte header
- little-endian fields
- CRC32 of payload bytes
- payload format and entry kind semantics

If generated code claims to compute CRC32, it must compute real CRC32.
If it does not compute CRC32, rename the field/function to avoid false claims.

## Pseudocode/codegen expectations

The generator must parse pseudocode structurally, not heuristically where correctness matters.

Required behaviors:
- parse enums correctly
- parse structs correctly
- parse `REQUIRE` expressions correctly
- parse boot phases by actual phase blocks, not round-robin slicing
- keep manifest paths deterministic and repo-relative where possible

If a pseudocode reference cannot be represented directly in generated code, do one of:
1. add explicit representation,
2. map it to generated metadata,
3. fail generation with a clear error.

Do not silently drop semantic inputs.

## Known ambiguity policy

There is known tension between:
- raw-sector boot behavior in `MMUKO-OS.txt`
- file/path-oriented fields in `mmuko-boot.psc`

When blocked by ambiguity:
- do not invent new behavior
- do not guess
- document the ambiguity in the PR
- preserve canonical spec priority

## Manifest rules

`tools/mmuko_codegen/manifest.txt` must be generated, not manually curated.

Requirements:
- no merge conflict markers
- `authoritative_input` must match the actual `--spec` input
- `primary_pseudocode` must match the actual `--primary` input
- `pseudocode_sources` must be computed from the actual pseudocode directory contents
- emitted paths should be deterministic

## Testing requirements

When modifying `tools/mmuko_codegen/`, add or update tests for:
- enum parsing
- struct parsing
- function parsing if touched
- `REQUIRE` extraction
- phase block extraction
- manifest generation
- path normalization
- CRC32 behavior if checksum code changes

Prefer small unit tests over broad integration-only coverage.

## PR expectations for agents

For any non-trivial PR, include:
- root cause
- files changed
- whether outputs were regenerated
- any remaining spec ambiguity
- why the chosen fix is minimal and spec-consistent

## Safety rails

Do not:
- invent undocumented boot sectors, filesystems, or loader behavior
- rebrand raw disk layout as FAT
- claim checksums are CRC32 unless they are
- leave generated and source files inconsistent after a generator change
- resolve spec conflicts by preference for existing code over the canonical spec

## Suggested workflow

1. Read `MMUKO-OS.txt`
2. Read `mmuko-boot/pseudocode/mmuko-boot.psc`
3. Inspect `tools/mmuko_codegen/`
4. Make minimal generator/spec-aligned changes
5. Regenerate artifacts
6. Run tests
7. Summarize ambiguities explicitly
