from __future__ import annotations

from dataclasses import dataclass, field
from enum import IntEnum
from typing import Iterable

from importlib import import_module

_firmware = import_module("mmuko_os._firmware")


class TrinaryState(IntEnum):
    YES = _firmware.TRINARY_YES
    NO = _firmware.TRINARY_NO
    MAYBE = _firmware.TRINARY_MAYBE_VALUE
    MAYBE_NOT = _firmware.TRINARY_MAYBE_NOT


class MembraneOutcome(IntEnum):
    PASS = _firmware.MEMBRANE_PASS
    HOLD = _firmware.MEMBRANE_HOLD
    ALERT = _firmware.MEMBRANE_ALERT


class EnzymeOp(IntEnum):
    CREATE = _firmware.ENZYME_CREATE
    DESTROY = _firmware.ENZYME_DESTROY
    BUILD = _firmware.ENZYME_BUILD
    BREAK = _firmware.ENZYME_BREAK
    RENEW = _firmware.ENZYME_RENEW
    REPAIR = _firmware.ENZYME_REPAIR


class KanbanTrack(IntEnum):
    FOUNDATION_A = 0
    ASPIRATION_B = 1
    ADVERSARIAL_W = 2


class KanbanColumn(IntEnum):
    BACKLOG = 0
    IN_PROGRESS = 1
    DONE = 2
    BLOCKED = 3


class MPDAState(IntEnum):
    PREBOOT = _firmware.BZY_STATE_PREBOOT
    CALIBRATING = _firmware.BZY_STATE_CALIBRATING
    PENDING = _firmware.BZY_STATE_PENDING
    VIOLATED = _firmware.BZY_STATE_VIOLATED
    ESCALATED = _firmware.BZY_STATE_ESCALATED
    ACCEPTED = _firmware.BZY_STATE_ACCEPTED


class DiscriminantRegion(IntEnum):
    STABLE = _firmware.DISC_STABLE
    CRITICAL = _firmware.DISC_CRITICAL
    FAULT = _firmware.DISC_FAULT


@dataclass(slots=True)
class KanbanTask:
    title: str
    track: KanbanTrack
    state: TrinaryState = TrinaryState.MAYBE
    column: KanbanColumn = KanbanColumn.BACKLOG
    id: str | None = None
    pending_op: EnzymeOp | None = None


@dataclass(slots=True)
class HeartfullFirmware:
    boot_passed: bool
    tasks: list[KanbanTask] = field(default_factory=list)
    tier1: TrinaryState = TrinaryState.MAYBE
    tier2: TrinaryState = TrinaryState.MAYBE
    w_actor: TrinaryState = TrinaryState.MAYBE
    outcome: MembraneOutcome = MembraneOutcome.HOLD
    discriminant: float = 0.0
    native_snapshot: dict = field(default_factory=dict)

    @classmethod
    def create(cls, boot_passed_from_assembly: bool) -> "HeartfullFirmware | None":
        if not boot_passed_from_assembly:
            return None
        return cls(boot_passed=True)

    @staticmethod
    def trinary_compose(a: TrinaryState, b: TrinaryState) -> TrinaryState:
        return TrinaryState(_firmware.trinary_compose(int(a), int(b)))

    @staticmethod
    def apply_enzyme(op: EnzymeOp, current: TrinaryState) -> TrinaryState:
        return TrinaryState(_firmware.enzyme_apply(int(op), int(current)))

    @staticmethod
    def drift_radial(prev_scan: tuple[TrinaryState, TrinaryState, TrinaryState],
                     curr_scan: tuple[TrinaryState, TrinaryState, TrinaryState]) -> float:
        return float(_firmware.cpp_drift_radial(tuple(map(int, prev_scan)), tuple(map(int, curr_scan))))

    def run_nsigii(self, tier1: TrinaryState, tier2: TrinaryState,
                   w_actor: TrinaryState = TrinaryState.MAYBE) -> MembraneOutcome:
        self.tier1 = TrinaryState(tier1)
        self.tier2 = TrinaryState(tier2)
        self.w_actor = TrinaryState(w_actor)

        # Mirror the public C# orchestration while delegating primitive operations to native code.
        self.tier1 = self.apply_enzyme(EnzymeOp.RENEW, self.tier1)
        self.tier2 = self.apply_enzyme(EnzymeOp.BUILD, self.tier2)
        probe = self.trinary_compose(self.trinary_compose(self.tier1, self.tier2), TrinaryState.MAYBE)

        tri = _firmware.tripartite_summary(int(self.tier1), int(self.tier2), int(self.w_actor))
        self.discriminant = float(tri["delta"])
        self.native_snapshot = _firmware.membrane_snapshot(int(tier1), int(tier2))

        if probe == TrinaryState.NO:
            self.outcome = MembraneOutcome.HOLD
        elif self.tier1 == TrinaryState.NO or self.discriminant < 0.0:
            self.outcome = MembraneOutcome.ALERT
        elif self.tier1 == TrinaryState.MAYBE:
            self.outcome = MembraneOutcome.HOLD
        else:
            self.outcome = MembraneOutcome.PASS
        return self.outcome

    @property
    def track_b_open(self) -> bool:
        return self.outcome == MembraneOutcome.PASS

    def add_task(self, title: str, track: KanbanTrack,
                 state: TrinaryState = TrinaryState.MAYBE) -> KanbanTask:
        if track == KanbanTrack.ASPIRATION_B and not self.track_b_open:
            raise ValueError("Track B is locked until the membrane issues PASS")
        task = KanbanTask(title=title, track=track, state=state)
        self.tasks.append(task)
        return task

    def get_tasks(self, track: KanbanTrack) -> list[KanbanTask]:
        return [task for task in self.tasks if task.track == track]

    def process_maybe_state(self, state: TrinaryState, preferred_op: EnzymeOp) -> tuple[TrinaryState, EnzymeOp | None]:
        if state == TrinaryState.MAYBE_NOT:
            return (state, None)
        if state == TrinaryState.MAYBE:
            return (self.apply_enzyme(preferred_op, state), preferred_op)
        return (state, None)

    def mpda_snapshot(self, inputs: Iterable[TrinaryState], reverse_steps: int = 0) -> dict:
        return _firmware.mpda_snapshot([int(item) for item in inputs], self.discriminant, reverse_steps)


