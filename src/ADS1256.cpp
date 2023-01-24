#include <fcntl.h>
#include <linux/gpio.h>
#include <linux/spi/spidev.h>
#include <math.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <time.h>
#include <iostream>
#include <inttypes.h>

#include "spi.h"
#include "gpio.h"
#include "ADS1256.h"

ADS1256::ADS1256(void) {}

ADS1256::ADS1256(const char spidev[], const char gpiodev[], __u32 DRDY, __u32 RESET, __u32 SYNC, int clock) {
  this->spidev = std::string(spidev);
  this->gpiochip = std::string(gpiochip);
  this->drdy_pin = DRDY;
  this->reset_pin = RESET;
  this->sync_pin = SYNC;
  this->CLOCK = clock;
}

int ADS1256::open(const char spidev[], const char gpiodev[]) {
  if (ADS1256::spi_open(spidev) <= 0) {
    std::cerr << std::string(spidev) + " is not open." << std::endl;
    return -1;
  }
  if (ADS1256::gpio_open(gpiodev) <= 0) {
    std::cerr << std::string(gpiochip) + " is not open." << std::endl;
    return -1;
  }
  return 1;
}

int ADS1256::open(void) {
  if (ADS1256::spi_open(spidev.c_str()) <= 0) {
    std::cerr << spidev + " is not open." << std::endl;
    return -1;
  }
  if (ADS1256::gpio_open(gpiochip.c_str()) <= 0) {
    std::cerr << gpiochip + " is not open." << std::endl;
    return -1;
  }
  return 1;
}

int ADS1256::init(void) {
  ADS1256::gpio_set_default_flag(GPIO_V2_LINE_FLAG_OUTPUT);
  ADS1256::gpio_set_attrs_output_values(0);
  ADS1256::gpio_set_attrs_flag(1, GPIO_V2_LINE_FLAG_INPUT | GPIO_V2_LINE_FLAG_EDGE_FALLING);
  ADS1256::gpio_attrs_add_pin(reset_pin, 0, HIGH);
  ADS1256::gpio_attrs_add_pin(sync_pin, 0, HIGH);
  ADS1256::gpio_attrs_add_pin(drdy_pin, 1);

  if (ADS1256::gpio_init(1) < 0) {
    return -1;
  }
  if (ADS1256::spi_mode(SPI_MODE_1) < 0) {
    return -1;
  }
  if (ADS1256::setClock() < 0) {
    return -1;
  }
  return 0;
}

// 指定レジスタの読み取り
int ADS1256::ReadReg(__u8 reg, __u8 *value) {
  __u8 tx[2];
  struct spi_ioc_transfer arg[2] = {0};

  tx[0] = 0b00010000 | (reg & 0b00001111);
  tx[1] = 0;

  arg[0].tx_buf = reinterpret_cast<uintptr_t>(&tx);
  arg[0].rx_buf = 0;
  arg[0].len = 2;
  arg[0].delay_usecs = delay_sclk;
  arg[0].speed_hz = speed;
  arg[0].bits_per_word = 8;
  arg[0].cs_change = 0;

  arg[1].tx_buf = 0;
  arg[1].rx_buf = reinterpret_cast<uintptr_t>(value);
  arg[1].len = 1;
  arg[1].delay_usecs = 0;
  arg[1].speed_hz = speed;
  arg[1].bits_per_word = 8;
  arg[1].cs_change = 0;

  return ADS1256::spi_transfer(arg, 2);
}

// 指定レジスタに書き込み
int ADS1256::WriteReg(__u8 reg, __u8 value) {
  __u8 tx[2];
  struct spi_ioc_transfer arg[2] = {0};

  tx[0] = 0b01010000 | (reg & 0b00001111);
  tx[1] = 0;

  arg[0].tx_buf = reinterpret_cast<uintptr_t>(tx);
  arg[0].rx_buf = 0;
  arg[0].len = 2;
  arg[0].delay_usecs = delay_sclk;
  arg[0].speed_hz = speed;
  arg[0].bits_per_word = 8;
  arg[0].cs_change = 0;

  arg[1].tx_buf = reinterpret_cast<uintptr_t>(&value);
  arg[1].rx_buf = 0;
  arg[1].len = 1;
  arg[1].delay_usecs = 0;
  arg[1].speed_hz = speed;
  arg[1].bits_per_word = 8;
  arg[1].cs_change = 0;

  return ADS1256::spi_transfer(arg, 2);
}

