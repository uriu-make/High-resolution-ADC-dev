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
  const double rate;

  double *buf_v;
  int64_t *buf_t;

 public:
  DOWNSAMPLING(int size, int N);
  ~DOWNSAMPLING();
  int sampling(int64_t *t, double *v, std::complex<double> *sample, int len);
  void fft(std::complex<double> *F, int len);
  int calc(int64_t *t, double *v, int len, std::complex<double> *F);
  void clear();
};
