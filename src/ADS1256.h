#include <sys/time.h>
#include <fcntl.h>
#include <linux/gpio.h>
#include <linux/spi/spidev.h>
#include <math.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string>

#include "spi.h"
#include "gpio.h"

#pragma once

//サンプリングレート
#define DATARATE_30000 0b11110000
#define DATARATE_15000 0b11100000
#define DATARATE_7500  0b11010000
#define DATARATE_3750  0b11000000
#define DATARATE_2000  0b10110000
#define DATARATE_1000  0b10100001
#define DATARATE_500   0b10010010
#define DATARATE_100   0b10000010
#define DATARATE_60    0b01110010
#define DATARATE_50    0b01100011
#define DATARATE_30    0b01010011
#define DATARATE_25    0b01000011
#define DATARATE_15    0b00110011
#define DATARATE_10    0b00100011
#define DATARATE_5     0b00010011
#define DATARATE_2_5   0b00000011

// PGAゲイン
#define GAIN_1  0b000
#define GAIN_2  0b001
#define GAIN_4  0b010
#define GAIN_8  0b011
#define GAIN_16 0b100
#define GAIN_32 0b101
#define GAIN_64 0b110

//クロック出力
#define CLOCK_OFF   0b00
#define CLOCK_DIV_1 0b01
#define CLOCK_DIV_2 0b10
#define CLOCK_DIV_4 0b11

//開放/短絡センサー検出
#define SDC_OFF   0b00
#define SDC_500nA 0b01
#define SDC_2uA   0b10
#define SDC_10uA  0b11

// GPIOモード
#define OUTPUT 0
#define INPUT  1

#define HIGH 1
#define LOW  0

#define AIN0 0b0000
#define AIN1 0b0001
#define AIN2 0b0010
#define AIN3 0b0011
#define AIN4 0b0100
#define AIN5 0b0101
#define AIN6 0b0110
#define AIN7 0b0111
#define AGND 0b1000

class ADS1256 : private SPI, private GPIO {
 private:
  std::string spidev;
  std::string gpiochip;
  int CLOCK;         // ADS1256に接続された水晶振動子の周波数
  __u8 GAIN = 1;     // PGAのゲイン
  double VREF;       //接続された基準電圧
  __u16 delay_sclk;  //連続した通信に挟む待ち時間
  __u32 drdy_pin;
  __u32 reset_pin;
  __u32 sync_pin;

 public:
  ADS1256(void);
  ADS1256(const char spidev[], const char gpiodev[], __u32 DRDY, __u32 RESET, __u32 SYNC, int clock);
  int open(const char spidev[], const char gpiodev[]);
  int open(void);
  int init(__u32 DRDY, __u32 RESET, __u32 SYNC);
  int init(void);

  int ReadReg(__u8 reg, __u8 *value);  //指定レジスタの読み取り
  int WriteReg(__u8 reg, __u8 value);  //指定レジスタに書き込み

  void reset(void);

  int setClock(int clock);  // ADS1256に入力される水晶振動子の周波数から、その他の周波数を設定する
  int setClock(void);       // ADS1256に入力される水晶振動子の周波数から、その他の周波数を設定する

  void setVREF(double vref);                 //リファレンス電圧の設定
  int setAnalogBuffer(bool buf);             //アナログバッファの有効/無効
  int setSampleRate(__u8 rate);              //サンプリングレートの設定
  int setPGA(__u8 gain);                     //プログラマブルゲインアンプの設定
  int setAIN(__u8 positive, __u8 negative);  //アナログ入力のピンを設定
  int setClockOUT(__u8 mode);                //クロック出力を設定
  int setSDC(__u8 mode);                     //開放/短絡センサー検出の設定

  int pinMode(__u8 pin, __u8 mode);        // GPIOピンのモードを設定
  int digitalRead(__u8 pin);               // GPIO入力
  int digitalWrite(__u8 pin, __u8 value);  // GPIO出力

  int enable_event();       //イベント検出を有効化
  int disable_event();      //イベント検出を無効化
  int selfCal(void);        //セルフキャリブレーション
  double AnalogRead(void);  // ADCの値を電圧で返す
  int AnalogReadRaw(void);  // ADCの値を生で返す

  double AnalogReadSync(struct timeval *t);  // ADCの値を電圧で返す
  int AnalogReadRawSync(struct timeval *t);  // ADCの値を生で返す
  double convertVolt(int raw);               //生データを電圧に変換
  void ADS1256_close(void);

  void gpio_reset(void);
  void drdy_ready(void);
};
