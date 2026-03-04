# MMUKO-OS Export Manifest Specification

## Purpose
This specification defines the required release artifacts and naming rules for MMUKO-OS exports across Windows, Linux, and macOS.

## Required Outputs Per Platform
Each platform export **must** contain the following artifact categories:

1. **Boot image**
   - Format: `.img`
   - Required file role: raw boot image suitable for emulator/VM boot testing.
2. **Native binary**
   - Format: executable (`.exe` for Windows, no extension for Linux/macOS).
   - Required file role: platform-specific (or compatible fallback) command-line native build.
3. **UI bundle**
   - Format: `.zip`
   - Required file role: distributable UI/design asset package.
4. **Metadata/checksums**
   - Format: release-level `export-manifest.json` + `SHA256SUMS` text file.
   - Required file role: machine-readable inventory and integrity verification.

## Platforms
Exports are required for:
- `windows`
- `linux`
- `macos`

## Naming and Versioning
Artifacts must use this convention:

`mmuko-os_<version>_<platform>_<artifact-kind>.<ext>`

Examples:
- `mmuko-os_0.1.0_windows_boot.img`
- `mmuko-os_0.1.0_linux_native`
- `mmuko-os_0.1.0_macos_ui-bundle.zip`

Release layout:

- `dist/exports/<release>/windows/...`
- `dist/exports/<release>/linux/...`
- `dist/exports/<release>/macos/...`
- `dist/exports/<release>/export-manifest.json`
- `dist/exports/<release>/mmuko-os_<version>_SHA256SUMS.txt`

## Machine-Readable Manifest
`export-manifest.json` must include:
- `schema_version`
- `release`
- `version`
- `generated_at` (ISO 8601 UTC)
- `platforms[]`
  - `platform`
  - `artifacts[]`
    - `category` (`boot_image`, `native_binary`, `ui_bundle`)
    - `file` (relative path)
    - `sha256`
    - `size`
    - optional `note` (for fallback/cross-compile provenance)
- `metadata`
  - `checksums`
  - `checksums_sha256`

## Integrity Requirements
- SHA-256 must be generated for every artifact.
- `SHA256SUMS` must include one line per artifact in format:

`<sha256>  <filename>`

- `export-manifest.json` must embed the same SHA-256 values used in the checksum file.

## Reference Implementation
Use:
- `scripts/export_artifacts.py`
- `scripts/export_artifacts.sh`

Example:

```bash
scripts/export_artifacts.sh 0.1.0
```

This writes a complete release export under `dist/exports/v0.1.0/`.
