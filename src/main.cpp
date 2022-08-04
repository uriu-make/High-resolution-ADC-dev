#include <iostream>
#include <sys/time.h>
#include "ADS1256.h"

#define ADS1256_CLOCK 7680000
#define DRDY          17
#define RESET         18
#define SYNC          27

#define SAMPLENUM 10000

struct data {
  double adc;
  __u32 raw;
  struct timeval time;
  struct timeval rate;
};

int main() {
  //初期化
  struct data data[SAMPLENUM + 1] = {0};
  ADS1256 ads1256("/dev/spidev0.0", "/dev/gpiochip0", DRDY, RESET, SYNC, ADS1256_CLOCK);
  // double data[SAMPLENUM] = {0};
  ads1256.open();        //デバイスを開く
  ads1256.init();        // GPIOなどを初期化
  ads1256.setVREF(2.5);  //基準電圧を2.5Vに設定
  ads1256.reset();       // ADS1256をリセット
  usleep(50000);
  ads1256.setClockOUT(CLOCK_OFF);         //外部クロック出力は使用しない
  ads1256.setSampleRate(DATARATE_30000);  //サンプルレートを30kSPSに設定
  ads1256.setAIN(AIN6, AIN7);             //正をAIN6、負をAIN7に設定する
  ads1256.setPGA(GAIN_64);                 // PGAのゲインを設定
  usleep(1000000);
  ads1256.selfCal();  // ADCの自動校正

  ads1256.drdy_ready();    //校正の終了を待つ
  ads1256.enable_event();  // drdyのイベント検出を有効化
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < SAMPLENUM + 1; j++) {
      // data[j].adc = ads1256.AnalogReadSync();  //同期をとり、セトリング・タイムを制御して測定
      data[j].raw = ads1256.AnalogReadRaw();  //同期を取らずにデータを取る
      gettimeofday(&data[j].time, NULL);
    }
  }

  for (int i = 1; i < SAMPLENUM + 1; i++) {
    data[i].rate.tv_sec = data[i].time.tv_sec - data[i - 1].time.tv_sec;
    data[i].rate.tv_usec = data[i].time.tv_usec - data[i - 1].time.tv_usec;
  }
  data[0] = data[1];
  for (int i = 1; i < SAMPLENUM + 1; i++) {
    data[i].time.tv_sec = data[i].time.tv_sec - data[0].time.tv_sec;
    data[i].time.tv_usec = data[i].time.tv_usec - data[0].time.tv_usec;
  }

  for (int i = 0; i < SAMPLENUM + 1; i++) {
    data[i].adc = ads1256.convertVolt(data[i].raw);
  }

  std::printf("count,t,rate,volt,adc\n");
  std::fflush(stdout);
  for (int i = 1; i < SAMPLENUM + 1; i++) {
    std::printf("%d,%ld,%ld,%.8lf,%d\n",
                i,
                data[i].time.tv_sec * 1000000 + data[i].time.tv_usec,
                data[i].rate.tv_sec * 1000000 + data[i].rate.tv_usec,
                data[i].adc,
                data[i].raw);
  }

  ads1256.ADS1256_close();
  return 0;
}