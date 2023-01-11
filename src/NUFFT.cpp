#include "NUFFT.h"
#include <cblas64.h>

NUFFT::NUFFT(int m, int N, int q) {
  this->omega = std::polar(1.0, (2.0 * M_PI) / double(m * N));
  std::complex<double> F[(q + 1) * (q + 1)];

  invF = new std::complex<double>[(q + 1) * (q + 1)];
  S_j = new double[N];

  std::complex<double> tmp;
  this->m = m;
  this->N = N;
  this->q = q;

  for (int h = 0; h < q + 1; h++) {  // F(m,N,q)/N
    for (int w = 0; w < q + 1; w++) {
      if (h == w) {
        F[w + h * (q + 1)].real(double(N));
        F[w + h * (q + 1)].imag(0.0);
      } else {
        F[w + h * (q + 1)] = (std::pow(omega, (h - w) * N / 2.0) - std::pow(omega, (w - h) * N / 2.0)) /
                             /*------------------------------------------------------------------------*/
                             (1.0 - std::pow(omega, (w - h) * N / 2.0));
      }
    }
  }

  for (int h = 0; h < q + 1; h++) {  // 単位行列
    for (int w = 0; w < q + 1; w++) {
      if (w == h) {
        invF[w + h * (q + 1)].real(1.0);
        invF[w + h * (q + 1)].imag(0.0);
      }
    }
  }

  // 掃き出し法
  for (int row = 0; row < q + 1; row++) {
    tmp = F[row + row * (q + 1)];

    for (int colmn = 0; colmn < q + 1; colmn++) {
      F[colmn + row * (q + 1)] = F[colmn + row * (q + 1)] / tmp;
      invF[colmn + row * (q + 1)] = invF[colmn + row * (q + 1)] / tmp;
    }

    for (int i = 0; i < q + 1; i++) {
      if (row != i) {
        tmp = F[row + i * (q + 1)];
        for (int colmn = 0; colmn < q + 1; colmn++) {
          F[colmn + i * (q + 1)] = F[colmn + i * (q + 1)] - F[colmn + row * (q + 1)] * tmp;
          invF[colmn + i * (q + 1)] = invF[colmn + i * (q + 1)] - invF[colmn + row * (q + 1)] * tmp;
        }
      }
    }
  }

  for (int j = 0; j < N; j++) {
    S_j[j] = std::cos(M_PI * (j - N / 2) / static_cast<double>(m * N));
  }
}

const void NUFFT::nufft(double *alpha, double *omega, std::complex<double> *f) {
  std::complex<double> blas_alpha = {std::complex<double>(1.0, 0.0)}, blas_beta = {std::complex<double>(0.0, 0.0)};
  std::complex<double> X[N][(q + 1)];
  std::complex<double> X_sum[N] = {{0.0, 0.0}};

  std::complex<double> tau[m * N] = {std::complex<double>(0.0, 0.0)};
  const double omega_tmp = omega[0];
  double m_omega;
  int floor_m_omega_k[N];
  double dif_f_m_omega;
  // int l_array[m * N];

  for (int t = 0; t < N; t++) {  // すべてのx_j(omega_k)を計算
    omega[t] = omega[t] - omega_tmp;
    m_omega = m * omega[t];
    floor_m_omega_k[t] = static_cast<int>(std::floor(m_omega));
    dif_f_m_omega = m_omega - static_cast<double>(floor_m_omega_k[t]);

    std::complex<double> a[q + 1];     // x_j(omega_k)=F^{-1}*a
    for (int k = 0; k < q + 1; k++) {  // a_kの計算
      a[k] = std::complex<double>(0.0, 0.0);
      for (int j = -N / 2, i = 0; j < N / 2; j++, i++) {
        a[k] += std::polar(S_j[j + N / 2], 2.0 * M_PI * (dif_f_m_omega + q / 2 - k) * j);
      }
    }
    cblas_zgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, q + 1, 1, q + 1, &blas_alpha, invF, q + 1, a, 1, &blas_beta, &X[t], 1);
    for (int i = 0; i < q + 1; i++) {  // Xの総和を計算
      X_sum[t] += X[t][i];
    }
  }

  for (int j = 0; j < N; j++) {
    for (int k = 0; k < N; k++) {
      tau[floor_m_omega_k[k] + j] += alpha[k] * X_sum[j];
    }
  }
  NUFFT::fft(tau, m * N);
  for (int i = 0; i < N; i++) {
    f[i] = tau[i] / S_j[i];
  }
  return;
}

inline void NUFFT::fft(std::complex<double> *f, int N_orig) {
  int m = int(std::log2(N_orig));
  for (int s = 0; s < m; s++) {
    int div = int(std::pow(2, s));
    int N = N_orig / div;
    int n = N / 2;
    for (int j = 0; j < div; j++) {
      for (int i = 0; i < n; i++) {
        std::complex<double> W = std::polar(1.0, -2 * M_PI * i / N);
        std::complex<double> f_tmp = f[N * j + i] - f[N * j + n + i];
        f[N * j + i] += f[N * j + n + i];
        f[N * j + n + i] = W * f_tmp;
      }
    }
  }
  int i = 0;
  for (int j = 1; j < N_orig - 1; j++) {
    for (int k = N_orig >> 1; k > (i ^= k); k >>= 1)
      ;
    if (j < i) {
      std::complex<double> f_tmp = f[j];
      f[j] = f[i];
      f[i] = f_tmp;
    }
  }
}