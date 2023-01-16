#include <complex>
#include <string.h>
#pragma once

struct buffer {
  int32_t len;
  double volt[10000];
  int64_t t[10000];
};

class DOWNSAMPLING {
 private:
  int size;
  int N;
  double rate;
  double *buf_v;
  double *buf_t;
  double *sample;

 public:
  DOWNSAMPLING(int size, int N);
  ~DOWNSAMPLING();
  void sampling(double *t, double *v, int len);
  void fft();
};

DOWNSAMPLING::DOWNSAMPLING(int size, int N) {
  this->size = size;
  this->N = N;
  buf_t = new double[size];
  buf_v = new double[size];
  sample = new double[N];
  rate = 1.0 / static_cast<double>(N);
}

DOWNSAMPLING::~DOWNSAMPLING() {
}
