#include <iostream>
#include "ADS1256.h"

#define ADS1256_CLOCK 7680000
#define DRDY          17
#define RESET         18
#define PDWN          27

#define NUM 5

int main() {
  ADS1256 ads1256("/dev/spidev0.0", "/dev/gpiochip0", 17, 18, 27, ADS1256_CLOCK);
  ads1256.open();
  ads1256.init();
  ads1256.setVREF(2.5);
  ads1256.reset();
  usleep(50000);
  ads1256.setClockOUT(CLOCK_OFF);
  ads1256.setSampleRate(DATARATE_30000);
  ads1256.setAIN(AIN6, AIN7);
  ads1256.setPGA(GAIN_64);

  ads1256.selfCal();
  ads1256.enable_event();
  for (int i = 0; i < NUM; i++) {
    std::cout << ads1256.AnalogReadSync() << std::endl;
  }
  ads1256.disable_event();
  ads1256.gpio_reset();

  // ads1256.reset();
  // usleep(50000);
  ads1256.setClockOUT(CLOCK_OFF);
  ads1256.setSampleRate(DATARATE_30000);
  ads1256.setAIN(AIN7, AIN6);
  ads1256.setPGA(GAIN_64);
  ads1256.selfCal();
  ads1256.enable_event();
  for (int i = 0; i < NUM; i++) {
    std::cout << ads1256.AnalogReadSync() << std::endl;
  }
  ads1256.ADS1256_close();
  return 0;
}