#include <stdint.h>

struct spi_config {
  uint32_t freq;
  uint8_t mode;
  uint8_t bpw;
};

struct spi_master {
  uint miso_pin;
  uint mosi_pin;
  uint sck_pin;
  uint slave_count;
  struct spi_slave *slave;
  struct spi_config config;
};

struct spi_slave {
  uint cs_pin;
};

struct spi_driver {
  uint master_count;
  struct spi_master *master;
  void (*init)(struct spi_master master);
};
