# cython: language_level=3
from libc.stddef cimport size_t

from mmuko_os.heartfull_firmware cimport (
    PerspectiveMembrane,
    MaslowNeedsState,
    TripointerScan,
    TrinaryState,
    MembraneOutcome,
    EnzymeOp,
    MASLOW_TIER1_PHYSIOLOGICAL,
    MASLOW_TIER2_SAFETY,
    TRINARY_MAYBE,
    membrane_init,
    membrane_calibrate,
    trinary_resolve as c_trinary_resolve,
    trinary_compose as c_trinary_compose,
    enzyme_apply as c_enzyme_apply,
    kanban_track_b_unlocked,
    drift_radial as c_drift_radial,
)
from mmuko_os.bzy_mpda cimport (
    MPDA,
    BZY_State,
    mpda_init,
    mpda_run,
    mpda_accepts,
    mpda_reverse_read,
)
from mmuko_os.tripartite_discriminant cimport (
    TripartiteState,
    DiscriminantRegion,
    tripartite_build,
    tripartite_discriminant as c_tripartite_discriminant,
    tripartite_check as c_tripartite_check,
    tripartite_consensus as c_tripartite_consensus,
    tripartite_roots,
)
from mmuko_os.nsigii_cpp_wrapper cimport (
    nsigii_cpp_compose,
    nsigii_cpp_discriminant,
    nsigii_cpp_drift_radial,
)

TRINARY_YES = 1
TRINARY_NO = 0
TRINARY_MAYBE_VALUE = -1
TRINARY_MAYBE_NOT = -2
MEMBRANE_PASS = 0
MEMBRANE_HOLD = 1
MEMBRANE_ALERT = 2
ENZYME_CREATE = 0
ENZYME_DESTROY = 1
ENZYME_BUILD = 2
ENZYME_BREAK = 3
ENZYME_RENEW = 4
ENZYME_REPAIR = 5
BZY_STATE_PREBOOT = 0
BZY_STATE_CALIBRATING = 1
BZY_STATE_PENDING = 2
BZY_STATE_VIOLATED = 3
BZY_STATE_ESCALATED = 4
BZY_STATE_ACCEPTED = 5
DISC_STABLE = 1
DISC_CRITICAL = 0
DISC_FAULT = -1
BYZ_ALL_AGREE = 0
BYZ_V_OR_W_FAILING = 1
BYZ_NOISE_DECOHERENT = 2

cdef inline TrinaryState _as_trinary(int value):
    return <TrinaryState>value

cdef inline EnzymeOp _as_enzyme(int value):
    return <EnzymeOp>value

cdef list _copy_reverse_read(MPDA *mpda, size_t steps):
    cdef list result = []
    cdef TrinaryState buffer[64]
    cdef size_t i
    if steps > 64:
        steps = 64
    if mpda_reverse_read(mpda, steps, buffer):
        for i in range(steps):
            result.append(<int>buffer[i])
    return result

cpdef int trinary_compose(int a, int b):
    return <int>c_trinary_compose(_as_trinary(a), _as_trinary(b))

cpdef int trinary_resolve(int want, int need, int should):
    return <int>c_trinary_resolve(_as_trinary(want), _as_trinary(need), _as_trinary(should))

cpdef int enzyme_apply(int op, int current):
    return <int>c_enzyme_apply(_as_enzyme(op), _as_trinary(current))

cpdef double drift_radial(tuple prev_scan, tuple curr_scan):
    cdef TripointerScan prev
    cdef TripointerScan curr
    prev.alpha = _as_trinary(prev_scan[0])
    prev.beta = _as_trinary(prev_scan[1])
    prev.gamma = _as_trinary(prev_scan[2])
    curr.alpha = _as_trinary(curr_scan[0])
    curr.beta = _as_trinary(curr_scan[1])
    curr.gamma = _as_trinary(curr_scan[2])
    return c_drift_radial(&prev, &curr)

cpdef double cpp_drift_radial(tuple prev_scan, tuple curr_scan):
    return nsigii_cpp_drift_radial(
        int(prev_scan[0]), int(prev_scan[1]), int(prev_scan[2]),
        int(curr_scan[0]), int(curr_scan[1]), int(curr_scan[2]),
    )

cpdef int cpp_compose(int a, int b):
    return nsigii_cpp_compose(a, b)

cpdef double cpp_discriminant(int u, int v, int w):
    return nsigii_cpp_discriminant(u, v, w)

cpdef dict tripartite_summary(int u, int v, int w):
    cdef TripartiteState tri = tripartite_build(_as_trinary(u), _as_trinary(v), _as_trinary(w))
    cdef double delta = c_tripartite_discriminant(&tri)
    cdef double root_pos = 0.0
    cdef double root_neg = 0.0
    cdef bint has_roots = tripartite_roots(&tri, &root_pos, &root_neg)
    return {
        "u": u,
        "v": v,
        "w": w,
        "delta": delta,
        "region": <int>c_tripartite_check(_as_trinary(u), _as_trinary(v), _as_trinary(w)),
        "consensus": <int>c_tripartite_consensus(&tri),
        "roots": (root_pos, root_neg) if has_roots else None,
        "cpp_delta": nsigii_cpp_discriminant(u, v, w),
    }

cpdef dict membrane_snapshot(int tier1, int tier2):
    cdef PerspectiveMembrane membrane
    cdef MaslowNeedsState needs
    cdef int i
    membrane_init(&membrane)
    for i in range(6):
        needs.tier[i] = _as_trinary(TRINARY_MAYBE)
    needs.tier[<int>MASLOW_TIER1_PHYSIOLOGICAL] = _as_trinary(tier1)
    needs.tier[<int>MASLOW_TIER2_SAFETY] = _as_trinary(tier2)
    needs.tier1_satisfied = False
    needs.tier2_satisfied = False
    needs.pending_mask = 0
    needs.violated_mask = 0
    membrane_calibrate(&membrane, &needs)
    return {
        "outcome": <int>membrane.outcome,
        "discriminant": membrane.discriminant,
        "boot_phase": membrane.boot_phase,
        "track_b_open": bool(kanban_track_b_unlocked(&membrane)),
        "scan": (<int>membrane.scan.alpha, <int>membrane.scan.beta, <int>membrane.scan.gamma),
        "needs": {
            "tier1": <int>membrane.needs.tier[<int>MASLOW_TIER1_PHYSIOLOGICAL],
            "tier2": <int>membrane.needs.tier[<int>MASLOW_TIER2_SAFETY],
            "tier1_satisfied": bool(membrane.needs.tier1_satisfied),
            "tier2_satisfied": bool(membrane.needs.tier2_satisfied),
        },
    }

cpdef dict mpda_snapshot(object inputs, double discriminant=0.0, int reverse_steps=0):
    cdef MPDA machine
    cdef list values = [int(value) for value in inputs]
    cdef size_t length = len(values)
    cdef TrinaryState buffer[64]
    cdef size_t i
    mpda_init(&machine)
    if length > 64:
        raise ValueError("mpda_snapshot currently supports up to 64 inputs")
    for i, value in enumerate(values):
        buffer[i] = _as_trinary(value)
    mpda_run(&machine, buffer, length)
    return {
        "state": <int>machine.current_state,
        "theta": machine.theta,
        "stack_depth": machine.stack.top + 1,
        "accepted": bool(mpda_accepts(&machine, discriminant)),
        "reverse_read": _copy_reverse_read(&machine, reverse_steps),
    }
