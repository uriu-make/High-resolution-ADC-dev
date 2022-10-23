#include <iostream>
#include <sys/time.h>
#include "ADS1256.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
// #include <mutex>
#include <thread>
#include <pthread.h>

#define ADS1256_CLOCK 7680000
#define DRDY          17
#define RESET         18
#define SYNC          27

#define SAMPLENUM 256

struct send_data {
  int32_t len;
  double volt[SAMPLENUM];
  int64_t t[SAMPLENUM];
};

ADS1256 ads1256("/dev/spidev0.0", "/dev/gpiochip0", DRDY, RESET, SYNC, ADS1256_CLOCK);

class DATA {
 private:
  // int len = -1;
  struct send_data buf = {-1};
  // double volt[SAMPLENUM] = {0};
  // int64_t t[SAMPLENUM] = {0};
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

 public:
  void getADC(pthread_spinlock_t *spin);
  void write_socket(int sock, pthread_spinlock_t *spin);
  void read_socket(int sock, pthread_spinlock_t *spin);
};

DATA data;

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

  std::jthread adc{&DATA::getADC, &data, &lock};
  std::jthread socket_write{&DATA::write_socket, &data, sock, &lock};
  std::thread socket_read(&DATA::read_socket, &data, sock, &lock);

  // struct sched_param param;
  // param.sched_priority = 0;
  // std::cerr << pthread_setschedparam(adc.native_handle(), SCHED_FIFO, &param) << std::endl;

  // param.sched_priority = 0;
  // std::cerr << pthread_setschedparam(socket_write.native_handle(), SCHED_FIFO, &param) << std::endl;

  adc.join();
  socket_write.join();
  socket_read.join();

  adc.request_stop();
  socket_write.request_stop();

  ads1256.ADS1256_close();
  close(sock);
  close(sock_listen);
  return 0;
}

void DATA::getADC(pthread_spinlock_t *spin) {
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
    switch (com.mode) {
      case 0:
        while (run_measure) {
          pthread_spin_lock(spin);
          buf.len++;
          if (com.sync) {
            buf.volt[buf.len] = ads1256.AnalogReadSync(&time);
          } else {
            buf.volt[buf.len] = ads1256.AnalogRead();  //同期を取らずにデータを取る
            gettimeofday(&time, NULL);
          }
          buf.t[buf.len] = time.tv_sec * 1000000 + time.tv_usec;
          pthread_spin_unlock(spin);
        }
        break;
    }
  }
}

void DATA::write_socket(int sock, pthread_spinlock_t *spin) {
  cpu_set_t cpu_set;
  int result;
  CPU_ZERO(&cpu_set);
  CPU_SET(2, &cpu_set);
  result = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpu_set);
  if (result != 0) {
    std::cerr << "ERROR" << std::endl;
    exit(0);
  }

  while (!com.kill) {
    if (run_measure) {
      pthread_spin_lock(spin);
      if (buf.len > -1) {
        buf.len++;
        send(sock, &buf, sizeof(buf), 0);

        // send(sock, &len, sizeof(int), MSG_MORE);
        // send(sock, volt, sizeof(double) * len, MSG_MORE);
        // send(sock, t, sizeof(int64_t) * len, MSG_MORE);
        // send(sock, buf, sizeof(send_data) * (len), 0);
        buf.len = -1;
      }
      pthread_spin_unlock(spin);
      std::this_thread::sleep_for(std::chrono::microseconds(1000));
    }
  }
}

void DATA::read_socket(int sock, pthread_spinlock_t *spin) {
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
    int result = read(sock, &com, sizeof(com));

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