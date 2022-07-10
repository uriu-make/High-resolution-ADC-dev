#include <fcntl.h>
#include <linux/gpio.h>
#include <linux/spi/spidev.h>
#include <math.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <time.h>
#include <iostream>
#ifndef __SPI_H_
#define __SPI_H_

class spi {
 private:
 protected:
  int fd;
  __u32 speed;

 public:
  int openSPI(const char dev[]);
  int spiMode(__u8 mode);
  int spiSpeed(__u32 speed_hz);
  int transfer(const struct spi_ioc_transfer* t, int n);
};
#endif