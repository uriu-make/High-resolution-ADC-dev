#include <fcntl.h>
#include <linux/gpio.h>
#include <math.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <time.h>
#include "gpio.h"

int GPIO::gpio_open(const char dev[]) {
  fd = open("/dev/gpiochip0", O_RDWR | O_NONBLOCK);
  return fd;
}

void GPIO::gpio_set_default_flag(__u64 flag) {
  req.config.flags = flag;
}

void GPIO::gpio_set_attrs_flag(__u32 attrs_num, __u64 flags) {
  if (attrs_num < GPIO_V2_LINE_NUM_ATTRS_MAX) {
    req.config.attrs[attrs_num].attr.id = GPIO_V2_LINE_ATTR_ID_FLAGS;
    req.config.attrs[attrs_num].attr.flags = flags;
  }
}

void GPIO::gpio_set_attrs_devaunce(__u32 attrs_num, __u32 debounce_period_us) {
  if (attrs_num < GPIO_V2_LINE_NUM_ATTRS_MAX) {
    req.config.attrs[attrs_num].attr.id = GPIO_V2_LINE_ATTR_ID_DEBOUNCE;
    req.config.attrs[attrs_num].attr.debounce_period_us = debounce_period_us;
  }
}

void GPIO::gpio_set_attrs_output_values(__u32 attrs_num) {
  if (attrs_num < GPIO_V2_LINE_NUM_ATTRS_MAX) {
    req.config.attrs[attrs_num].attr.id = GPIO_V2_LINE_ATTR_ID_OUTPUT_VALUES;
    req.config.attrs[attrs_num].attr.values = 0;
  }
}

void GPIO::gpio_attrs_add_pin(__u32 pin, __u32 attrs_num) {
  if (attrs_num < GPIO_V2_LINE_NUM_ATTRS_MAX && req.num_lines < GPIO_V2_LINES_MAX) {
    req.config.attrs[attrs_num].mask = req.config.attrs[attrs_num].mask | _BITULL(req.num_lines);
    bitmap[pin] = _BITULL(req.num_lines);
    req.offsets[req.num_lines] = pin;
    req.num_lines++;
  }
}

void GPIO::gpio_attrs_add_pin(__u32 pin, __u32 attrs_num, __u64 values) {
  if (attrs_num < GPIO_V2_LINE_NUM_ATTRS_MAX && req.num_lines < GPIO_V2_LINES_MAX) {
    req.config.attrs[attrs_num].mask = req.config.attrs[attrs_num].mask | _BITULL(req.num_lines);
    req.config.attrs[attrs_num].attr.values = req.config.attrs[attrs_num].attr.values | (_BITULL(req.num_lines) * values);
    bitmap[pin] = _BITULL(req.num_lines);
    req.offsets[req.num_lines] = pin;
    req.num_lines++;
  }
}

void GPIO::gpio_attrs_add_pins(__u32 pin[], __u32 num, __u32 attrs_num) {
  for (__u32 i = 0; i < num; i++) {
    if (attrs_num < GPIO_V2_LINE_NUM_ATTRS_MAX && req.num_lines < GPIO_V2_LINES_MAX) {
      req.config.attrs[attrs_num].mask = req.config.attrs[attrs_num].mask | _BITULL(req.num_lines);
      bitmap[pin[i]] = _BITULL(req.num_lines);
      req.offsets[req.num_lines] = pin[i];
      req.num_lines++;
    }
  }
}

void GPIO::gpio_attrs_add_pins(__u32 pin[], __u32 num, __u32 attrs_num, __u64 values) {
  for (__u32 i = 0; i < num; i++) {
    if (attrs_num < GPIO_V2_LINE_NUM_ATTRS_MAX && req.num_lines < GPIO_V2_LINES_MAX) {
      req.config.attrs[attrs_num].mask = req.config.attrs[attrs_num].mask | _BITULL(req.num_lines);
      bitmap[pin[i]] = _BITULL(req.num_lines);
      req.offsets[req.num_lines] = pin[i];
      req.num_lines++;
    }
  }
  req.config.attrs[attrs_num].attr.values = req.config.attrs[attrs_num].attr.values | values;
}

int GPIO::gpio_init(__u32 enable_attrs_num) {
  req.config.num_attrs = enable_attrs_num;
  return ioctl(fd, GPIO_V2_GET_LINE_IOCTL, &req);
}

void GPIO::gpio_set_attrs_set_mask(__u32 attrs_num, __u64 mask) {
  req.config.attrs[attrs_num].mask = mask;
}

int GPIO::gpio_reconfig(__u32 enable_attrs_num) {
  req.config.num_attrs = enable_attrs_num;
  return ioctl(req.fd, GPIO_V2_LINE_SET_CONFIG_IOCTL, &req.config);
}

__u64 GPIO::gpio_get_attrs_mask(__u32 attrs_num) {
  return req.config.attrs[attrs_num].mask;
}

int GPIO::gpio_write(__u32 pin, __u64 value) {
  values.bits = bitmap[pin] * value;
  values.mask = bitmap[pin];
  return ioctl(req.fd, GPIO_V2_LINE_SET_VALUES_IOCTL, &values);
}

int GPIO::gpio_write_bitmap(__u64 mask, __u64 values) {
  this->values.bits = values;
  this->values.mask = mask;
  return ioctl(req.fd, GPIO_V2_LINE_SET_VALUES_IOCTL, &this->values);
}

int GPIO::gpio_read(__u32 pin) {
  values.mask = bitmap[pin];
  ioctl(req.fd, GPIO_V2_LINE_GET_VALUES_IOCTL, &values);
  if (values.bits != 0) {
    return 1;
  } else {
    return 0;
  }
}

__u64 GPIO::gpio_read_bitmap(__u64 mask) {
  values.mask = mask;
  ioctl(req.fd, GPIO_V2_LINE_GET_VALUES_IOCTL, &values);
  return values.bits;
}

int GPIO::gpio_get_event(struct gpio_v2_line_event* event) {
  return read(req.fd, event, sizeof(struct gpio_v2_line_event));
}

void GPIO::gpio_close(void) {
  close(fd);
}