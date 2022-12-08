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
  std::complex<double> *alpha_k;
  std::complex<double> *omega_k;
  std::complex<double> *tau;
  std::complex<double> *f_j;
  const std::complex<double> *invF;  // F(m,N,q)
  inline void fft(std::complex<double> *f, int N_orig);

 public:
  NUFFT(int m, int N, int q);
  void nufft(double *alpha, double *omega);
};