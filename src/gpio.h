#include <fcntl.h>
#include <linux/gpio.h>
#include <math.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <time.h>
#define HIGH 1
#define LOW  0
#pragma once

class GPIO {
 private:
 protected:
  int fd;
  struct gpio_v2_line_request req = {0};
  struct gpio_v2_line_values values = {0};
  __u64 bitmap[GPIO_V2_LINES_MAX] = {0};

 public:
  int gpio_open(const char dev[]);
  void set_gpioline_name(const char name[]);

  void gpio_set_default_flag(__u64 flag);

  void gpio_set_attrs_flag(__u32 attrs_num, __u64 flags);
  void gpio_set_attrs_devaunce(__u32 attrs_num, __u32 debounce_period_us);
  void gpio_set_attrs_output_values(__u32 attrs_num);
  void gpio_attrs_add_pin(__u32 pin, __u32 attrs_num);
  void gpio_attrs_add_pin(__u32 pin, __u32 attrs_num, __u64 values);

  void gpio_attrs_add_pins(__u32 pin[], __u32 num, __u32 attrs_num);
  void gpio_attrs_add_pins(__u32 pin[], __u32 num, __u32 attrs_num, __u64 values);

  int gpio_init(__u32 enable_attrs_num);
  void gpio_set_attrs_set_mask(__u32 attrs_num, __u64 mask);

  int gpio_reconfig(__u32 enable_attrs_num);
  __u64 gpio_get_attrs_mask(__u32 attrs_num);

  int gpio_write(__u32 pin, __u64 value);
  int gpio_write_bitmap(__u64 mask, __u64 value);
  int gpio_read(__u32 pin);
  __u64 gpio_read_bitmap(__u64 mask);
  int gpio_get_event(struct gpio_v2_line_event *event);
  void gpio_close(void);
};