class HeartFeltFirmware:
    def __init__(self, firmware: HeartfullFirmware) -> None:
        if firmware.outcome != MembraneOutcome.PASS:
            raise ValueError(f"Cannot render UI when membrane outcome is {firmware.outcome.name}")
        self.firmware = firmware

    def render_board(self) -> str:
        lines = [
            "MMUKO-OS | NSIGII HeartFelt Compositor",
            f"Outcome: {self.firmware.outcome.name}",
            f"Discriminant: {self.firmware.discriminant:.4f}",
            f"Track B: {'UNLOCKED' if self.firmware.track_b_open else 'LOCKED'}",
        ]
        for track in KanbanTrack:
            label = track.name.replace('_', ' ')
            lines.append(f"[{label}]")
            tasks = self.firmware.get_tasks(track)
            if not tasks:
                lines.append("  (empty)")
                continue
            for task in tasks:
                lines.append(f"  - {task.title} :: {task.column.name} :: {task.state.name}")
        return "\n".join(lines)


def populate_kanban(firmware: HeartfullFirmware) -> None:
    t1 = firmware.add_task("Physiological needs verified", KanbanTrack.FOUNDATION_A, TrinaryState.YES)
    t1.column = KanbanColumn.DONE
    t2 = firmware.add_task("Safety scan completed", KanbanTrack.FOUNDATION_A, TrinaryState.YES)
    t2.column = KanbanColumn.DONE
    t3 = firmware.add_task("NSIGII 6-phase boot", KanbanTrack.FOUNDATION_A, TrinaryState.MAYBE)
    t3.column = KanbanColumn.IN_PROGRESS
    if firmware.track_b_open:
        b1 = firmware.add_task("Build OBINexus platform", KanbanTrack.ASPIRATION_B, TrinaryState.MAYBE)
        b1.column = KanbanColumn.BACKLOG
        b2 = firmware.add_task("Deploy CORN governance module", KanbanTrack.ASPIRATION_B, TrinaryState.MAYBE_NOT)
        b2.column = KanbanColumn.BACKLOG
    w1 = firmware.add_task("W-actor discriminant monitor", KanbanTrack.ADVERSARIAL_W, TrinaryState.MAYBE)
    w1.column = KanbanColumn.IN_PROGRESS


def parse_trinary(value: str) -> TrinaryState:
    normalized = value.strip().lower()
    if normalized in {"yes", "1", "+1"}:
        return TrinaryState.YES
    if normalized in {"no", "0"}:
        return TrinaryState.NO
    if normalized in {"maybenot", "maybe_not", "-2"}:
        return TrinaryState.MAYBE_NOT
    return TrinaryState.MAYBE
