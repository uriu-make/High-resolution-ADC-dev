#include <complex>
#include <iostream>
#include <string.h>
#pragma once

struct buffer {
  int32_t len;
  double volt[10000];
  int64_t t[10000];
};

class DOWNSAMPLING {
 private:
  const int size;
  const int N;
  double *buf_v;
  double *buf_t;
  double rate;

 public:
  DOWNSAMPLING(int size, int N);
  ~DOWNSAMPLING();
  void sampling(double *t, double *v, std::complex<double> *sample, int len);
  void fft(std::complex<double> *F, int len);
  void calc(double *t, double *v, int len, std::complex<double> *F);
};
