#include <fcntl.h>
#include <linux/gpio.h>
#include <linux/spi/spidev.h>
#include <math.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <time.h>

#include "spi.h"

int SPI::spi_open(const char dev[]) {
  SPI::fd = open(dev, O_RDWR | O_NONBLOCK | O_CLOEXEC);
  return SPI::fd;
}
int SPI::spi_mode(__u8 mode) {
  int ret = ioctl(SPI::fd, SPI_IOC_WR_MODE, &mode);
  if (ret >= 0) {
    ret = ioctl(SPI::fd, SPI_IOC_RD_MODE, &mode);
  }
  return ret;
}

int SPI::spi_speed(__u32 speed_hz) {
  SPI::speed = speed_hz;
  int ret = ioctl(SPI::fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed_hz);
  if (ret >= 0) {
    ret = ioctl(SPI::fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed_hz);
  }
  return ret;
}

int SPI::spi_transfer(const struct spi_ioc_transfer* arg, int n) {
  return ioctl(SPI::fd, SPI_IOC_MESSAGE(n), arg);
}

void SPI::spi_close(void) {
  close(SPI::fd);
}