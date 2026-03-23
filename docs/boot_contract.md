# MMUKO Boot Contract

`include/boot_contract.h` defines a fixed 52-byte loader-to-native handoff block at physical address `0x0500`. The contract is intended to be written by stage-1/stage-2 assembly and consumed by portable C/C++ boot or kernel logic. The assembly constants in `boot.asm` mirror the same offsets so stage-1 and native code agree on the layout.

## Contract summary

- **Magic/version**: validate that native code is reading a populated handoff block.
- **Boot flags**: advertise whether keyboard bootstrap was requested/present and whether the next hop is stage-2 or kernel-native C.
- **Keyboard buffer**: serializes BIOS `int 16h` input in a stable format for later C/C++ parsing.
- **NSIGII membrane outcome**: stores the on-wire membrane result byte (`PASS=0xAA`, `HOLD=0xBB`, `ALERT=0xCC`).
- **Loader-to-kernel transfer state**: indicates where control currently is in the boot pipeline.

## Physical layout at `0x0500`

| Offset | Size | Field | Meaning |
| --- | ---: | --- | --- |
| `0x00` | 2 | `magic` | `0x4D42` (`MB`) |
| `0x02` | 2 | `version` | current contract version |
| `0x04` | 2 | `total_size` | structure size in bytes |
| `0x06` | 2 | `boot_flags` | shared stage flags |
| `0x08` | 1 | `transfer_state` | stage-1/stage-2/kernel progress |
| `0x09` | 1 | `membrane_outcome` | `0xAA`, `0xBB`, or `0xCC` |
| `0x0A` | 1 | `membrane_phase` | latest NSIGII phase ordinal |
| `0x0C` | 2 | `native_entry_offset` | optional 16-bit native entry offset |
| `0x0E` | 2 | `native_entry_segment` | optional 16-bit native entry segment |
| `0x10` | 1 | `keyboard.length` | bytes captured |
| `0x11` | 1 | `keyboard.capacity` | maximum bytes (`32`) |
| `0x12` | 1 | `keyboard.last_scan_code` | last BIOS scan code returned by `int 16h` |
| `0x13` | 1 | `keyboard.flags` | bit0 indicates valid input |
| `0x14` | 32 | `keyboard.bytes` | ASCII payload copied from BIOS input |

## Boot flags

| Flag | Value | Meaning |
| --- | ---: | --- |
| `MMUKO_BOOT_FLAG_KEYBOARD_REQUIRED` | `1 << 0` | stage-1 or stage-2 must solicit keyboard bootstrap input |
| `MMUKO_BOOT_FLAG_KEYBOARD_PRESENT` | `1 << 1` | keyboard buffer contains user input |
| `MMUKO_BOOT_FLAG_NATIVE_C_READY` | `1 << 2` | native C/C++ entrypoint can safely consume the contract |
| `MMUKO_BOOT_FLAG_STAGE2_MODE` | `1 << 3` | portable logic is running in stage-2 context |
| `MMUKO_BOOT_FLAG_KERNEL_MODE` | `1 << 4` | portable logic is running in kernel context |
| `MMUKO_BOOT_FLAG_NSIGII_READY` | `1 << 5` | NSIGII membrane metadata is populated |

## Keyboard buffer format

The keyboard buffer is intentionally simple so it can be written from 16-bit assembly without parsing libraries:

1. `length`: number of bytes captured so far.
2. `capacity`: fixed maximum buffer size (`32`).
3. `last_scan_code`: most recent BIOS scan code.
4. `flags`: bit0 set once at least one byte has been written.
5. `bytes[32]`: ASCII payload, not necessarily NUL-terminated.

Stage-1 now uses BIOS `int 16h` to fill this block before transitioning to native logic.

## Membrane outcome location

The canonical NSIGII membrane byte is `contract->membrane_outcome` at offset `0x09` from the contract base. Assembly initializes it to `HOLD`, then updates it to `PASS` or `ALERT` as the NSIGII gate resolves.

## Loader-to-kernel transfer states

| State | Value | Meaning |
| --- | ---: | --- |
| `MMUKO_TRANSFER_RESET` | `0` | contract not initialized |
| `MMUKO_TRANSFER_STAGE1_READY` | `1` | stage-1 initialized the contract |
| `MMUKO_TRANSFER_KEYBOARD_DONE` | `2` | keyboard input serialized |
| `MMUKO_TRANSFER_STAGE2_READY` | `3` | portable stage-2 logic may begin |
| `MMUKO_TRANSFER_NATIVE_C_ENTRY` | `4` | assembly is about to hand off to C/C++ |
| `MMUKO_TRANSFER_KERNEL_ENTRY` | `5` | kernel has accepted the contract |

## Boot phase / file alignment

| Boot phase | Pseudocode section | Implementation |
| --- | --- | --- |
| Contract seeding + keyboard capture | stage-1 preamble before Phase N | `boot.asm` |
| Phase 0–6 portable execution | `mmuko-boot/pseudocode/mmuko-boot.psc` main boot sequence | `mmuko-boot/mmuko-boot.c` |
| Contract definition | shared preconditions for all phases | `include/boot_contract.h` |
