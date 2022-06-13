#include <fcntl.h>
#include <linux/gpio.h>
#include <linux/spi/spidev.h>
#include <math.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <iostream>

int main() {
  __u8 tx[4] = {0xFE, 0, 0, 0};
  __u8 rx[4] = {0, 0, 0, 0};
  __u8 mode = SPI_MODE_1;
  __u32 freq = 3500000;
  int gpio_fd = open("/dev/gpiochip0", O_RDWR | O_NONBLOCK);
  int spi_fd = open("/dev/spidev0.0", O_RDWR | O_NONBLOCK);
  if (gpio_fd <= 0) {
    std::cerr << "/dev/gpiochip0 is not open." << std::endl;
    exit(0);
  }
  if (spi_fd <= 0) {
    std::cerr << "/dev/spidev0.0 is not open." << std::endl;
    exit(0);
  }
  // gpioピンの初期化
  struct gpio_v2_line_request gpio_req;
  memset(&gpio_req, 0, sizeof(gpio_req));
  gpio_req.num_lines = 1;
  gpio_req.offsets[0] = 17;  // 17番を使用
  std::sprintf(gpio_req.consumer, "ADS1256_DRDY_PIN");
  gpio_req.config.flags = GPIO_V2_LINE_FLAG_INPUT;
  gpio_req.config.num_attrs = 1;
  gpio_req.config.attrs[0].mask = _BITULL(0);
  gpio_req.config.attrs[0].attr.id = GPIO_V2_LINE_ATTR_ID_FLAGS;
  gpio_req.config.attrs[0].attr.flags = GPIO_V2_LINE_FLAG_INPUT | GPIO_V2_LINE_FLAG_EDGE_FALLING;  //入力、

  ioctl(gpio_fd, GPIO_V2_GET_LINE_IOCTL, &gpio_req);
  if (gpio_req.fd <= 0) {
    std::cout << "gpio request error." << std::endl;
    exit(0);
  }

  struct gpio_v2_line_event event;

  std::cout << ioctl(spi_fd, SPI_IOC_WR_MODE, &mode) << std::endl;
  std::cout << ioctl(spi_fd, SPI_IOC_RD_MODE, &mode) << std::endl;
  std::cout << ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &freq) << std::endl;
  std::cout << ioctl(spi_fd, SPI_IOC_RD_MAX_SPEED_HZ, &freq) << std::endl;

  struct spi_ioc_transfer arg[2];
  memset(&arg, 0, sizeof(arg));
  memset(tx, 0, sizeof(tx));
  memset(rx, 0, sizeof(rx));
  arg[0].tx_buf = (__u64)tx;
  arg[0].rx_buf = (__u64)rx;
  arg[0].len = 1;
  arg[0].delay_usecs = 0;
  arg[0].speed_hz = freq;
  arg[0].bits_per_word = 8;
  arg[0].cs_change = 0;
  // std::cout << ioctl(spi_fd, SPI_IOC_MESSAGE(1), &arg) << std::endl;
  // pow(10, 6) * 60 / freq;
  tx[0] = 0b00010000 | 0x03;
  arg[0].len = 2;
  arg[0].delay_usecs = 7;
  arg[1].tx_buf = (__u64)&tx[2];
  arg[1].rx_buf = (__u64)&rx[2];
  arg[1].len = 1;
  arg[1].delay_usecs = 0;
  arg[1].speed_hz = freq;
  arg[1].bits_per_word = 8;
  arg[1].cs_change = 0;
  read(gpio_req.fd, &event, sizeof(event));
  std::cout << ioctl(spi_fd, SPI_IOC_MESSAGE(2), arg) << std::endl;
  std::printf("%d\n", rx[2]);
  return 0;
}
