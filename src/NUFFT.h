#include <complex>

#pragma once

struct COMPLEX {
  double freq;
  double real;
  double image;
};

class NUFFT {
 private:
  int m, N, q;
  std::complex<double> omega;
  std::complex<double> *invF;  // F(m,N,q)
  std::complex<double> *S_j;  // F(m,N,q)

  inline void fft(std::complex<double> *f, int N_orig);

 public:
  NUFFT(int m, int N, int q);
  const void nufft(double *alpha, double *omega);
};