void ADS1256::reset(void) {
  ADS1256::gpio_write(reset_pin, LOW);
  usleep(5 * 1000000 / CLOCK);
  ADS1256::gpio_write(reset_pin, HIGH);
}

// ADS1256に入力される水晶振動子の周波数から、その他の周波数を設定する
int ADS1256::setClock(int clock) {
  CLOCK = clock;
  delay_sclk = 50 * 1000000 / CLOCK;
  return ADS1256::spi_speed(ADS1256::CLOCK / 4);
}

int ADS1256::setClock(void) {
  delay_sclk = 50 * 1000000 / CLOCK;
  return ADS1256::spi_speed(ADS1256::CLOCK / 4);
}

// リファレンス電圧の設定
void ADS1256::setVREF(double vref) {
  VREF = vref;
}

// アナログバッファの有効/無効
int ADS1256::setAnalogBuffer(bool buf) {
  __u8 reg, value;
  reg = 0b00000000;
  if (ADS1256::ReadReg(reg, &value) <= 0) {
    return -1;
  }

  if (buf == true) {
    value = (value & 0b11111100) | 0b00000010;
  } else {
    value = (value & 0b11111100) | 0b00000000;
  }
  return ADS1256::WriteReg(reg, value);
}

// サンプリングレートの設定
int ADS1256::setSampleRate(__u8 rate) {
  __u8 reg = 0b00000011;
  return ADS1256::WriteReg(reg, rate);
}

// プログラマブルゲインアンプの設定
int ADS1256::setPGA(__u8 gain) {
  __u8 reg, value;
  reg = 0b0000010;

  if (ADS1256::ReadReg(reg, &value) <= 0) {
    return -1;
  }

  value = (value & 0b01111000) | (gain & 0b00000111);
  if (gain > GAIN_64) {
    gain = GAIN_64;
  }
  GAIN = pow(2, gain);
  return ADS1256::WriteReg(reg, value);
}

// アナログ入力のピンを設定
int ADS1256::setAIN(__u8 positive, __u8 negative) {
  __u8 reg, ain;
  reg = 0b00000001;
  ain = ((positive << 4) & 0b11110000) | (negative & 0b00001111);
  return ADS1256::WriteReg(reg, ain);
}

// クロック出力を設定
int ADS1256::setClockOUT(__u8 mode) {
  __u8 reg, value;
  reg = 0b0000010;

  if (ADS1256::ReadReg(reg, &value) <= 0) {
    return -1;
  }
  value = (value & 0b00011111) | ((mode << 5) & 0b01100000);
  return ADS1256::WriteReg(reg, value);
}

// 開放/短絡センサー検出の設定
int ADS1256::setSDC(__u8 mode) {
  __u8 reg, value;
  reg = 0b0000010;

  if (ADS1256::ReadReg(reg, &value) <= 0) {
    return -1;
  }
  value = (value & 0b01100111) | ((mode << 3) & 0b00011000);
  return ADS1256::WriteReg(reg, value);
}

// GPIOピンのモードを設定
int ADS1256::pinMode(__u8 pin, __u8 mode) {
  __u8 reg, value;
  reg = 0b00000100;

  if (ADS1256::ReadReg(reg, &value) <= 0) {
    return -1;
  }
  if (mode == OUTPUT) {
    value = value & ~(1 << (pin + 4));
  } else {
    value = value | (1 << (pin + 4));
  }
  return ADS1256::WriteReg(reg, value);
}

// GPIO入力
int ADS1256::digitalRead(__u8 pin) {
  __u8 reg, value;
  reg = 0b00000100;

  if (ADS1256::ReadReg(reg, &value) <= 0) {
    return -1;
  }
  return 0b00000001 & (value >> pin);
}

// GPIO出力
int ADS1256::digitalWrite(__u8 pin, __u8 value) {
  __u8 reg, buf;
  reg = 0b00000100;

  if (ADS1256::ReadReg(reg, &buf) <= 0) {
    return -1;
  }

  if (value == HIGH) {
    buf = buf | (1 << (pin));
  } else {
    buf = buf & ~(1 << (pin));
  }
  return ADS1256::WriteReg(reg, buf);
}

int ADS1256::enable_event() {
  return ADS1256::gpio_reconfig(2);
}

int ADS1256::disable_event() {
  return ADS1256::gpio_reconfig(1);
}

