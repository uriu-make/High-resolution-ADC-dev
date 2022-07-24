#include <fcntl.h>
#include <linux/gpio.h>
#include <linux/spi/spidev.h>
#include <math.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <time.h>
// #include <iostream>
#ifndef __SPI_H_
#define __SPI_H_

class SPI {
 private:
 protected:
  int fd;
  __u32 speed;

 public:
  int spi_open(const char dev[]);                          // SPIバスを開く
  int spi_mode(__u8 mode);                                 // SPIモードを設定
  int spi_speed(__u32 speed_hz);                           // SCLKの最大値を設定(Hz)
  int spi_transfer(const struct spi_ioc_transfer* t, int n);  //データの送受信を行う
  int spi_end();
};
#endif