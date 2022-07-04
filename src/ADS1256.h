#include <fcntl.h>
#include <linux/gpio.h>
#include <linux/spi/spidev.h>
#include <math.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <time.h>
#include <iostream>

#include "spi.h"

#ifndef __ADS1256_H_
#define __ADS1256_H_

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

class ADS1256 : spi {
 public:
  int setDRDYpin();
  int setDataRate(__u8 rate);
  int setPGA(int gain);
  int setVREF(double vref);
  int setAIN(__u8 positive, __u8 negative);
  int AnalogRead();
};

#endif