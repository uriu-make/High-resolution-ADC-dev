#include <complex>

#pragma once

struct COMPLEX {
  double freq;
  double real;
  double image;
};

void nudft(double *X, double *t, double freq_delta, struct COMPLEX *F, int len);