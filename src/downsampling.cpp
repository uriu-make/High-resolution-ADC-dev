#include "downsampling.h"

DOWNSAMPLING::DOWNSAMPLING(int size, int N)
    : size(size), N(N), rate(1.0 / static_cast<double>(N)) {
  buf_t = new int64_t[size]{0};
  buf_v = new double[size]{0.0};
}

DOWNSAMPLING::~DOWNSAMPLING() {
}

int DOWNSAMPLING::sampling(int64_t *t, double *v, std::complex<double> *sample, int len) {
  static int sum = 0;
  int64_t tmp = 0;
  double ave_buf = 0.0;

  memmove(buf_t, &buf_t[len], sizeof(uint64_t) * (size - len));
  memmove(buf_v, &buf_v[len], sizeof(double) * (size - len));
  memcpy(&buf_t[size - len], t, sizeof(uint64_t) * len);
  memcpy(&buf_v[size - len], v, sizeof(double) * len);

  if (size > sum) {
    sum += len;
  } else {
    for (int i = 0, j = 0, count = 0; i < size && j < N; i++) {
      tmp = buf_t[i] - buf_t[0];
      if (tmp < 1000000 * rate * (static_cast<double>(j) + 1.0)) {
        ave_buf += buf_v[i];
        count++;
      } else if (count == 0) {
        return -1;
      } else {
        sample[j] = std::complex<double>(ave_buf / static_cast<double>(count), 0.0);
        count = 0;
        ave_buf = 0.0;
        j++;
        i--;
      }
    }
    return 0;
  }
  return -1;
}

void DOWNSAMPLING::fft(std::complex<double> *f, int len) {
  if (len > 1) {
    int n = len / 2;
    for (int i = 0; i < n; i++) {
      std::complex<double> W = std::polar(1.0, -2 * M_PI * i / static_cast<double>(len));
      std::complex<double> f_tmp = f[i] - f[n + i];
      f[i] += f[n + i];
      f[n + i] = W * f_tmp;
    }
    fft(&f[0], n);
    fft(&f[n], n);
    std::complex<double> F[len];
    for (int i = 0; i < n; i++) {
      F[2 * i] = f[i];
      F[2 * i + 1] = f[n + i];
    }
    for (int i = 0; i < len; i++) {
      f[i] = F[i];
    }
  }
}

int DOWNSAMPLING::calc(int64_t *t, double *v, int len, std::complex<double> *F) {
  if (sampling(t, v, F, len) == 0) {
    fft(F, N);
    return 0;
  } else {
    return -1;
  }
}

void DOWNSAMPLING::clear() {
  for (int i = 0; i < size; i++) {
    buf_t[i] = 0;
    buf_v[i] = 0.0;
  }
}
