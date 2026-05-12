from pathlib import Path

from tools.mmuko_codegen import generate as codegen


def test_standard_crc32_known_vectors():
    assert codegen._crc32_standard(b"") == 0x00000000
    assert codegen._crc32_standard(b"123456789") == 0xCBF43926
    assert codegen._crc32_standard(b"MMUKO") == 0x90A6CD91


def test_handoff_checksum_fields_exclude_checksum_field():
    fields = codegen._parse_handoff_checksum_fields(
        Path("mmuko-boot/pseudocode/mmuko-boot.psc").read_text(encoding="utf-8")
    )

    assert fields == [
        "magic",
        "revision",
        "firmware_id",
        "outcome",
        "completed_phases",
        "last_completed_phase",
        "filesystem_target",
        "kernel_path",
        "artifact_manifest_path",
        "config_path",
        "kernel_entry_segment",
        "kernel_entry_offset",
        "validation_flags",
    ]
    assert "handoff_checksum" not in fields


def test_generated_stage2_uses_real_crc32_not_placeholder_checksum():
    loader = Path("kernel/mmuko_stage2_loader.c").read_text(encoding="utf-8")

    assert "0xEDB88320u" in loader
    assert "DEADBEEF" not in loader
    assert "Simple additive checksum" not in loader
    assert "handoff_checksum is intentionally excluded" in loader
