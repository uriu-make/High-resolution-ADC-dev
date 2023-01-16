#include "downsampling.h"

void DOWNSAMPLING::sampling(double *t, double *v, int len) {
  static double t_0 = 0.0;
  static int sum = 0;

  memcpy(buf_t, &buf_t[len], sizeof(double) * len);
  memcpy(buf_v, &buf_v[len], sizeof(double) * len);
  memcpy(&buf_t[size - len], t, sizeof(double) * len);
  memcpy(&buf_v[size - len], v, sizeof(double) * len);
  if (size > sum) {
    sum += len;
  } else {
    t_0 = buf_t[0];

    for (int i = 0, j = 1, basepoint = 0; i < size; i++) {
      buf_t[i] = buf_t[i] - t_0;

      if (buf_t[i] >= rate * j) {
        for (int k = basepoint; k < i; k++) {
          sample[j - 1] += buf_v[k];
        }
        sample[j - 1] /= i - basepoint;
        basepoint = i;
      }
    }
  }
}