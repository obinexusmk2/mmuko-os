from __future__ import annotations

import argparse
from dataclasses import dataclass, field
from enum import Enum
from typing import Iterable

from . import PerspectiveMembraneHandle, TRINARY_VALUES, drift_radial, tripartite_summary, trinary_compose, enzyme_apply


class KanbanTrack(str, Enum):
    FOUNDATION = "foundation"
    ASPIRATION = "aspiration"
    ADVERSARIAL = "adversarial"


class KanbanColumn(str, Enum):
    BACKLOG = "backlog"
    IN_PROGRESS = "in_progress"
    DONE = "done"
    BLOCKED = "blocked"


@dataclass
class KanbanTask:
    title: str
    track: KanbanTrack
    column: KanbanColumn = KanbanColumn.BACKLOG
    state: int = TRINARY_VALUES["maybe"]


@dataclass
class ConsoleBoard:
    membrane: PerspectiveMembraneHandle
    tasks: list[KanbanTask] = field(default_factory=list)

    def add_task(self, task: KanbanTask) -> None:
        if task.track is KanbanTrack.ASPIRATION and not self.membrane.track_b_unlocked:
            raise RuntimeError("Track B is locked until the membrane reaches PASS.")
        self.tasks.append(task)

    def render(self) -> str:
        snapshot = self.membrane.snapshot()
        lines = [
            "MMUKO-OS | NSIGII Python UI",
            f"Outcome: {snapshot['outcome_name'].upper()}  Δ={snapshot['discriminant']:.4f}  Track B={'OPEN' if snapshot['track_b_unlocked'] else 'LOCKED'}",
            "",
        ]
        for track in KanbanTrack:
            lines.append(f"[{track.value.upper()}]")
            entries = [task for task in self.tasks if task.track is track]
            if not entries:
                lines.append("  (empty)")
            else:
                for task in entries:
                    lines.append(f"  - {task.column.value:<11} {task.title} [{task.state}]")
            lines.append("")
        return "\n".join(lines).rstrip()


def _parse_trinary(value: str) -> int:
    key = value.strip().lower().replace('-', '_')
    if key not in TRINARY_VALUES:
        raise argparse.ArgumentTypeError(f"invalid trinary value: {value}")
    return TRINARY_VALUES[key]


def demo_board(tier1: int, tier2: int, w_actor: int) -> str:
    membrane = PerspectiveMembraneHandle()
    membrane.calibrate({1: tier1, 2: tier2})

    board = ConsoleBoard(membrane)
    board.add_task(KanbanTask("Physiological needs verified", KanbanTrack.FOUNDATION, KanbanColumn.DONE, tier1))
    board.add_task(KanbanTask("Safety scan completed", KanbanTrack.FOUNDATION, KanbanColumn.DONE, tier2))
    if membrane.track_b_unlocked:
        board.add_task(KanbanTask("Build OBINexus platform", KanbanTrack.ASPIRATION, KanbanColumn.BACKLOG, enzyme_apply(2, tier1)))
    board.add_task(KanbanTask("W-actor discriminant monitor", KanbanTrack.ADVERSARIAL, KanbanColumn.IN_PROGRESS, w_actor))

    tri = tripartite_summary(tier1, tier2, w_actor)
    drift = drift_radial((tier1, tier2, w_actor), (trinary_compose(tier1, tier2), tier2, w_actor))

    return "\n".join(
        [
            board.render(),
            "",
            f"Tripartite region: {tri['region_name']} ({tri['delta']:.4f})",
            f"Consensus: {tri['consensus_name']}",
            f"Radial drift: {drift:.4f}",
        ]
    )


def main(argv: Iterable[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Run the MMUKO-OS Python console UI.")
    parser.add_argument("--tier1", default="yes", type=_parse_trinary)
    parser.add_argument("--tier2", default="yes", type=_parse_trinary)
    parser.add_argument("--w-actor", default="yes", type=_parse_trinary)
    args = parser.parse_args(list(argv) if argv is not None else None)
    print(demo_board(args.tier1, args.tier2, args.w_actor))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
