#include <stdint.h>

struct i2c_master_config
{
    const char *name;
    uint16_t address;
    uint32_t freq;
    uint8_t port_num;
    uint16_t sda_io_num;
    uint16_t scl_io_num;
};

struct i2c_master_driver
{
    uint16_t master_count;
    struct i2c_master_config *master_config;

    /**
     * @brief Initialize the I2C master driver for the specified port number.
     * @param port_num The I2C master port number to initialize.
     * @param sda The GPIO number for the I2C SDA line.
     * @param scl The GPIO number for the I2C SCL line.
     * @return 0 on success, or a negative error code on failure.
     */
    int32_t (*init)(uint8_t port_num, uint16_t sda, uint16_t scl);

    /**
     * @brief Deinitialize the I2C master driver for the specified port number.
     * @param port_num The I2C master port number to deinitialize.
     */
    int32_t (*deinit)(uint8_t port_num);

    /**
     * @brief Read data from an I2C slave device.
     * @param port_num The I2C master port number.
     * @param addr The I2C slave device address.
     * @param data Pointer to the buffer where the read data will be stored.
     * @param len The number of bytes to read
     * @return The number of bytes read on success, or a negative error code on failure.
     */
    int32_t (*read)(uint8_t port_num, uint8_t slave_addr,
                    uint8_t mem_addr_len, uint32_t mem_addr,
                    uint16_t len, uint8_t *data, uint32_t timeout_ms);

    /**
     * @brief Write data to an I2C slave device.
     * @param port_num The I2C master port number.
     * @param addr The I2C slave device address.
     * @param data Pointer to the buffer containing the data to be written.
     * @param len The number of bytes to write.
     * @return The number of bytes written on success, or a negative error code on failure.
     */
    int32_t (*write)(uint8_t port_num, uint8_t slave_addr,
                     uint8_t mem_addr_len, uint32_t mem_addr,
                     uint16_t len, uint8_t *data, uint32_t timeout_ms);

    /**
     * @brief Check if the specified I2C master port is enabled.
     * @param port_num The I2C master port number to check.
     * @return true if the port is enabled, false otherwise.
     */
    bool (*is_enabled)(uint8_t port_num);
};
