#include <fcntl.h>
#include <linux/gpio.h>
#include <linux/spi/spidev.h>
#include <math.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <time.h>
#include <iostream>

class spi {
 private:
  int fd;
  __u32 speed;

 public:
  int openSPI(const char dev[]);
  int spiMode(__u8 mode);
  int spiSpeed(__u32 speed_hz);
  int transfer(struct spi_ioc_transfer* t, int n);
};

int spi::openSPI(const char dev[]) {
  spi::fd = open(dev, O_RDWR | O_NONBLOCK);
  return spi::fd;
}
int spi::spiMode(__u8 mode) {
  ioctl(spi::fd, SPI_IOC_WR_MODE, &mode);
  ioctl(spi::fd, SPI_IOC_RD_MODE, &mode);
}

int spi::spiSpeed(__u32 speed_hz) {}