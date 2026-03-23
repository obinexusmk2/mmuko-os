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
        unsigned int pending_mask
        unsigned int violated_mask

    ctypedef struct PerspectiveMembrane:
        MembraneOutcome outcome
        TripointerScan scan
        QubitCompass compass
        MaslowNeedsState needs
        double discriminant
        unsigned int boot_phase

    ctypedef enum NSIGIIPhase:
        NSIGII_PHASE_N
        NSIGII_PHASE_S
        NSIGII_PHASE_I
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
