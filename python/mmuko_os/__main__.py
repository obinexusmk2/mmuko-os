from __future__ import annotations

import argparse

from . import HeartFeltFirmware, HeartfullFirmware, TrinaryState, parse_trinary, populate_kanban


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="MMUKO-OS NSIGII Cython compositor")
    parser.add_argument("--boot-passed", default="false")
    parser.add_argument("--simulate-pass", action="store_true")
    parser.add_argument("--tier1", default="maybe")
    parser.add_argument("--tier2", default="maybe")
    parser.add_argument("--w-actor", default="maybe")
    return parser


def main() -> int:
    args = build_parser().parse_args()
    boot_passed = args.simulate_pass or args.boot_passed.lower() == "true"

    firmware = HeartfullFirmware.create(boot_passed)
    if firmware is None:
        print("[COMPOSITOR] BLOCKED — boot gate not passed.")
        return 1

    outcome = firmware.run_nsigii(
        parse_trinary(args.tier1),
        parse_trinary(args.tier2),
        parse_trinary(args.w_actor),
    )
    print(f"[COMPOSITOR] Outcome: {outcome.name}")
    print(f"[COMPOSITOR] Discriminant: {firmware.discriminant:.4f}")

    populate_kanban(firmware)

    prev_scan = (TrinaryState.MAYBE, TrinaryState.MAYBE, TrinaryState.MAYBE)
    curr_scan = (TrinaryState.YES, TrinaryState.MAYBE, TrinaryState.MAYBE)
    print(f"[DRIFT] radial={HeartfullFirmware.drift_radial(prev_scan, curr_scan):.4f}")
    print(f"[MPDA] {firmware.mpda_snapshot([parse_trinary(args.tier1), parse_trinary(args.tier2), parse_trinary(args.w_actor)], reverse_steps=3)}")

    if outcome.name == "PASS":
        print(HeartFeltFirmware(firmware).render_board())

    return 0 if outcome.name == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
