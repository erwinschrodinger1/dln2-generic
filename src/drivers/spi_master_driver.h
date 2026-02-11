#include <stdint.h>

struct spi_master
{
  uint32_t freq;
  uint8_t mode;
  uint8_t bpw;
  uint16_t miso_pin;
  uint16_t mosi_pin;
  uint16_t sck_pin;
  uint16_t slave_count;
  struct spi_slave *slave;
};

struct spi_slave
{
  uint16_t cs_pin;
};

struct spi_master_driver
{
  uint16_t master_count;
  struct spi_master *master;
  void (*init)(struct spi_master master);
};
