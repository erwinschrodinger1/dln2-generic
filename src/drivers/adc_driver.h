#include <stdint.h>

#pragma once
#include <stdbool.h>
#include <stdint.h>

/* opaque handle */
typedef struct adc_repeating_timer adc_repeating_timer_t;

/* generic callback */
typedef bool (*adc_repeating_timer_callback_t)(adc_repeating_timer_t *timer);

struct adc_driver {
  int count;
  uint16_t *ports;
  // for general adc init if required
  int (*init)();
  void (*deinit)();
  int (*read)(uint16_t input);
  bool (*add_repeating_timer_us)(int64_t delay_us,
                                 adc_repeating_timer_callback_t callback,
                                 void *user_data, adc_repeating_timer_t *out);
  void (*adc_gpio_init)(uint16_t gpio);
  bool (*cancel_repeating_timer)(adc_repeating_timer_t *timer);
};
