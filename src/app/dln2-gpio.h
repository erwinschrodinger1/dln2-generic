enum gpio_irq_level {
  GPIO_IRQ_LEVEL_LOW = 0x1u,  ///< IRQ when the GPIO pin is a logical 0
  GPIO_IRQ_LEVEL_HIGH = 0x2u, ///< IRQ when the GPIO pin is a logical 1
  GPIO_IRQ_EDGE_FALL = 0x4u,  ///< IRQ when the GPIO has transitioned from a
                              ///< logical 1 to a logical 0
  GPIO_IRQ_EDGE_RISE = 0x8u,  ///< IRQ when the GPIO has transitioned from a
                              ///< logical 0 to a logical 1
};
