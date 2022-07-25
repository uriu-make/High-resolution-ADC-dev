#include <iostream>
#include "gpio.h"

int main() {
  GPIO gpio;
  std::cout << gpio.gpio_open("/dev/gpiochip0") << std::endl;
  return 0;
}