from mmuko_os.heartfull_firmware cimport TrinaryState

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
