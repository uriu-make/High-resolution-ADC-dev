#include <fcntl.h>
#include <linux/gpio.h>
#include <math.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <time.h>
#include "gpio.h"

int GPIO::gpio_open(const char dev[]) {
  GPIO::fd = open("/dev/gpiochip0", O_RDWR | O_NONBLOCK);
  return GPIO::fd;
}

void GPIO::gpio_set_default_flag(__u64 flag) {
  GPIO::req.config.flags = flag;
}

void GPIO::gpio_set_attrs_flag(__u32 attrs_num, __u64 flags) {
  if (attrs_num < GPIO_V2_LINE_NUM_ATTRS_MAX) {
    GPIO::req.config.attrs[attrs_num].attr.id = GPIO_V2_LINE_ATTR_ID_FLAGS;
    GPIO::req.config.attrs[attrs_num].attr.flags = flags;
  }
}

void GPIO::gpio_set_attrs_devaunce(__u32 attrs_num, __u32 debounce_period_us) {
  if (attrs_num < GPIO_V2_LINE_NUM_ATTRS_MAX) {
    GPIO::req.config.attrs[attrs_num].attr.id = GPIO_V2_LINE_ATTR_ID_DEBOUNCE;
    GPIO::req.config.attrs[attrs_num].attr.flags = debounce_period_us;
  }
}

void GPIO::gpio_set_attrs_output_values(__u32 attrs_num) {
  if (attrs_num < GPIO_V2_LINE_NUM_ATTRS_MAX) {
    GPIO::req.config.attrs[attrs_num].attr.id = GPIO_V2_LINE_ATTR_ID_OUTPUT_VALUES;
    GPIO::req.config.attrs[attrs_num].attr.flags = 0;
  }
}

void GPIO::gpio_attrs_add_pin(__u32 pin, __u32 attrs_num) {
  if (attrs_num < GPIO_V2_LINE_NUM_ATTRS_MAX && GPIO::req.num_lines < GPIO_V2_LINES_MAX) {
    GPIO::req.config.attrs[attrs_num].mask = GPIO::req.config.attrs[attrs_num].mask | _BITULL(GPIO::req.num_lines);
    GPIO::bitmap[pin] = _BITULL(GPIO::req.num_lines);
    GPIO::req.offsets[GPIO::req.num_lines] = pin;
    GPIO::req.num_lines++;
  }
}

void GPIO::gpio_attrs_add_pin(__u32 pin, __u32 attrs_num, __u64 values) {
  if (attrs_num < GPIO_V2_LINE_NUM_ATTRS_MAX && GPIO::req.num_lines < GPIO_V2_LINES_MAX) {
    GPIO::req.config.attrs[attrs_num].mask = GPIO::req.config.attrs[attrs_num].mask | _BITULL(GPIO::req.num_lines);
    GPIO::req.config.attrs[attrs_num].attr.flags = GPIO::req.config.attrs[attrs_num].attr.flags | _BITULL(GPIO::req.num_lines);
    GPIO::bitmap[pin] = _BITULL(GPIO::req.num_lines);
    GPIO::req.offsets[GPIO::req.num_lines] = pin;
    GPIO::req.num_lines++;
  }
}

int GPIO::gpio_init(__u32 enable_attrs_num) {
  GPIO::req.config.num_attrs = enable_attrs_num;
  return ioctl(GPIO::fd, GPIO_V2_GET_LINE_IOCTL, &GPIO::req);
}

int GPIO::gpio_write(__u32 pin, __u64 value) {
  GPIO::values.bits = GPIO::bitmap[pin] * value;
  GPIO::values.mask = GPIO::bitmap[pin];
  return ioctl(GPIO::req.fd, GPIO_V2_LINE_SET_VALUES_IOCTL, &GPIO::values);
}

int GPIO::gpio_read(__u32 pin) {
  GPIO::values.mask = GPIO::bitmap[pin];
  ioctl(GPIO::req.fd, GPIO_V2_LINE_GET_VALUES_IOCTL, &GPIO::values);
  if (GPIO::values.bits != 0) {
    return 1;
  } else {
    return 0;
  }
}

int GPIO::gpio_get_event(struct gpio_v2_line_event* event) {
  return read(GPIO::req.fd, event, sizeof(struct gpio_v2_line_event));
}

void GPIO::gpio_close(void) {
  close(GPIO::fd);
}