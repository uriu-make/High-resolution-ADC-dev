#include "ADS1256.h"
#include "NUFFT.h"

#include <iostream>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <pthread.h>

#define ADS1256_CLOCK 7680000
#define DRDY          17
#define RESET         18
#define SYNC          27

#define SAMPLELEN 126

struct send_data {
  int32_t len;
  double volt[SAMPLELEN];
  int64_t t[SAMPLELEN];
};

ADS1256 ads1256("/dev/spidev0.0", "/dev/gpiochip0", DRDY, RESET, SYNC, ADS1256_CLOCK);

class APP {
 private:
  struct send_data buf = {-1};

  struct COMMAND {
    __u8 rate;
    __u8 gain;
    __u8 positive;
    __u8 negative;
    __u8 buf;
    __u8 sync;
    __u8 mode;
    __u8 run;
    __u8 kill = 0;
  };
  struct COMMAND com;
  bool run_measure = false;
  pthread_spinlock_t *spin;

 public:
  void set_pthread_spinlock_t(pthread_spinlock_t *spin) {
    this->spin = spin;
  }
  void getADC(void);
  void write_socket(int sock);
  void read_socket(int sock);
};

APP app;

int main() {
  pid_t pid;
  cpu_set_t cpu_set;
  int result;

  pid = getpid();
  CPU_ZERO(&cpu_set);
  CPU_SET(0, &cpu_set);

  result = sched_setaffinity(pid, sizeof(cpu_set_t), &cpu_set);
  if (result != 0) {
    std::cerr << "ERROR" << std::endl;
    exit(0);
  }

  //初期化
  pthread_spinlock_t lock;
  int pshared = PTHREAD_PROCESS_SHARED;
  pthread_spin_init(&lock, pshared);

  ads1256.open();        //デバイスを開く
  ads1256.init();        // GPIOなどを初期化
  ads1256.setVREF(2.5);  //基準電圧を2.5Vに設定
  ads1256.reset();       // ADS1256をリセット

  //ソケット作成
  int sock_listen = socket(AF_INET, SOCK_STREAM, 0);

  struct sockaddr_in server_addr = {0};
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(60000);
  bind(sock_listen, (struct sockaddr *)&server_addr, sizeof(server_addr));
  listen(sock_listen, 5);
  struct sockaddr_in client_addr;
  socklen_t len = sizeof(struct sockaddr_in);

  int sock;
  do {
    sock = accept(sock_listen, (struct sockaddr *)&client_addr, &len);
  } while (sock <= 0);
  close(sock_listen);
  app.set_pthread_spinlock_t(&lock);

  std::jthread adc{&APP::getADC, &app};
  std::jthread socket_write{&APP::write_socket, &app, sock};
  std::jthread socket_read(&APP::read_socket, &app, sock);

  adc.join();
  socket_write.join();
  socket_read.join();

  adc.request_stop();
  socket_write.request_stop();
  socket_read.request_stop();

  ads1256.ADS1256_close();
  close(sock);
  close(sock_listen);
  return 0;
}

void APP::getADC() {
  cpu_set_t cpu_set;
  int result;
  CPU_ZERO(&cpu_set);
  CPU_SET(3, &cpu_set);
  result = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpu_set);

  if (result != 0) {
    std::cerr << "ERROR" << std::endl;
    exit(0);
  }
  struct timeval time;
  while (!com.kill) {
    if (com.sync) {
      while (run_measure) {
        pthread_spin_lock(spin);
        buf.len++;
        buf.volt[buf.len] = ads1256.AnalogReadSync(&time);
        buf.t[buf.len] = time.tv_sec * 1000000 + time.tv_usec;
        pthread_spin_unlock(spin);
      }
    } else {
      while (run_measure) {
        pthread_spin_lock(spin);
        buf.len++;
        buf.volt[buf.len] = ads1256.AnalogRead();  //同期を取らずにデータを取る
        gettimeofday(&time, NULL);
        buf.t[buf.len] = time.tv_sec * 1000000 + time.tv_usec;
        pthread_spin_unlock(spin);
      }
    }
  }
}

void APP::write_socket(int sock) {
  cpu_set_t cpu_set;
  int result;
  CPU_ZERO(&cpu_set);
  CPU_SET(2, &cpu_set);
  result = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpu_set);
  if (result != 0) {
    std::cerr << "ERROR" << std::endl;
    exit(0);
  }

  double t_buf[1000];
  // double t[1000];
  double v[1000];
  // struct COMPLEX F[1000];
  int writepoint = 0;

  while (!com.kill) {
    if (run_measure) {
      if (com.mode == 0) {
        pthread_spin_lock(spin);
        if (buf.len > -1) {
          buf.len++;
          if (send(sock, &buf, sizeof(buf), 0) < 0) {
            com.kill = 1;
            pthread_spin_unlock(spin);
            break;
          }
          buf.len = -1;
        }
        pthread_spin_unlock(spin);
        std::this_thread::sleep_for(std::chrono::microseconds(5000));
      } else if (com.mode == 1) {
        pthread_spin_lock(spin);
        if (buf.len > -1) {
          buf.len++;
          writepoint += buf.len;
          memcpy(t_buf, &t_buf[buf.len], sizeof(int64_t) * (1000 - buf.len));
          memcpy(v, &v[buf.len], sizeof(double) * (1000 - buf.len));
          memcpy(&t_buf[1000 - buf.len], buf.t, sizeof(int64_t) * buf.len);
          memcpy(&v[1000 - buf.len], buf.volt, sizeof(double) * buf.len);
          buf.len = -1;
        }
        pthread_spin_unlock(spin);
        if (writepoint >= 1000) {
          // nudft(v, t, -2.0 * M_PI / 100, F, 100);
        }
      }
    }
  }
}

void APP::read_socket(int sock) {
  std::this_thread::yield();
  cpu_set_t cpu_set;
  int result;
  CPU_ZERO(&cpu_set);
  CPU_SET(1, &cpu_set);
  result = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpu_set);
  if (result != 0) {
    std::cerr << "ERROR" << std::endl;
    exit(0);
  }

  while (true) {
    int result = recv(sock, &com, sizeof(com), MSG_WAITALL);

    if (result > 0) {
      pthread_spin_lock(spin);
      run_measure = false;
      pthread_spin_unlock(spin);

      if (com.kill) {
        break;
      }

      ads1256.setSampleRate(com.rate);
      ads1256.setAIN(com.positive, com.negative);
      ads1256.setPGA(com.gain);
      ads1256.selfCal();  // ADCの自動校正

      ads1256.drdy_ready();  //校正の終了を待つ
      ads1256.disable_event();
      ads1256.gpio_reset();
      ads1256.enable_event();  // drdyのイベント検出を有効化
      pthread_spin_lock(spin);
      buf.len = -1;
      run_measure = (bool)com.run;
      pthread_spin_unlock(spin);
    }
  }
}