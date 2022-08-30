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

// #define SAMPLENUM 10000

struct ADC {
  double volt;
  struct timeval time;
};

struct send_data {
  double volt;
  __u64 t;
};

ADS1256 ads1256("/dev/spidev0.0", "/dev/gpiochip0", DRDY, RESET, SYNC, ADS1256_CLOCK);

class DATA {
 private:
  std::queue<struct ADC> q;
  std::mutex m;

 public:
  void getADC();
  void sendData(int sock);
};

DATA data;

int main() {
  pid_t pid;
  cpu_set_t cpu_set;
  int result;

  pid = getpid();
  CPU_ZERO(&cpu_set);
  CPU_SET(1, &cpu_set);

  result = sched_setaffinity(pid, sizeof(cpu_set_t), &cpu_set);
  if (result != 0) {
    std::cerr << "ERROR" << std::endl;
    exit(0);
  }

  // int sock_listen = socket(AF_INET, SOCK_STREAM, 0);

  // struct sockaddr_in server_addr = {0};
  // server_addr.sin_family = AF_INET;
  // server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  // server_addr.sin_port = htons(60000);
  // bind(sock_listen, (struct sockaddr*)&server_addr, sizeof(server_addr));
  // listen(sock_listen, 5);
  // struct sockaddr_in client_addr;
  // socklen_t len = sizeof(struct sockaddr_in);

  // int sock = accept(sock_listen, (struct sockaddr*)&client_addr, &len);
  // printf("accepted.\n");

  //初期化

  ads1256.open();        //デバイスを開く
  ads1256.init();        // GPIOなどを初期化
  ads1256.setVREF(2.5);  //基準電圧を2.5Vに設定
  ads1256.reset();       // ADS1256をリセット
  usleep(50000);
  ads1256.setClockOUT(CLOCK_OFF);         //外部クロック出力は使用しない
  ads1256.setSampleRate(DATARATE_30000);  //サンプルレートを30kSPSに設定
  ads1256.setAIN(AIN0, AGND);             //正をAIN6、負をAIN7に設定する
  ads1256.setPGA(GAIN_1);                 // PGAのゲインを設定
  usleep(1000000);
  ads1256.selfCal();  // ADCの自動校正

  ads1256.drdy_ready();    //校正の終了を待つ
  ads1256.enable_event();  // drdyのイベント検出を有効化

  int sock_listen = socket(AF_INET, SOCK_STREAM, 0);

  struct sockaddr_in server_addr = {0};
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(60000);
  bind(sock_listen, (struct sockaddr*)&server_addr, sizeof(server_addr));
  listen(sock_listen, 5);
  struct sockaddr_in client_addr;
  socklen_t len = sizeof(struct sockaddr_in);

  int sock = accept(sock_listen, (struct sockaddr*)&client_addr, &len);
  printf("accepted.\n");

  std::printf("t,volt\n");
  std::fflush(stdout);
  std::thread th0{&DATA::getADC, &data};
  std::thread th1{&DATA::sendData, &data, sock};

  th0.join();
  th1.join();

  ads1256.ADS1256_close();
  // close(sock);
  // close(sock_listen);
  return 0;
}

void DATA::getADC() {
  struct ADC buf;
  cpu_set_t cpu_set;
  int result;
  CPU_ZERO(&cpu_set);
  CPU_SET(3, &cpu_set);
  result = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpu_set);
  if (result != 0) {
    std::cerr << "ERROR" << std::endl;
    exit(0);
  }

  while (true) {
    // data[j].raw= ads1256.AnalogReadRawSync();  //同期をとり、セトリング・タイムを制御して測定
    buf.volt = ads1256.AnalogRead();  //同期を取らずにデータを取る
    gettimeofday(&buf.time, NULL);
    m.lock();
    q.push(ADC{buf.volt, buf.time});
    m.unlock();
  }
}

void DATA::sendData(int sock) {
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
    usleep(1000);
    m.lock();
    int size = q.size();
    if (size > 0) {
      struct ADC buf;
      struct send_data data[size];
      // m.lock();
      for (int i = 0; i < size; i++) {
        buf = q.front();
        q.pop();
        data[i].volt = buf.volt;
        data[i].t = buf.time.tv_sec * 1000000 + buf.time.tv_usec;
      }
      m.unlock();
      write(sock, &data, sizeof(data));
      // for (int i = 0; i < size; i++) {
      //   std::printf("%lld,%lf\n", data[i].t, data[i].volt);
      // }
    } else {
      m.unlock();
    }
  }
}