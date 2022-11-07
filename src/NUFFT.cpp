#include "NUFFT.h"

void nudft(double *X, double *t, double freq_delta, struct COMPLEX *F, int len) {
  double omega;
  std::complex<double> F_buf;
  for (int k = 0; k < len; k++) {
    omega = freq_delta * (double)k;
    F_buf.real(0);
    F_buf.imag(0);
    for (int n = 0; n < len; n++) {
      F_buf += std::polar(X[n], omega * t[n]);
    }
    F[k].freq = double(omega);
    F[k].real = F_buf.real();
    F[k].image = F_buf.imag();
  }
}