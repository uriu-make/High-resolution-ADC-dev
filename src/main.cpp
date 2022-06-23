#include <fcntl.h>
#include <linux/gpio.h>
#include <linux/spi/spidev.h>
#include <math.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <time.h>
#include <iostream>

#define ADS1256_CLOCK 7680000

int main() {
  int data = 0;
  __u8 tx[4] = {0, 0, 0, 0};
  __u8 rx[4] = {0, 0, 0, 0};
  __u8 reg[11];

  __u8 mode = SPI_MODE_1;
  __u32 freq = 2000000;
  unsigned short delay_sclk = std::ceil(50 * 1000000 / ADS1256_CLOCK);
  double buf[10000];

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
  gpio_req.num_lines = 2;
  gpio_req.offsets[0] = 18;
  gpio_req.offsets[1] = 17;  // 17番を使用
  std::sprintf(gpio_req.consumer, "ADS1256");
  gpio_req.config.flags = GPIO_V2_LINE_FLAG_OUTPUT;
  gpio_req.config.num_attrs = 1;
  gpio_req.config.attrs[0].mask = _BITULL(0);
  gpio_req.config.attrs[0].attr.id = GPIO_V2_LINE_ATTR_ID_OUTPUT_VALUES;
  gpio_req.config.attrs[0].attr.values = _BITULL(0);

  gpio_req.config.attrs[1].mask = _BITULL(1);
  gpio_req.config.attrs[1].attr.id = GPIO_V2_LINE_ATTR_ID_FLAGS;
  gpio_req.config.attrs[1].attr.flags = GPIO_V2_LINE_FLAG_INPUT | GPIO_V2_LINE_FLAG_EDGE_FALLING;  //入力、High->Lowイベント検出

  ioctl(gpio_fd, GPIO_V2_GET_LINE_IOCTL, &gpio_req);  // gpioを設定 gpioイベントの検出はなし

  if (gpio_req.fd <= 0) {
    std::cerr << "gpio request error." << std::endl;
    exit(0);
  }

  struct gpio_v2_line_event event;

  ioctl(spi_fd, SPI_IOC_WR_MODE, &mode);
  ioctl(spi_fd, SPI_IOC_RD_MODE, &mode);
  if (ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &freq)) {
    std::cerr << "speed error." << std::endl;
    exit(0);
  }
  if (ioctl(spi_fd, SPI_IOC_RD_MAX_SPEED_HZ, &freq)) {
    std::cerr << "speed error." << std::endl;
    exit(0);
  }
  //初期化
  struct gpio_v2_line_values value;
  value.bits = 0;
  value.mask = _BITULL(0);
  ioctl(gpio_req.fd, GPIO_V2_LINE_SET_VALUES_IOCTL, &value);
  usleep(1);
  value.bits = _BITULL(0);
  value.mask = _BITULL(0);
  ioctl(gpio_req.fd, GPIO_V2_LINE_SET_VALUES_IOCTL, &value);

  //初期設定読み込み
  struct spi_ioc_transfer arg[2];
  memset(&arg, 0, sizeof(arg));
  memset(tx, 0, sizeof(tx));
  memset(rx, 0, sizeof(rx));
  memset(reg, 0, sizeof(reg));
  tx[0] = 0b00010000;
  tx[1] = 10;
  // tx[0] = 0b01010000 | 0x03;
  // tx[2] = 0b01110010;

  arg[0].tx_buf = (__u64)&tx[0];
  arg[0].rx_buf = (__u64)NULL;
  arg[0].len = 2;
  arg[0].delay_usecs = delay_sclk;
  arg[0].speed_hz = freq;
  arg[0].bits_per_word = 8;
  arg[0].cs_change = 0;

  arg[1].tx_buf = (__u64)NULL;
  arg[1].rx_buf = (__u64)&reg;
  arg[1].len = 11;
  arg[1].delay_usecs = 0;
  arg[1].speed_hz = freq;
  arg[1].bits_per_word = 8;
  arg[1].cs_change = 0;

  usleep(1000);

  ioctl(spi_fd, SPI_IOC_MESSAGE(2), arg);
  //設定変更
  reg[0] = reg[0] | 0b00000100;
  reg[1] = 0b0001000;
  reg[2] = 0b00000000;
  reg[3] = 0b01110010;
  // reg[3] = 0b10110000;

  tx[0] = 0b01010000;
  tx[1] = 3;
  arg[0].tx_buf = (__u64)&tx[0];
  arg[0].rx_buf = (__u64)NULL;
  arg[0].len = 2;
  arg[0].delay_usecs = delay_sclk;
  arg[0].speed_hz = freq;
  arg[0].bits_per_word = 8;
  arg[0].cs_change = 0;

  arg[1].tx_buf = (__u64)&reg;
  arg[1].rx_buf = (__u64)NULL;
  arg[1].len = 4;
  arg[1].delay_usecs = 0;
  arg[1].speed_hz = freq;
  arg[1].bits_per_word = 8;
  arg[1].cs_change = 0;

  ioctl(spi_fd, SPI_IOC_MESSAGE(2), &arg);
  //測定
  tx[0] = 0b00000001;

  arg[0].tx_buf = (__u64)&tx[0];
  arg[0].rx_buf = (__u64)NULL;
  arg[0].len = 1;
  arg[0].delay_usecs = delay_sclk;
  arg[0].cs_change = 0;
  arg[1].tx_buf = (__u64)NULL;
  arg[1].rx_buf = (__u64)&rx[1];
  arg[1].len = 3;
  arg[1].delay_usecs = 0;
  arg[1].speed_hz = freq;
  arg[1].bits_per_word = 8;
  arg[1].cs_change = 0;

  gpio_req.config.num_attrs = 2;  // gpioイベントの検出を開始
  ioctl(gpio_req.fd, GPIO_V2_LINE_SET_CONFIG_IOCTL, &gpio_req.config);

  for (int i = 0; i < 10000; i++) {
    data = 0;
    read(gpio_req.fd, &event, sizeof(event));
    ioctl(spi_fd, SPI_IOC_MESSAGE(2), arg);
    data = (rx[1] << 16) | (rx[2] << 8) | rx[3];
    buf[i] = (double)data * 5 / 0x7FFFFF;
    // std::printf(" %d\n%lf\r", i, (double)data * 5 / 0x7FFFFF);
    // std::printf("\33[1A");

    // std::printf("%d %x:%x:%x,%lf\n", data, rx[1], rx[2], rx[3], (double)data * 5 / 0x7FFFFF);
    // std::fflush(stdout);
  }
  double sum = 0;
  for (int i = 0; i < 10000; i++) {
    sum += buf[i];
  }
  double ave = sum / 10000;
  double variance = 0;
  for (int i = 0; i < 10000; i++) {
    variance += std::pow(ave - buf[i], 2);
  }
  variance = variance / 10000;

  // std::FILE *fp = std::fopen("data/test.csv", "w");
  // if (fp != NULL) {
  //   for (int i = 0; i < 10000; i++) {
  //     std::fprintf(fp, "%lf\n", buf[i]);
  //   }
  // }
  std::printf("ave:%lf variance:%lf\n", ave, variance);
  return 0;
}
