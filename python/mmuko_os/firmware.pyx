# cython: language_level=3
from cpython.mem cimport PyMem_Malloc, PyMem_Free
from .firmware cimport *

TRINARY_VALUES = {
    "yes": TRINARY_YES,
    "no": TRINARY_NO,
    "maybe": TRINARY_MAYBE,
    "maybe_not": TRINARY_MAYBE_NOT,
    "defer": TRINARY_MAYBE_NOT,
}

MEMBRANE_LABELS = {
    MEMBRANE_PASS: "pass",
    MEMBRANE_HOLD: "hold",
    MEMBRANE_ALERT: "alert",
}

BZY_STATE_LABELS = {
    BZY_STATE_PREBOOT: "preboot",
    BZY_STATE_CALIBRATING: "calibrating",
    BZY_STATE_PENDING: "pending",
    BZY_STATE_VIOLATED: "violated",
    BZY_STATE_ESCALATED: "escalated",
    BZY_STATE_ACCEPTED: "accepted",
}

DISCRIMINANT_LABELS = {
    DISC_STABLE: "stable",
    DISC_CRITICAL: "critical",
    DISC_FAULT: "fault",
}

CONSENSUS_LABELS = {
    BYZ_ALL_AGREE: "all_agree",
    BYZ_V_OR_W_FAILING: "v_or_w_failing",
    BYZ_NOISE_DECOHERENT: "noise_decoherent",
}

cdef TrinaryState _coerce_trinary(object value):
    cdef int ivalue
    if isinstance(value, str):
        key = value.strip().lower()
        if key not in TRINARY_VALUES:
            raise ValueError(f"unknown trinary state: {value!r}")
        return <TrinaryState>TRINARY_VALUES[key]
    ivalue = int(value)
    return <TrinaryState>ivalue

cdef EnzymeOp _coerce_enzyme(object value):
    cdef int ivalue = int(value)
    return <EnzymeOp>ivalue

cdef NSIGIIPhase _coerce_phase(object value):
    cdef int ivalue = int(value)
    return <NSIGIIPhase>ivalue

cdef MaslowTier _coerce_tier(object value):
    cdef int ivalue = int(value)
    return <MaslowTier>ivalue

cdef TemporalFrame _coerce_frame(object value):
    cdef int ivalue = int(value)
    return <TemporalFrame>ivalue

cdef dict _tripointer_to_dict(TripointerScan scan):
    return {
        "alpha": <int>scan.alpha,
        "beta": <int>scan.beta,
        "gamma": <int>scan.gamma,
    }

cdef dict _compass_to_dict(QubitCompass compass):
    return {
        "ring1_past": <int>compass.ring1_past,
        "ring2_present": <int>compass.ring2_present,
        "ring3_future": <int>compass.ring3_future,
        "theta_degrees": compass.theta_degrees,
        "delta_theta": compass.delta_theta,
    }

cdef dict _needs_to_dict(MaslowNeedsState needs):
    return {
        "tiers": [<int>needs.tier[i] for i in range(1, 6)],
        "tier1_satisfied": bool(needs.tier1_satisfied),
        "tier2_satisfied": bool(needs.tier2_satisfied),
        "pending_mask": needs.pending_mask,
        "violated_mask": needs.violated_mask,
    }

cdef dict _membrane_to_dict(PerspectiveMembrane membrane):
    return {
        "outcome": <int>membrane.outcome,
        "outcome_name": MEMBRANE_LABELS[membrane.outcome],
        "scan": _tripointer_to_dict(membrane.scan),
        "compass": _compass_to_dict(membrane.compass),
        "needs": _needs_to_dict(membrane.needs),
        "discriminant": membrane.discriminant,
        "boot_phase": membrane.boot_phase,
        "track_b_unlocked": bool(kanban_track_b_unlocked(&membrane)),
    }

cdef dict _stack_entry_to_dict(BZY_StackEntry entry):
    return {
        "tier": <int>entry.tier,
        "result": <int>entry.result,
        "frame": <int>entry.frame,
        "theta": entry.theta,
    }

cpdef int trinary_compose_py(object a, object b):
    return <int>trinary_compose(_coerce_trinary(a), _coerce_trinary(b))

cpdef int trinary_resolve_py(object want, object need, object should):
    return <int>trinary_resolve(_coerce_trinary(want), _coerce_trinary(need), _coerce_trinary(should))

cpdef int enzyme_apply_py(object op, object current):
    return <int>enzyme_apply(_coerce_enzyme(op), _coerce_trinary(current))

cpdef double drift_radial_py(tuple previous, tuple current):
    cdef TripointerScan prev_scan
    cdef TripointerScan curr_scan
    prev_scan.alpha = _coerce_trinary(previous[0])
    prev_scan.beta = _coerce_trinary(previous[1])
    prev_scan.gamma = _coerce_trinary(previous[2])
    curr_scan.alpha = _coerce_trinary(current[0])
    curr_scan.beta = _coerce_trinary(current[1])
    curr_scan.gamma = _coerce_trinary(current[2])
    return drift_radial(&prev_scan, &curr_scan)

