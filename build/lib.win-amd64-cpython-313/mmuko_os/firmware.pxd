from libc.stdint cimport uint32_t
from libc.stddef cimport size_t

cdef extern from "heartfull_firmware.h":
    ctypedef enum TrinaryState:
        TRINARY_YES
        TRINARY_NO
        TRINARY_MAYBE
        TRINARY_MAYBE_NOT

    ctypedef enum MembraneOutcome:
        MEMBRANE_PASS
        MEMBRANE_HOLD
        MEMBRANE_ALERT

    ctypedef enum MaslowTier:
        MASLOW_TIER1_PHYSIOLOGICAL
        MASLOW_TIER2_SAFETY
        MASLOW_TIER3_BELONGING
        MASLOW_TIER4_ESTEEM
        MASLOW_TIER5_ACTUALISATION

    ctypedef struct TripointerScan:
        TrinaryState alpha
        TrinaryState beta
        TrinaryState gamma

    ctypedef enum TemporalFrame:
        TEMPORAL_THERE_AND_THEN
        TEMPORAL_HERE_AND_NOW
        TEMPORAL_WHEN_AND_WHERE

    ctypedef struct QubitCompass:
        TrinaryState ring1_past
        TrinaryState ring2_present
        TrinaryState ring3_future
        float theta_degrees
        float delta_theta

    ctypedef struct MaslowNeedsState:
        TrinaryState tier[6]
        bint tier1_satisfied
        bint tier2_satisfied
        uint32_t pending_mask
        uint32_t violated_mask

    ctypedef struct PerspectiveMembrane:
        MembraneOutcome outcome
        TripointerScan scan
        QubitCompass compass
        MaslowNeedsState needs
        double discriminant
        uint32_t boot_phase

    ctypedef enum NSIGIIPhase:
        NSIGII_PHASE_N
        NSIGII_PHASE_S
        NSIGII_PHASE_I1
        NSIGII_PHASE_G
        NSIGII_PHASE_I2
        NSIGII_PHASE_I3

    ctypedef enum EnzymeOp:
        ENZYME_CREATE
        ENZYME_DESTROY
        ENZYME_BUILD
        ENZYME_BREAK
        ENZYME_RENEW
        ENZYME_REPAIR

    ctypedef enum KanbanTrack:
        KANBAN_TRACK_A
        KANBAN_TRACK_B
        KANBAN_TRACK_W

    void membrane_init(PerspectiveMembrane *m)
    MembraneOutcome membrane_calibrate(PerspectiveMembrane *m, const MaslowNeedsState *needs)
    TrinaryState trinary_resolve(TrinaryState want, TrinaryState need, TrinaryState should)
    TrinaryState trinary_compose(TrinaryState a, TrinaryState b)
    TrinaryState enzyme_apply(EnzymeOp op, TrinaryState current)
    void compass_rotate(QubitCompass *c, float new_theta)
    bint kanban_track_b_unlocked(const PerspectiveMembrane *m)
    bint nsigii_run_phase(PerspectiveMembrane *m, NSIGIIPhase phase)
    double drift_radial(const TripointerScan *v_prev, const TripointerScan *v_curr)

cdef extern from "bzy_mpda.h":
    ctypedef enum BZY_State:
        BZY_STATE_PREBOOT
        BZY_STATE_CALIBRATING
        BZY_STATE_PENDING
        BZY_STATE_VIOLATED
        BZY_STATE_ESCALATED
        BZY_STATE_ACCEPTED

    ctypedef struct BZY_StackEntry:
        MaslowTier tier
        TrinaryState result
        TemporalFrame frame
        float theta

    ctypedef struct BZY_Stack:
        BZY_StackEntry entries[64]
        int top

    ctypedef struct BZY_Transition:
        BZY_State from_state
        TrinaryState input_symbol
        float theta_in
        BZY_State to_state
        float theta_out
        EnzymeOp enzyme_action

    ctypedef struct MPDA:
        BZY_State current_state
        BZY_Stack stack
        float theta
        BZY_Transition transitions[64]
        int n_transitions
        BZY_State q0
        bint stack_empty_accept
        double discriminant_floor

    void mpda_init(MPDA *m)
    bint mpda_push(MPDA *m, MaslowTier tier, TrinaryState result, TemporalFrame frame, float theta)
    bint mpda_pop(MPDA *m, BZY_StackEntry *out)
    bint mpda_peek(const MPDA *m, BZY_StackEntry *out)
    bint mpda_transition(MPDA *m, TrinaryState input)
    BZY_State mpda_run(MPDA *m, const TrinaryState *input_str, size_t len)
    bint mpda_accepts(const MPDA *m, double discriminant)
    bint mpda_reverse_read(MPDA *m, size_t steps, TrinaryState *out_buf)
    TemporalFrame mpda_theta_to_frame(float theta)
    void mpda_print_state(const MPDA *m)

cdef extern from "tripartite_discriminant.h":
    ctypedef struct TripartiteState:
        TrinaryState u_state
        TrinaryState v_state
        TrinaryState w_state
        double a
        double b
        double c

    ctypedef enum ByzantineConsensus:
        BYZ_ALL_AGREE
        BYZ_V_OR_W_FAILING
        BYZ_NOISE_DECOHERENT

    ctypedef enum DiscriminantRegion:
        DISC_STABLE
        DISC_CRITICAL
        DISC_FAULT

    double tripartite_discriminant(const TripartiteState *tri)
    DiscriminantRegion tripartite_classify(double delta)
    ByzantineConsensus tripartite_consensus(const TripartiteState *tri)
    TripartiteState tripartite_build(TrinaryState u, TrinaryState v, TrinaryState w)
    bint tripartite_fault_detected(const TripartiteState *tri)
    bint tripartite_roots(const TripartiteState *tri, double *root_pos, double *root_neg)
    DiscriminantRegion tripartite_check(TrinaryState u, TrinaryState v, TrinaryState w)
