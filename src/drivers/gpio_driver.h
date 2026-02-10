#ifndef _GPIO_DRIVER_H_
#define _GPIO_DRIVER_H_

#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef void (*gpio_irq_callback_t)(unsigned int gpio, uint32_t event_mask);

struct gpio_driver {
  uint32_t gpio_count;
  uint32_t *pin;

  /*! \brief Initialise a GPIO for (enabled I/O and set func to GPIO_FUNC_SIO)
   *  \ingroup hardware_gpio
   *
   * Clear the output enable (i.e. set to input).
   * Clear any output value.
   *
   * \param gpio GPIO number
   */
  void (*init)(uint32_t gpio);

  /*! \brief Resets a GPIO back to the NULL function, i.e. disables it.
   *  \ingroup hardware_gpio
   *
   * \param gpio GPIO number
   */
  void (*deinit)(uint32_t gpio);
  void (*pull_down)(uint32_t gpio);
  bool (*get)(uint32_t gpio);
  void (*put)(uint32_t gpio, bool value);
  bool (*get_out_level)(uint32_t pin);
  void (*set_dir)(uint32_t gpio, bool out);
  uint32_t (*get_dir)(uint32_t gpio);
  void (*set_irq_enabled)(uint32_t gpio, uint32_t event_mask, bool enabled);
  void (*intr_enable)(uint32_t gpio);
  void (*set_irq_callback)(gpio_irq_callback_t callback);
  void (*uninstall_irq_callback)(void);
};

#endif
