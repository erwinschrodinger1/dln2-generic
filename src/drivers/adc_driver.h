#include <stdint.h>

#pragma once
#include <stdbool.h>
#include <stdint.h>

/* opaque handle */
typedef struct adc_repeating_timer adc_repeating_timer_t;

/* generic callback */
typedef bool (*adc_repeating_timer_callback_t)(adc_repeating_timer_t *timer);

struct adc_port {
  uint16_t channel_count;
  uint16_t *channels;
};

struct adc_driver {
  int port_count;
  struct adc_port *ports;

  int (*init)();
  // for general adc init if required
  int (*port_enable)(uint8_t port);
  int (*channel_enable)(uint8_t port, uint16_t gpio);
  void (*port_disable)(uint8_t port);

  void (*deinit)();
  int (*read)(uint8_t port, uint16_t channel);
  bool (*add_repeating_timer_us)(int64_t delay_us,
                                 adc_repeating_timer_callback_t callback,
                                 void *user_data, adc_repeating_timer_t *out);
  bool (*cancel_repeating_timer)(adc_repeating_timer_t *timer);
};
