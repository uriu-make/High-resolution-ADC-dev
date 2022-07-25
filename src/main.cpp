#include <fcntl.h>
#include <linux/gpio.h>
#include <linux/spi/spidev.h>
#include <math.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <time.h>
#include <iostream>
#include <sys/time.h>

#define ADS1256_CLOCK 7680000
#define NUM           10000

struct data {
  int adc;
  struct timeval time;
  struct timeval rate;
};

int main() {
  // int data = 0;
  __u8 tx[4] = {0, 0, 0, 0};
  __u8 rx[4] = {0, 0, 0, 0};
  __u8 reg[11] = {0};

  __u8 mode = SPI_MODE_1;
  __u32 freq = 1900000;
  unsigned short delay_sclk = std::ceil(50 * 1000000 / ADS1256_CLOCK);
  int buf;
  struct data data[NUM + 1];

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
  struct gpio_v2_line_request gpio_req = {0};
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
  // struct gpio_v2_line_values value;
  // value.bits = 0;
  // value.mask = _BITULL(0);
  // ioctl(gpio_req.fd, GPIO_V2_LINE_SET_VALUES_IOCTL, &value);
  // usleep(1);
  // value.bits = _BITULL(0);
  // value.mask = _BITULL(0);
  // ioctl(gpio_req.fd, GPIO_V2_LINE_SET_VALUES_IOCTL, &value);

  //初期設定読み込み
  struct spi_ioc_transfer arg[2] = {0};

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
  reg[1] = 0b01100111;
  // reg[1] = 0b00000001;
  // reg[2] = 0b00000011;  // GPA
  reg[2] = 0b00000110;
  // reg[3] = 0b10000100;
  // reg[3] = 0b10000010;
  reg[3] = 0b11110000;

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

  tx[0] = 0b11110000;
  tx[1] = 0;
  arg[0].tx_buf = (__u64)&tx[0];
  arg[0].rx_buf = (__u64)NULL;
  arg[0].len = 1;
  arg[0].delay_usecs = 0;
  arg[0].speed_hz = freq;
  arg[0].bits_per_word = 8;
  arg[0].cs_change = 0;
  sleep(1);
  ioctl(spi_fd, SPI_IOC_MESSAGE(1), &arg);
  sleep(1);

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

  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < NUM + 1; j++) {
      read(gpio_req.fd, &event, sizeof(event));
      ioctl(spi_fd, SPI_IOC_MESSAGE(2), arg);
      gettimeofday(&data[j].time, NULL);
      buf = (rx[1] << 16) | (rx[2] << 8) | rx[3];
      // data[j].adc = (double)buf * 5 / 0x7FFFFF;
      data[j].adc = buf;
    }
  }
  for (int i = 1; i < NUM + 1; i++) {
    data[i].rate.tv_sec = data[i].time.tv_sec - data[i - 1].time.tv_sec;
    data[i].rate.tv_usec = data[i].time.tv_usec - data[i - 1].time.tv_usec;
  }
  data[0] = data[1];
  for (int i = 1; i < NUM + 1; i++) {
    data[i].time.tv_sec = data[i].time.tv_sec - data[0].time.tv_sec;
    data[i].time.tv_usec = data[i].time.tv_usec - data[0].time.tv_usec;
  }

  std::printf("count,t,rate,volt,adc\n");
  std::fflush(stdout);
  for (int i = 1; i < NUM + 1; i++) {
    std::printf("%d,%ld,%ld,%.8lf,%d\n",
                i,
                data[i].time.tv_sec * 1000000 + data[i].time.tv_usec,
                data[i].rate.tv_sec * 1000000 + data[i].rate.tv_usec,
                double(data[i].adc) * 5 / (pow(2, reg[2] & 0b00000111) * 0x7FFFFF),
                data[i].adc);
  }

  close(gpio_req.fd);
  close(gpio_fd);
  close(spi_fd);

  return 0;
}
