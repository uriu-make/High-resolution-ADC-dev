#include <iostream>
#include "ADS1256.h"

#define ADS1256_CLOCK 7680000
#define DRDY          17
#define RESET         18
#define SYNC          27

#define SAMPLENUM 1000

int main() {
  //初期化
  ADS1256 ads1256("/dev/spidev0.0", "/dev/gpiochip0", DRDY, RESET, SYNC, ADS1256_CLOCK);
  double data[SAMPLENUM] = {0};
  ads1256.open();        //デバイスを開く
  ads1256.init();        // GPIOなどを初期化
  ads1256.setVREF(2.5);  //基準電圧を2.5Vに設定
  ads1256.reset();       // ADS1256をリセット
  usleep(50000);
  ads1256.setClockOUT(CLOCK_OFF);       //外部クロック出力は使用しない
  ads1256.setSampleRate(DATARATE_30000);  //サンプルレートを30kSPSに設定
  ads1256.setAIN(AIN6, AIN7);             //正をAIN6、負をAIN7に設定する
  ads1256.setPGA(GAIN_64);                // PGAのゲインを設定

  ads1256.selfCal();  // ADCの自動校正

  ads1256.drdy_ready();    //校正の終了を待つ
  ads1256.enable_event();  // drdyのイベント検出を有効化
  for (int i = 0; i < SAMPLENUM; i++) {
    data[i] = ads1256.AnalogReadSync();  //同期をとり、セトリング・タイムを制御して測定
    // data[i] = ads1256.AnalogRead();      //同期を取らずにデータを取る
  }

  for (int i = 0; i < SAMPLENUM; i++) {
    std::cout << data[i] << std::endl;
  }
  ads1256.ADS1256_close();
  return 0;
}