cpdef dict tripartite_summary(object u, object v, object w):
    cdef TripartiteState tri = tripartite_build(_coerce_trinary(u), _coerce_trinary(v), _coerce_trinary(w))
    cdef double root_pos = 0.0
    cdef double root_neg = 0.0
    cdef bint has_roots = tripartite_roots(&tri, &root_pos, &root_neg)
    cdef double delta = tripartite_discriminant(&tri)
    cdef DiscriminantRegion region = tripartite_classify(delta)
    cdef ByzantineConsensus consensus = tripartite_consensus(&tri)
    return {
        "u": <int>tri.u_state,
        "v": <int>tri.v_state,
        "w": <int>tri.w_state,
        "a": tri.a,
        "b": tri.b,
        "c": tri.c,
        "delta": delta,
        "region": <int>region,
        "region_name": DISCRIMINANT_LABELS[region],
        "consensus": <int>consensus,
        "consensus_name": CONSENSUS_LABELS[consensus],
        "fault_detected": bool(tripartite_fault_detected(&tri)),
        "has_real_roots": bool(has_roots),
        "roots": (root_pos, root_neg) if has_roots else None,
    }

cdef class PerspectiveMembraneHandle:
    cdef PerspectiveMembrane _membrane

    def __cinit__(self):
        membrane_init(&self._membrane)

    def calibrate(self, tiers):
        cdef MaslowNeedsState needs
        cdef int i
        membrane_init(&self._membrane)
        for i in range(6):
            needs.tier[i] = TRINARY_MAYBE
        needs.tier1_satisfied = False
        needs.tier2_satisfied = False
        needs.pending_mask = 0
        needs.violated_mask = 0

        if isinstance(tiers, dict):
            for i in range(1, 6):
                if i in tiers:
                    needs.tier[i] = _coerce_trinary(tiers[i])
        else:
            for i, value in enumerate(tiers, start=1):
                if i > 5:
                    break
                needs.tier[i] = _coerce_trinary(value)

        outcome = membrane_calibrate(&self._membrane, &needs)
        return MEMBRANE_LABELS[outcome]

    def run_phase(self, phase):
        return bool(nsigii_run_phase(&self._membrane, _coerce_phase(phase)))

    def rotate_compass(self, theta):
        compass_rotate(&self._membrane.compass, float(theta))

    def snapshot(self):
        return _membrane_to_dict(self._membrane)

    @property
    def discriminant(self):
        return self._membrane.discriminant

    @property
    def outcome(self):
        return MEMBRANE_LABELS[self._membrane.outcome]

    @property
    def track_b_unlocked(self):
        return bool(kanban_track_b_unlocked(&self._membrane))

cdef class MPDAHandle:
    cdef MPDA _mpda

    def __cinit__(self):
        mpda_init(&self._mpda)

    def reset(self):
        mpda_init(&self._mpda)

    def push(self, tier, result, frame, theta):
        return bool(mpda_push(&self._mpda, _coerce_tier(tier), _coerce_trinary(result), _coerce_frame(frame), float(theta)))

    def pop(self):
        cdef BZY_StackEntry entry
        if not mpda_pop(&self._mpda, &entry):
            return None
        return _stack_entry_to_dict(entry)

    def peek(self):
        cdef BZY_StackEntry entry
        if not mpda_peek(&self._mpda, &entry):
            return None
        return _stack_entry_to_dict(entry)

    def transition(self, value):
        return bool(mpda_transition(&self._mpda, _coerce_trinary(value)))

    def run(self, values):
        cdef Py_ssize_t size = len(values)
        cdef TrinaryState *buffer = <TrinaryState *>PyMem_Malloc(size * sizeof(TrinaryState))
        cdef Py_ssize_t i
        if buffer == NULL and size != 0:
            raise MemoryError()
        try:
            for i in range(size):
                buffer[i] = _coerce_trinary(values[i])
            return BZY_STATE_LABELS[mpda_run(&self._mpda, buffer, size)]
        finally:
            PyMem_Free(buffer)

    def accepts(self, discriminant):
        return bool(mpda_accepts(&self._mpda, float(discriminant)))

    def reverse_read(self, steps):
        cdef Py_ssize_t n = int(steps)
        cdef TrinaryState *buffer = <TrinaryState *>PyMem_Malloc(n * sizeof(TrinaryState))
        cdef Py_ssize_t i
        if buffer == NULL and n != 0:
            raise MemoryError()
        try:
            if not mpda_reverse_read(&self._mpda, n, buffer):
                return []
            return [<int>buffer[i] for i in range(n)]
        finally:
            PyMem_Free(buffer)

    def snapshot(self):
        return {
            "state": BZY_STATE_LABELS[self._mpda.current_state],
            "theta": self._mpda.theta,
            "stack_depth": self._mpda.stack.top + 1,
            "stack_empty_accept": bool(self._mpda.stack_empty_accept),
            "discriminant_floor": self._mpda.discriminant_floor,
        }

__all__ = [
    "TRINARY_VALUES",
    "MEMBRANE_LABELS",
    "BZY_STATE_LABELS",
    "DISCRIMINANT_LABELS",
    "CONSENSUS_LABELS",
    "PerspectiveMembraneHandle",
    "MPDAHandle",
    "trinary_compose_py",
    "trinary_resolve_py",
    "enzyme_apply_py",
    "drift_radial_py",
    "tripartite_summary",
]
