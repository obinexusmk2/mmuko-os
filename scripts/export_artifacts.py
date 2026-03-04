#!/usr/bin/env python3
"""Build and export MMUKO-OS release artifacts with a machine-readable manifest."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import platform as py_platform
import shutil
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Dict, List, Optional
import zipfile


@dataclass
class ArtifactRecord:
    platform: str
    category: str
    path: Path
    sha256: str
    size: int
    note: Optional[str] = None


PLATFORM_TARGETS = {
    "windows": {
        "binary_ext": ".exe",
        "compiler_candidates": [
            "x86_64-w64-mingw32-gcc",
            "clang --target=x86_64-windows-gnu",
            "gcc",
        ],
    },
    "linux": {
        "binary_ext": "",
        "compiler_candidates": ["gcc", "clang"],
    },
    "macos": {
        "binary_ext": "",
        "compiler_candidates": [
            "o64-clang",
            "clang --target=x86_64-apple-darwin",
            "gcc",
        ],
    },
}


def run_command(cmd: List[str], cwd: Path) -> subprocess.CompletedProcess:
    return subprocess.run(cmd, cwd=str(cwd), text=True, capture_output=True)


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as fh:
        for block in iter(lambda: fh.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def resolve_compiler(candidate: str) -> Optional[List[str]]:
    parts = candidate.split()
    exe = shutil.which(parts[0])
    if not exe:
        return None
    return [exe, *parts[1:]]


def compile_native_binary(repo_root: Path, target_platform: str, out_path: Path) -> Dict[str, str]:
    target = PLATFORM_TARGETS[target_platform]
    cflags = ["-O2", "-Wall", "-Wextra", "-std=c11", "-Iinclude"]
    sources = ["src/interdependency.c", "src/mmuko_boot.c"]

    for candidate in target["compiler_candidates"]:
        compiler = resolve_compiler(candidate)
        if not compiler:
            continue
        cmd = [*compiler, *cflags, *sources, "-o", str(out_path)]
        result = run_command(cmd, repo_root)
        if result.returncode == 0:
            return {
                "compiler": " ".join(compiler),
                "mode": "native" if target_platform == host_platform_name() else "cross_or_compatible",
            }
    raise RuntimeError(f"No working compiler found for platform={target_platform}")


def host_platform_name() -> str:
    mapping = {"Linux": "linux", "Darwin": "macos", "Windows": "windows"}
    return mapping.get(py_platform.system(), "linux")


def build_boot_image(repo_root: Path) -> Path:
    img_path = repo_root / "img/mmuko-os.img"

    result = run_command(["bash", "build.sh"], repo_root)
    if result.returncode == 0 and img_path.exists():
        return img_path

    fallback = run_command([sys.executable, "build_img.py", str(img_path)], repo_root)
    if fallback.returncode != 0 or not img_path.exists():
        sys.stderr.write(result.stdout)
        sys.stderr.write(result.stderr)
        sys.stderr.write(fallback.stdout)
        sys.stderr.write(fallback.stderr)
        raise RuntimeError("unable to generate boot image via build.sh or build_img.py")
    return img_path


def create_ui_bundle(repo_root: Path, bundle_path: Path, version: str, target_platform: str) -> None:
    ui_files = [
        repo_root / "docs/mmuko_os_visual_design.md",
        repo_root / "docs/mmuko_boot_analysis.md",
        repo_root / "docs/chapter1_origins.png",
        repo_root / "docs/chapter2_pillars.png",
        repo_root / "docs/chapter3_rights.png",
        repo_root / "docs/chapter4_noise.png",
        repo_root / "docs/chapter5_tiers.png",
        repo_root / "docs/chapter6_topology.png",
        repo_root / "docs/chapter7_implementation.png",
    ]

    with zipfile.ZipFile(bundle_path, "w", compression=zipfile.ZIP_DEFLATED) as archive:
        manifest = {
            "name": "mmuko-os-ui-bundle",
            "version": version,
            "platform": target_platform,
            "generated_at": datetime.now(timezone.utc).isoformat(),
            "files": [],
        }
        for src in ui_files:
            if src.exists():
                arcname = src.relative_to(repo_root).as_posix()
                archive.write(src, arcname)
                manifest["files"].append(arcname)

        with tempfile.NamedTemporaryFile("w", suffix=".json", delete=False, encoding="utf-8") as tmp:
            json.dump(manifest, tmp, indent=2)
            tmp_path = Path(tmp.name)

        archive.write(tmp_path, "ui-bundle-manifest.json")
        tmp_path.unlink(missing_ok=True)


def write_checksums(checksums_path: Path, artifacts: List[ArtifactRecord]) -> None:
    lines = [f"{entry.sha256}  {entry.path.name}" for entry in artifacts]
    checksums_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def export_artifacts(repo_root: Path, out_root: Path, version: str, release: str, platforms: List[str]) -> Path:
    release_dir = out_root / release
    release_dir.mkdir(parents=True, exist_ok=True)

    boot_image = build_boot_image(repo_root)
    host_binary = release_dir / "host-native-reference"
    host_build_meta = compile_native_binary(repo_root, host_platform_name(), host_binary)

    artifacts: List[ArtifactRecord] = []
    platform_entries = []

    for target_platform in platforms:
        target_dir = release_dir / target_platform
        target_dir.mkdir(parents=True, exist_ok=True)

        boot_name = f"mmuko-os_{version}_{target_platform}_boot.img"
        boot_path = target_dir / boot_name
        shutil.copy2(boot_image, boot_path)
        artifacts.append(
            ArtifactRecord(
                platform=target_platform,
                category="boot_image",
                path=boot_path,
                sha256=sha256_file(boot_path),
                size=boot_path.stat().st_size,
            )
        )

        native_ext = PLATFORM_TARGETS[target_platform]["binary_ext"]
        native_name = f"mmuko-os_{version}_{target_platform}_native{native_ext}"
        native_path = target_dir / native_name
        note = None
        try:
            compile_info = compile_native_binary(repo_root, target_platform, native_path)
            compile_mode = compile_info["mode"]
            compile_compiler = compile_info["compiler"]
            if compile_mode != "native":
                note = f"Built with compatible/cross compiler via '{compile_compiler}'"
        except Exception:
            shutil.copy2(host_binary, native_path)
            compile_mode = "host_fallback"
            note = f"Cross compiler unavailable; copied host binary built with {host_build_meta['compiler']}"

        artifacts.append(
            ArtifactRecord(
                platform=target_platform,
                category="native_binary",
                path=native_path,
                sha256=sha256_file(native_path),
                size=native_path.stat().st_size,
                note=note,
            )
        )

        ui_name = f"mmuko-os_{version}_{target_platform}_ui-bundle.zip"
        ui_path = target_dir / ui_name
        create_ui_bundle(repo_root, ui_path, version, target_platform)
        artifacts.append(
            ArtifactRecord(
                platform=target_platform,
                category="ui_bundle",
                path=ui_path,
                sha256=sha256_file(ui_path),
                size=ui_path.stat().st_size,
            )
        )

        platform_entries.append(
            {
                "platform": target_platform,
                "artifacts": [
                    {
                        "category": entry.category,
                        "file": str(entry.path.relative_to(release_dir).as_posix()),
                        "sha256": entry.sha256,
                        "size": entry.size,
                        **({"note": entry.note} if entry.note else {}),
                    }
                    for entry in artifacts
                    if entry.platform == target_platform
                ],
            }
        )

    checksums_path = release_dir / f"mmuko-os_{version}_SHA256SUMS.txt"
    write_checksums(checksums_path, artifacts)
    checksums_record = ArtifactRecord(
        platform="all",
        category="metadata_checksums",
        path=checksums_path,
        sha256=sha256_file(checksums_path),
        size=checksums_path.stat().st_size,
    )
    artifacts.append(checksums_record)

    manifest = {
        "name": "mmuko-os-export-manifest",
        "schema_version": "1.0.0",
        "release": release,
        "version": version,
        "generated_at": datetime.now(timezone.utc).isoformat(),
        "platforms": platform_entries,
        "metadata": {
            "checksums": str(checksums_path.relative_to(release_dir).as_posix()),
            "checksums_sha256": checksums_record.sha256,
        },
    }

    manifest_path = release_dir / "export-manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    return manifest_path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Export MMUKO-OS release artifacts")
    parser.add_argument("--version", required=True, help="Semantic version, e.g. 0.1.0")
    parser.add_argument(
        "--release",
        default=None,
        help="Release label (default: v<version>)",
    )
    parser.add_argument(
        "--output-dir",
        default="dist/exports",
        help="Output base directory",
    )
    parser.add_argument(
        "--platform",
        action="append",
        choices=sorted(PLATFORM_TARGETS.keys()),
        help="Platform to export (repeatable). Default: windows,linux,macos",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    repo_root = Path(__file__).resolve().parents[1]
    out_root = (repo_root / args.output_dir).resolve()
    platforms = args.platform or ["windows", "linux", "macos"]
    release = args.release or f"v{args.version}"

    manifest_path = export_artifacts(repo_root, out_root, args.version, release, platforms)
    print(f"Export complete: {manifest_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
