#include <iostream>
#include <sys/time.h>
#include "ADS1256.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sched.h>
#include <queue>
#include <mutex>
#include <thread>

#define ADS1256_CLOCK 7680000
#define DRDY          17
#define RESET         18
#define SYNC          27

#define SAMPLENUM 20000

struct send_data {
  double volt;
  __u64 t;
};

struct buffer {
  int len;
  struct send_data data[SAMPLENUM];
};

ADS1256 ads1256("/dev/spidev0.0", "/dev/gpiochip0", DRDY, RESET, SYNC, ADS1256_CLOCK);

class DATA {
 private:
  std::mutex m;
  struct buffer buf = {-1, 0};
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
  void getADC();
  void write_socket(int sock);
  void read_socket(int sock);
};

DATA data;

void message() {
  std::cout << "socket is close" << std::endl;
}

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
  atexit(message);

  //初期化

  ads1256.open();        //デバイスを開く
  ads1256.init();        // GPIOなどを初期化
  ads1256.setVREF(2.5);  //基準電圧を2.5Vに設定
  ads1256.reset();       // ADS1256をリセット
  // usleep(50000);
  // ads1256.setClockOUT(CLOCK_OFF);         //外部クロック出力は使用しない
  // ads1256.setSampleRate(DATARATE_30000);  //サンプルレートを30kSPSに設定
  // ads1256.setAIN(AIN0, AGND);             //正をAIN6、負をAIN7に設定する
  // ads1256.setPGA(GAIN_1);                 // PGAのゲインを設定
  // usleep(1000000);
  // ads1256.selfCal();  // ADCの自動校正

  // ads1256.drdy_ready();    //校正の終了を待つ
  // ads1256.enable_event();  // drdyのイベント検出を有効化

  //ソケット作成
  int sock_listen = socket(AF_INET, SOCK_STREAM, 0);

  struct sockaddr_in server_addr = {0};
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(60000);
  bind(sock_listen, (struct sockaddr*)&server_addr, sizeof(server_addr));
  listen(sock_listen, 5);
  struct sockaddr_in client_addr;
  socklen_t len = sizeof(struct sockaddr_in);

  int sock;
  do {
    sock = accept(sock_listen, (struct sockaddr*)&client_addr, &len);
  } while (sock <= 0);
  printf("accepted.\n");
  // usleep(1000);

  // std::printf("t,volt\n");
  // std::fflush(stdout);
  // std::jthread adc{&DATA::getADC, &data};

  std::thread adc{&DATA::getADC, &data};
  std::thread socket_write{&DATA::write_socket, &data, sock};
  std::thread socket_read(&DATA::read_socket, &data, sock);
  adc.join();
  socket_write.join();
  // data.read_socket(sock);
  socket_read.join();
  ads1256.ADS1256_close();
  close(sock);
  close(sock_listen);
  return 0;
}

void DATA::getADC() {
  cpu_set_t cpu_set;
  int result;
  CPU_ZERO(&cpu_set);
  CPU_SET(3, &cpu_set);
  result = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpu_set);
  if (result != 0) {
    std::cerr << "ERROR" << std::endl;
    exit(0);
  }
  timeval time;
  double v;
  while (true) {
    if (run_measure) {
      // data[j].raw= ads1256.AnalogReadRawSync();  //同期をとり、セトリング・タイムを制御して測定
      v = ads1256.AnalogRead();  //同期を取らずにデータを取る
      gettimeofday(&time, NULL);

      m.lock();
      buf.len++;
      buf.data[buf.len].volt = v;
      buf.data[buf.len].t = time.tv_sec * 1000000 + time.tv_usec;
      m.unlock();
    }
  }
}

void DATA::write_socket(int sock) {
  std::this_thread::yield();
  cpu_set_t cpu_set;
  int result;
  CPU_ZERO(&cpu_set);
  CPU_SET(2, &cpu_set);
  result = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpu_set);
  if (result != 0) {
    std::cerr << "ERROR" << std::endl;
    exit(0);
  }
  while (true) {
    if (run_measure) {
      m.lock();
      if (buf.len >= 0) {
        write(sock, buf.data, sizeof(send_data) * (buf.len + 1));
        // for (int i = 0; i < buf.len + 1; i++) {
        //   std::cerr << std::to_string(buf.data[i].volt) << ":" << std::to_string(buf.len) << std::endl;
        // }
        // std::cerr << "end" << std::endl;
        buf.len = -1;
        memset(buf.data, 0, sizeof(buf.data));
      }
      m.unlock();
      usleep(1000);
    }
  }
}

void DATA::read_socket(int sock) {
  // std::this_thread::yield();
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
      m.lock();
      run_measure = false;
      m.unlock();

      if (com.kill) {
        exit(0);
      }

      ads1256.setSampleRate(com.rate);
      ads1256.setAIN(com.positive, com.negative);
      ads1256.setPGA(com.gain);
      ads1256.selfCal();  // ADCの自動校正

      ads1256.drdy_ready();  //校正の終了を待つ
      ads1256.disable_event();
      ads1256.gpio_reset();
      ads1256.enable_event();  // drdyのイベント検出を有効化
      m.lock();
      buf.len = -1;
      memset(buf.data, 0, sizeof(buf.data));
      run_measure = (bool)com.run;
      m.unlock();
    }
  }
}