// セルフキャリブレーション
int ADS1256::selfCal(void) {
  __u8 tx = 0b11110000;
  struct spi_ioc_transfer arg = {0};

  arg.tx_buf = reinterpret_cast<uintptr_t>(&tx);
  arg.rx_buf = 0;
  arg.len = 1;
  arg.delay_usecs = 0;
  arg.speed_hz = speed;
  arg.bits_per_word = 8;
  arg.cs_change = 0;
  return ADS1256::spi_transfer(&arg, 1);
}

// ADCの値を電圧で返す
double ADS1256::AnalogRead(void) {
  return ADS1256::convertVolt(ADS1256::AnalogReadRaw());
}

// ADCの値を生で返す
int ADS1256::AnalogReadRaw(void) {
  struct spi_ioc_transfer arg[2] = {0};
  struct gpio_v2_line_event event;
  __u8 tx = 0b00000001;
  __u8 rx[3] = {0};

  arg[0].tx_buf = reinterpret_cast<uintptr_t>(&tx);
  arg[0].rx_buf = 0;
  arg[0].len = 1;
  arg[0].delay_usecs = delay_sclk;
  arg[0].speed_hz = speed;
  arg[0].bits_per_word = 8;
  arg[0].cs_change = 0;

  arg[1].tx_buf = 0;
  arg[1].rx_buf = reinterpret_cast<uintptr_t>(rx);
  arg[1].len = 3;
  arg[1].delay_usecs = 0;
  arg[1].speed_hz = speed;
  arg[1].bits_per_word = 8;
  arg[1].cs_change = 0;

  // ADS1256::gpio_reset();
  // ADS1256::enable_event();
  ADS1256::gpio_get_event(&event);
  ADS1256::spi_transfer(arg, 2);
  // ADS1256::disable_event();
  return (rx[0] << 16) | (rx[1] << 8) | rx[2];
}

double ADS1256::AnalogReadSync(struct timeval *t) {
  return ADS1256::convertVolt(ADS1256::AnalogReadRawSync(t));
}

// ADCの値を生で返す
int ADS1256::AnalogReadRawSync(struct timeval *t) {
  struct spi_ioc_transfer arg[2] = {0};
  struct gpio_v2_line_event event;
  __u8 tx = 0b00000001;
  __u8 rx[3] = {0};

  arg[0].tx_buf = reinterpret_cast<uintptr_t>(&tx);
  arg[0].rx_buf = 0;
  arg[0].len = 1;
  arg[0].delay_usecs = delay_sclk;
  arg[0].speed_hz = speed;
  arg[0].bits_per_word = 8;
  arg[0].cs_change = 0;

  arg[1].tx_buf = 0;
  arg[1].rx_buf = reinterpret_cast<uintptr_t>(rx);
  arg[1].len = 3;
  arg[1].delay_usecs = 0;
  arg[1].speed_hz = speed;
  arg[1].bits_per_word = 8;
  arg[1].cs_change = 0;

  ADS1256::gpio_write(sync_pin, LOW);
  usleep(4 * 1000000 / CLOCK);
  gettimeofday(t, NULL);
  ADS1256::gpio_write(sync_pin, HIGH);
  ADS1256::gpio_reset();
  ADS1256::enable_event();
  ADS1256::gpio_get_event(&event);
  ADS1256::spi_transfer(arg, 2);
  ADS1256::disable_event();
  return (rx[0] << 16) | (rx[1] << 8) | rx[2];
}

// 生データを電圧に変換
double ADS1256::convertVolt(int raw) {
  if ((raw & _BITULL(23)) > 0) {
    return -1 * static_cast<double>(((~raw) + 1) & 0x7FFFFF) * 2 * VREF /
           /*-----------------------------------------------------------*/
           (GAIN * 0x7FFFFF);
  } else {
    return static_cast<double>(raw) * 2 * VREF /
           /*---------------------------------*/
           (GAIN * 0x7FFFFF);
  }
}

void ADS1256::ADS1256_close() {
  ADS1256::spi_close();
  ADS1256::gpio_close();
}

void ADS1256::gpio_reset(void) {
  close(req.fd);
  ADS1256::gpio_close();
  ADS1256::gpio_open(gpiochip.c_str());
  ioctl(gpio_fd, GPIO_V2_GET_LINE_IOCTL, &req);
}

void ADS1256::drdy_ready(void) {
  struct gpio_v2_line_event event;
  ADS1256::gpio_reset();
  ADS1256::enable_event();
  ADS1256::gpio_get_event(&event);
  ADS1256::disable_event();
  ADS1256::gpio_reset();
}