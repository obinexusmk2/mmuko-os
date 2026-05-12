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
    operator_identity : STRING = "UNBOUND_OPERATOR_IDENTITY"
    temporal_frame : STRING = "UNBOUND_TEMPORAL_FRAME"

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
    BIND operator_identity INTO handoff
    BIND temporal_frame INTO handoff
    complete_phase(handoff, PHASE_IDENTITY_CALIBRATION, 0x00000004)

    // Phase 4 — PHASE_GOVERNANCE_CHECK
    REQUIRE execution_policy == VERIFIED
    REQUIRE provenance_chain == VERIFIED
    REQUIRE filesystem_target == RAW_FIXED_SECTOR:mmuko-os.img:LBA0_STAGE1:LBA1_16_STAGE2:LBA17_48_RUNTIME
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
                    [
                        ("magic", "CHAR[5]", '"MMUKO"'),
                        ("revision", "UINT16", "0x0001"),
                        ("operator_identity", "STRING", '"UNBOUND_OPERATOR_IDENTITY"'),
                        ("temporal_frame", "STRING", '"UNBOUND_TEMPORAL_FRAME"'),
                    ],
                )
            ],
        )
        self.assertIn("helper", codegen._parse_functions(PSC_TEXT))
        self.assertEqual(len(codegen._parse_requires(PSC_TEXT)), 9)

    def test_phase_blocks_are_structured_from_mmuko_boot(self):
        phases = codegen._parse_phase_blocks(PSC_TEXT)
        self.assertEqual(len(phases), 6)
        self.assertEqual(phases[0].phase_number, 1)
        self.assertEqual(phases[0].enum_name, "PHASE_NEED_STATE_INIT")
        self.assertEqual(phases[0].requirements, ["tier1_state != NO"])
        self.assertEqual(phases[0].binds, [])
        self.assertEqual(phases[0].completion_flag, "0x00000001")
        self.assertEqual(
            phases[1].requirements,
            ["tier2_state != NO", "nsigii_minimum_safety_envelope == TRUE"],
        )
        self.assertEqual(phases[2].requirements, [])
        self.assertEqual(phases[2].binds, ["operator_identity", "temporal_frame"])

    def test_phase_3_binds_are_detected_and_emitted_when_represented(self):
        phases = codegen._parse_phase_blocks(PSC_TEXT)
        self.assertEqual(phases[2].phase_number, 3)
        self.assertEqual(phases[2].binds, ["operator_identity", "temporal_frame"])

        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "mmuko-boot" / "pseudocode").mkdir(parents=True)
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

            loader = (root / "kernel" / "mmuko_stage2_loader.c").read_text(encoding="utf-8")
            self.assertIn(
                "BIND operator_identity INTO handoff — represented by MMUKO_BOOT_HANDOFF.operator_identity",
                loader,
            )
            self.assertIn(
                "BIND temporal_frame INTO handoff — represented by MMUKO_BOOT_HANDOFF.temporal_frame",
                loader,
            )

    def test_phase_3_binds_fail_when_handoff_fields_are_missing(self):
        unrepresented_text = PSC_TEXT.replace(
            '    operator_identity : STRING = "UNBOUND_OPERATOR_IDENTITY"\n'
            '    temporal_frame : STRING = "UNBOUND_TEMPORAL_FRAME"\n',
            "",
        )
        phases = codegen._parse_phase_blocks(unrepresented_text)
        self.assertEqual(phases[2].binds, ["operator_identity", "temporal_frame"])

        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "mmuko-boot" / "pseudocode").mkdir(parents=True)
            (root / "tools" / "mmuko_codegen").mkdir(parents=True)
            (root / "MMUKO-OS.txt").write_text("spec\n", encoding="utf-8")
            primary = root / "mmuko-boot" / "pseudocode" / "mmuko-boot.psc"
            primary.write_text(unrepresented_text, encoding="utf-8")

            with self.assertRaises(SystemExit) as raised:
                codegen.generate(
                    root,
                    Path("MMUKO-OS.txt"),
                    Path("mmuko-boot/pseudocode/mmuko-boot.psc"),
                    Path("mmuko-boot/pseudocode"),
                )

            message = str(raised.exception)
            self.assertIn("BIND target has no handoff representation", message)
            self.assertIn("BIND operator_identity INTO handoff", message)
            self.assertIn("BIND temporal_frame INTO handoff", message)

    def test_kernel_entry_requirements_are_not_assigned_to_phase_runners(self):
        phases = codegen._parse_phase_blocks(PSC_TEXT)
        phase_requirements = [req for phase in phases for req in phase.requirements]
        self.assertNotIn('handoff.magic == "MMUKO"', phase_requirements)

        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "mmuko-boot" / "pseudocode").mkdir(parents=True)
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

            loader = (root / "kernel" / "mmuko_stage2_loader.c").read_text(encoding="utf-8")
            phase_runner_section = loader.split("int mmuko_verify_entry_contract", 1)[0]
            self.assertNotIn('REQUIRE handoff.magic == "MMUKO"', phase_runner_section)

    def test_repo_rel_normalizes_absolute_and_relative_inputs(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp).resolve()
            spec = root / "MMUKO-OS.txt"
            primary = root / "mmuko-boot" / "pseudocode" / "mmuko-boot.psc"
            primary.parent.mkdir(parents=True)
            spec.write_text("spec\n", encoding="utf-8")
            primary.write_text(PSC_TEXT, encoding="utf-8")

            self.assertEqual(codegen._repo_rel(root, spec), "MMUKO-OS.txt")
            self.assertEqual(codegen._repo_rel(root, Path("MMUKO-OS.txt")), "MMUKO-OS.txt")
            self.assertEqual(
                codegen._repo_rel(root, primary),
                "mmuko-boot/pseudocode/mmuko-boot.psc",
            )
            self.assertEqual(
                codegen._repo_rel(root, Path("mmuko-boot/pseudocode/mmuko-boot.psc")),
                "mmuko-boot/pseudocode/mmuko-boot.psc",
            )
            self.assertEqual(
                codegen._support_manifest(
                    [Path("mmuko-boot/pseudocode/mmuko-boot.psc")],
                    primary,
                    root,
                ),
                ["mmuko-boot/pseudocode/mmuko-boot.psc :: primary boot model"],
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
