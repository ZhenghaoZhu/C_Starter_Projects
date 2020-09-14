#include <stdint.h>
#include <math.h>

#include "debug.h"
#include "goertzel.h"

void goertzel_init(GOERTZEL_STATE *gp, uint32_t N, double k) {
    // TO BE IMPLEMENTED
    gp->A = (2 * M_PI) * (k/N);
    gp->B = cos(gp->A) * 2;
    gp->k = k;
    gp->N = N;
    gp->s0 = 0;
    gp->s1 = 0;
    gp->s2 = 0;

    return;
}

void goertzel_step(GOERTZEL_STATE *gp, double x) {
    // TO BE IMPLEMENTED
    gp->s0 = gp->B * gp->s1 + x - gp->s2;
    gp->s2 = gp->s1;
    gp->s1 = gp->s0;

    return;

}

double goertzel_strength(GOERTZEL_STATE *gp, double x) {
    // TO BE IMPLEMENTED
    COMPLEX_PAIR constant_c;
    COMPLEX_PAIR constant_d;
    COMPLEX_PAIR y_complex_pair;
    double complete_y;
    double sqrd_mag_y;
    double y_A;
    double y_B;
    double y_C;
    double y_D;

    constant_c.real = cos(gp->A);
    constant_c.imag = -1.0  * sin(gp->A);

    constant_d.real = cos(gp->A * ((gp->N) - 1.0));
    constant_d.imag = -1.0  * sin(gp->A * ((gp->N) - 1.0));

    // printf("A: %lf B: %lf, C Real: %lf, C Imag: %lf, D Real: %lf, D Imag: %lf \n", gp->A, gp->B, constant_c.real, constant_c.imag, constant_d.real, constant_d.imag);
    gp->s0 = gp->B*gp->s1 + x - gp->s2;
    // printf("s0: %lf, s1: %lf \n", gp->s0, gp->s1);
    y_A = (-1.0 * gp->s1 * constant_c.real) + gp->s0;
    y_B = gp->s1 * constant_c.imag;
    y_C = constant_d.real;
    y_D = constant_d.imag;

    // printf("Y Real: %lf, Y Imag: %lf \n", y_complex_pair.real, y_complex_pair.imag);
    
    y_complex_pair.real = (y_A * y_C) - (y_B * y_D);
    y_complex_pair.imag = (y_A * y_D) + (y_B * y_C);

    // printf("Y Real: %lf, Y Imag: %lf \n", y_complex_pair.real, y_complex_pair.imag);

    sqrd_mag_y = pow(y_complex_pair.real, 2.0) + pow(y_complex_pair.imag, 2.0);

    complete_y = (2.0 * sqrd_mag_y)/(gp->N * gp->N);

    return complete_y;
}
