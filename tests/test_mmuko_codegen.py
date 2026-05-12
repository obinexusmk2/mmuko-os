from pathlib import Path
import tempfile
import unittest

from tools.mmuko_codegen import generate as codegen


PSC_TEXT = '''
ENUM MMUKO_BOOT_OUTCOME:
    PASS  = 0xAA
    HOLD  = 0xBB

STRUCT MMUKO_BOOT_HANDOFF:
    magic : CHAR[5] = "MMUKO"
    revision : UINT16 = 0x0001

FUNC helper():
    RETURN TRUE

FUNC mmuko_boot() -> MMUKO_BOOT_HANDOFF:
    handoff = MMUKO_BOOT_HANDOFF()

    // Phase 1 — PHASE_NEED_STATE_INIT
    REQUIRE tier1_state != NO
    complete_phase(handoff, PHASE_NEED_STATE_INIT, 0x00000001)

    // Phase 2 — PHASE_SAFETY_SCAN
    REQUIRE tier2_state != NO
    REQUIRE nsigii_minimum_safety_envelope == TRUE
    complete_phase(handoff, PHASE_SAFETY_SCAN, 0x00000002)

    // Phase 3 — PHASE_IDENTITY_CALIBRATION
    complete_phase(handoff, PHASE_IDENTITY_CALIBRATION, 0x00000004)

    // Phase 4 — PHASE_GOVERNANCE_CHECK
    REQUIRE execution_policy == VERIFIED
    REQUIRE provenance_chain == VERIFIED
    REQUIRE filesystem_target == FAT12:mmuko-os.img
    complete_phase(handoff, PHASE_GOVERNANCE_CHECK, 0x00000008)

    // Phase 5 — PHASE_INTERNAL_PROBE
    REQUIRE memory_map_integrity == TRUE
    complete_phase(handoff, PHASE_INTERNAL_PROBE, 0x00000010)

    // Phase 6 — PHASE_INTEGRITY_VERIFICATION
    REQUIRE kernel_entry_is_resolved == TRUE
    complete_phase(handoff, PHASE_INTEGRITY_VERIFICATION, 0x00000020)

KERNEL ENTRY CONTRACT:
    REQUIRE handoff.magic == "MMUKO"
'''


class CodegenParserTests(unittest.TestCase):
    def test_parses_enums_structs_functions_and_requires(self):
        self.assertEqual(
            codegen._parse_enums(PSC_TEXT),
            [("MMUKO_BOOT_OUTCOME", [("PASS", "0xAA"), ("HOLD", "0xBB")])],
        )
        self.assertEqual(
            codegen._parse_structs(PSC_TEXT),
            [
                (
                    "MMUKO_BOOT_HANDOFF",
                    [("magic", "CHAR[5]", '"MMUKO"'), ("revision", "UINT16", "0x0001")],
                )
            ],
        )
        self.assertIn("helper", codegen._parse_functions(PSC_TEXT))
        self.assertEqual(len(codegen._parse_requires(PSC_TEXT)), 9)

    def test_phase_requires_are_grouped_by_phase_blocks(self):
        phases = codegen._parse_boot_phase_requires(PSC_TEXT)
        self.assertEqual(len(phases), 6)
        self.assertEqual(phases[0], ("PHASE_NEED_STATE_INIT", "0x00000001", ["tier1_state != NO"]))
        self.assertEqual(phases[2], ("PHASE_IDENTITY_CALIBRATION", "0x00000004", []))
        self.assertEqual(
            phases[3],
            (
                "PHASE_GOVERNANCE_CHECK",
                "0x00000008",
                ["execution_policy == VERIFIED", "provenance_chain == VERIFIED", "filesystem_target == FAT12:mmuko-os.img"],
            ),
        )

    def test_manifest_paths_are_repo_relative_and_generated_from_psc_dir(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "mmuko-boot" / "pseudocode").mkdir(parents=True)
            (root / "boot").mkdir()
            (root / "kernel").mkdir()
            (root / "include").mkdir()
            (root / "python").mkdir()
            (root / "tools" / "mmuko_codegen").mkdir(parents=True)
            (root / "MMUKO-OS.txt").write_text("spec\n", encoding="utf-8")
            primary = root / "mmuko-boot" / "pseudocode" / "mmuko-boot.psc"
            primary.write_text(PSC_TEXT, encoding="utf-8")

            codegen.generate(
                root,
                Path("MMUKO-OS.txt"),
                Path("mmuko-boot/pseudocode/mmuko-boot.psc"),
                Path("mmuko-boot/pseudocode"),
            )

            manifest = (root / "tools" / "mmuko_codegen" / "manifest.txt").read_text(encoding="utf-8")
            self.assertIn("authoritative_input=MMUKO-OS.txt", manifest)
            self.assertIn("primary_pseudocode=mmuko-boot/pseudocode/mmuko-boot.psc", manifest)
            self.assertIn("pseudocode_sources=1", manifest)
            asm = (root / "boot" / "mmuko_stage1_boot.asm").read_text(encoding="utf-8")
            self.assertNotIn("FAT12", asm)
            self.assertIn("MMUKORAW", asm)
            loader = (root / "kernel" / "mmuko_stage2_loader.c").read_text(encoding="utf-8")
            self.assertNotIn("FAT12", loader)


if __name__ == "__main__":
    unittest.main()
