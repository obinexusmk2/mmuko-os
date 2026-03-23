from libc.stddef cimport size_t
from mmuko_os.heartfull_firmware cimport MaslowTier, TrinaryState, TemporalFrame, EnzymeOp

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
