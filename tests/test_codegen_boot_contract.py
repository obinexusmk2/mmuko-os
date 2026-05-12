from pathlib import Path
import tempfile
import unittest

from tools.mmuko_codegen import generate as codegen


RAW_SPEC_TEXT = """
Status
------
The BIOS boot path uses a custom raw reserved-sector layout.
It is not FAT12, FAT32, or a defined "FAT64" format.

Design Decision
---------------
Therefore the disk layout is raw and fixed-position:

  LBA 0     : stage-1 boot sector (boot.asm)
  LBA 1-16  : stage-2 loader (mmuko-boot/stage2.asm)
  LBA 17-48 : native MMUKO runtime image (mmuko-boot/runtime.asm)
"""

RAW_PSC_TEXT = """
ENUM MMUKO_BOOT_OUTCOME:
    PASS  = 0xAA
    HOLD  = 0xBB
    ALERT = 0xCC

STRUCT MMUKO_BOOT_HANDOFF:
    magic : CHAR[5] = "MMUKO"
    revision : UINT16 = 0x0001
    filesystem_target : STRING = "RAW_FIXED_SECTOR:mmuko-os.img:LBA0_STAGE1:LBA1_16_STAGE2:LBA17_48_RUNTIME"

FUNC mmuko_boot() -> MMUKO_BOOT_HANDOFF:
    handoff = MMUKO_BOOT_HANDOFF()

    // Phase 1 — PHASE_NEED_STATE_INIT
    REQUIRE tier1_state != NO
    complete_phase(handoff, PHASE_NEED_STATE_INIT, 0x00000001)

    // Phase 2 — PHASE_SAFETY_SCAN
    REQUIRE tier2_state != NO
    complete_phase(handoff, PHASE_SAFETY_SCAN, 0x00000002)

    // Phase 3 — PHASE_IDENTITY_CALIBRATION
    complete_phase(handoff, PHASE_IDENTITY_CALIBRATION, 0x00000004)

    // Phase 4 — PHASE_GOVERNANCE_CHECK
    REQUIRE execution_policy == VERIFIED
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
"""

FAT_PSC_TEXT = RAW_PSC_TEXT.replace(
    "RAW_FIXED_SECTOR:mmuko-os.img:LBA0_STAGE1:LBA1_16_STAGE2:LBA17_48_RUNTIME",
    "FAT12:mmuko-os.img",
)


class CodegenBootContractTests(unittest.TestCase):
    def _write_inputs(self, root: Path, psc_text: str) -> None:
        (root / "mmuko-boot" / "pseudocode").mkdir(parents=True)
        (root / "tools" / "mmuko_codegen").mkdir(parents=True)
        (root / "MMUKO-OS.txt").write_text(RAW_SPEC_TEXT, encoding="utf-8")
        (root / "mmuko-boot" / "pseudocode" / "mmuko-boot.psc").write_text(psc_text, encoding="utf-8")

    def test_raw_boot_spec_generates_no_fat_identifiers_in_stage1(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self._write_inputs(root, RAW_PSC_TEXT)

            codegen.generate(
                root,
                Path("MMUKO-OS.txt"),
                Path("mmuko-boot/pseudocode/mmuko-boot.psc"),
                Path("mmuko-boot/pseudocode"),
            )

            stage1 = (root / "boot" / "mmuko_stage1_boot.asm").read_text(encoding="utf-8")
            self.assertNotIn("FAT12", stage1)
            self.assertNotIn("FAT32", stage1)
            self.assertNotIn("FAT64", stage1)
            self.assertIn("MMUKORAW", stage1)

    def test_raw_boot_spec_rejects_fat_oriented_filesystem_target(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self._write_inputs(root, FAT_PSC_TEXT)

            with self.assertRaisesRegex(SystemExit, "conflicts with MMUKO-OS.txt"):
                codegen.generate(
                    root,
                    Path("MMUKO-OS.txt"),
                    Path("mmuko-boot/pseudocode/mmuko-boot.psc"),
                    Path("mmuko-boot/pseudocode"),
                )


if __name__ == "__main__":
    unittest.main()
