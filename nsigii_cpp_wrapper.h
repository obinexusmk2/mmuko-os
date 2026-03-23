#ifndef NSIGII_CPP_WRAPPER_H
#define NSIGII_CPP_WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

double nsigii_cpp_drift_radial(int a1, int b1, int g1,
                               int a2, int b2, int g2);
double nsigii_cpp_discriminant(int u, int v, int w);
int nsigii_cpp_compose(int a, int b);

#ifdef __cplusplus
}
#endif

#endif /* NSIGII_CPP_WRAPPER_H */
