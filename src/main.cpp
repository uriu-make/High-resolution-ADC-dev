#include <iostream>
#include "ADS1256.h"

#define ADS1256_CLOCK 7680000
#define DRDY          17
#define RESET         18
#define PDWN          27

#define NUM 10000

int main() {
  ADS1256 ads1256("/dev/spidev0.0", "/dev/gpiochip0", 17, 18, 27);
  std::cout << ads1256.open() << std::endl;
  std::cout << ads1256.init() << std::endl;
  std::cout << ads1256.setClock(ADS1256_CLOCK) << std::endl;
  ads1256.setVREF(2.5);
  ads1256.reset();

  ads1256.setAIN(AIN0, AGND);
  std::cout << ads1256.setPGA(GAIN_1) << std::endl;
  ads1256.enavle_event();
  for (int i = 0; i < NUM; i++) {
    std::cout << ads1256.AnalogRead() << std::endl;
  }
  ads1256.ADS1256_close();
  return 0;
}