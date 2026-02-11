// SPDX-License-Identifier: CC0-1.0
/*
 * Written in 2021 by Noralf Trønnes <noralf@tronnes.org>
 *
 * To the extent possible under law, the author(s) have dedicated all copyright and related and
 * neighboring rights to this software to the public domain worldwide. This software is
 * distributed without any warranty.
 *
 * You should have received a copy of the CC0 Public Domain Dedication along with this software.
 * If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
 */

#include <stdio.h>
#include "dln2.h"
#include "i2c_master_driver.h"

#define LOG1 printf
#define LOG2 printf

#define DLN2_I2C_MASTER_CMD(cmd) DLN2_CMD(cmd, DLN2_MODULE_I2C_MASTER)

#define DLN2_I2C_MASTER_GET_PORT_COUNT DLN2_I2C_MASTER_CMD(0x00)
#define DLN2_I2C_MASTER_ENABLE DLN2_I2C_MASTER_CMD(0x01)
#define DLN2_I2C_MASTER_DISABLE DLN2_I2C_MASTER_CMD(0x02)
#define DLN_I2C_MASTER_IS_ENABLED DLN2_I2C_MASTER_CMD(0x03)
#define DLN_I2C_MASTER_SET_FREQUENCY DLN2_I2C_MASTER_CMD(0x04)
#define DLN_I2C_MASTER_GET_FREQUENCY DLN2_I2C_MASTER_CMD(0x05)
#define DLN2_I2C_MASTER_WRITE DLN2_I2C_MASTER_CMD(0x06)
#define DLN2_I2C_MASTER_READ DLN2_I2C_MASTER_CMD(0x07)
#define DLN_I2C_MASTER_SCAN_DEVICES DLN2_I2C_MASTER_CMD(0x08)
#define DLN_I2C_MASTER_PULLUP_ENABLE DLN2_I2C_MASTER_CMD(0x09)
#define DLN_I2C_MASTER_PULLUP_DISABLE DLN2_I2C_MASTER_CMD(0x0A)
#define DLN_I2C_MASTER_PULLUP_IS_ENABLED DLN2_I2C_MASTER_CMD(0x0B)

// Linux driver timeout is 200ms
#define DLN2_I2C_TIMEOUT_US (150 * 1000)

static struct i2c_master_driver *_i2c_master_driver = NULL;
static struct gpio_driver *_gpio_driver = NULL;

static bool dln2_i2c_master_enable(struct dln2_slot *slot, bool enable)
{
    uint8_t *port = dln2_slot_header_data(slot);

    if (*port >= _i2c_master_driver->master_count)
        return dln2_response_error(slot, DLN2_RES_INVALID_PORT_NUMBER);

    uint16_t scl = _i2c_master_driver->master_config[*port].scl_io_num;
    uint16_t sda = _i2c_master_driver->master_config[*port].sda_io_num;

    int res;

    LOG1("    %s: port=%u enable=%u\n", __func__, *port, enable);

    if (dln2_slot_header_data_size(slot) != sizeof(*port))
        return dln2_response_error(slot, DLN2_RES_INVALID_COMMAND_SIZE);
    if (*port)
        return dln2_response_error(slot, DLN2_RES_INVALID_PORT_NUMBER);

    if (enable)
    {
        if (_i2c_master_driver->is_enabled(*port))
            return dln2_response_error(slot, 0); // Already enabled, treat as success

        res = dln2_pin_request(scl, DLN2_MODULE_I2C_MASTER);
        if (res)
            return dln2_response_error(slot, res);

        res = dln2_pin_request(sda, DLN2_MODULE_I2C_MASTER);
        if (res)
        {
            dln2_pin_free(scl, DLN2_MODULE_I2C_MASTER);
            return dln2_response_error(slot, res);
        }

        if (0 != _i2c_master_driver->init(*port, sda, scl))
        {
            LOG1("I2C master initialization failed\n");
            return dln2_response_error(slot, DLN2_RES_FAIL);
        }
    }
    else
    {
        res = dln2_pin_free(sda, DLN2_MODULE_I2C_MASTER);
        if (res)
            return dln2_response_error(slot, res);

        res = dln2_pin_free(scl, DLN2_MODULE_I2C_MASTER);
        if (res)
            return dln2_response_error(slot, res);

        _i2c_master_driver->deinit(*port);
    }

    return dln2_response(slot, 0);
}

bool dln2_i2c_master_initiate_recovery(struct dln2_slot *slot)
{
    uint8_t *port = dln2_slot_header_data(slot);

    if (*port >= _i2c_master_driver->master_count)
        return dln2_response_error(slot, DLN2_RES_INVALID_PORT_NUMBER);

    uint16_t scl = _i2c_master_driver->master_config[*port].scl_io_num;
    uint16_t sda = _i2c_master_driver->master_config[*port].sda_io_num;

    // To reset a frozen I2C bus, the master must force the SCL (clock) line high and toggle it 8–16 times
    // to force any stuck slave device to release the SDA (data) line, followed by a STOP condition.

    LOG1("I2C master initialization failed, attempting bus recovery\n");
    _gpio_driver->init(sda);
    _gpio_driver->set_dir(sda, false);

    _gpio_driver->init(scl);
    _gpio_driver->set_dir(scl, true);

    for (int i = 0; i < 16; i++)
    {
        _gpio_driver->put(scl, 1);
        dln2_delay(2);
        _gpio_driver->put(scl, 0);
        dln2_delay(2);

        if (_gpio_driver->get(sda))
        {
            LOG1("I2C bus recovery successful after %d clock pulses\n", i + 1);
            break;
        }
    }

    if (!_gpio_driver->get(sda))
    {
        LOG1("I2C bus recovery failed, SDA line is still low\n");
        _gpio_driver->deinit(scl);
        _gpio_driver->deinit(sda);
        return dln2_response_error(slot, DLN2_RES_FAIL);
    }

    return dln2_response(slot, 0);
}

struct dln2_i2c_master_read_msg_tx
{
    uint8_t port;
    uint8_t addr;
    uint8_t mem_addr_len;
    uint32_t mem_addr;
    uint16_t buf_len;
} TU_ATTR_PACKED;

static bool dln2_i2c_master_read(struct dln2_slot *slot)
{
    struct dln2_i2c_master_read_msg_tx *msg = dln2_slot_header_data(slot);
    struct dln2_i2c_master_read_msg_rsp
    {
        uint16_t bufferLength;
        uint8_t buffer[256];
    } TU_ATTR_PACKED *rx = (struct dln2_i2c_master_read_msg_rsp *)dln2_slot_response_data(slot);
    size_t len = msg->buf_len;

    LOG1("    %s: port=%u addr=0x%02x buf_len=%u\n", __func__, msg->port, msg->addr, msg->buf_len);

    if (dln2_slot_header_data_size(slot) != sizeof(*msg))
        return dln2_response_error(slot, DLN2_RES_INVALID_COMMAND_SIZE);
    if (msg->port >= _i2c_master_driver->master_count)
        return dln2_response_error(slot, DLN2_RES_INVALID_PORT_NUMBER);

    int ret = _i2c_master_driver->read(msg->port, msg->addr, msg->mem_addr_len, msg->mem_addr, len, rx->buffer, DLN2_I2C_TIMEOUT_US / 1000);
    put_unaligned_le16(len, rx);
    LOG2("        i2c_master_driver->read: ret =%d\n", ret);

    if (ret < 0)
        return dln2_response_error(slot, DLN2_RES_I2C_MASTER_SENDING_ADDRESS_FAILED);
    // The linux driver returns -EPROTO if length differs, so use a descriptive error (there was no read error code)
    if (ret != len)
        return dln2_response_error(slot, DLN2_RES_I2C_MASTER_SENDING_DATA_FAILED);

    put_unaligned_le16(len, rx);
    return dln2_response(slot, len + 2);
}

struct dln2_i2c_master_write_msg
{
    uint8_t port;
    uint8_t addr;
    uint8_t mem_addr_len;
    uint32_t mem_addr;
    uint16_t buf_len;
    uint8_t buf[];
} TU_ATTR_PACKED;

static bool dln2_i2c_master_is_enabled(struct dln2_slot *slot)
{
    uint8_t *port = dln2_slot_header_data(slot);

    if (*port >= _i2c_master_driver->master_count)
        return dln2_response_error(slot, DLN2_RES_INVALID_PORT_NUMBER);

    bool enabled = _i2c_master_driver->is_enabled(*port);

    LOG1("    %s: port=%u enabled=%u\n", __func__, *port, enabled);

    if (dln2_slot_header_data_size(slot) != sizeof(*port))
        return dln2_response_error(slot, DLN2_RES_INVALID_COMMAND_SIZE);

    return dln2_response_u8(slot, enabled);
}

static bool dln2_i2c_master_write(struct dln2_slot *slot)
{
    struct dln2_i2c_master_write_msg *msg = dln2_slot_header_data(slot);

    LOG1("    %s: port=%u addr=0x%02x buf_len=%u\n", __func__, msg->port, msg->addr, msg->buf_len);

    if (dln2_slot_header_data_size(slot) < sizeof(*msg))
        return dln2_response_error(slot, DLN2_RES_INVALID_COMMAND_SIZE);
    if (msg->port >= _i2c_master_driver->master_count)
        return dln2_response_error(slot, DLN2_RES_INVALID_PORT_NUMBER);

    int ret = _i2c_master_driver->write(msg->port, msg->addr, msg->mem_addr_len, msg->mem_addr, msg->buf_len, msg->buf, DLN2_I2C_TIMEOUT_US / 1000);

    if (ret < 0)
    {
        return dln2_response_error(slot, DLN2_RES_I2C_MASTER_SENDING_ADDRESS_FAILED);
    }
    if (ret != msg->buf_len)
        return dln2_response_error(slot, DLN2_RES_I2C_MASTER_SENDING_DATA_FAILED);

    return dln2_response(slot, msg->buf_len);
}

bool dln2_handle_i2c(struct dln2_slot *slot)
{
    struct dln2_header *hdr = dln2_slot_header(slot);

    switch (hdr->id)
    {
    case DLN2_I2C_MASTER_GET_PORT_COUNT:
        LOG2("Received I2C_MASTER_GET_PORT_COUNT command\n");
        return dln2_response_u8(slot, _i2c_master_driver->master_count);
        break;
    case DLN2_I2C_MASTER_ENABLE:
        LOG2("Received I2C_MASTER_ENABLE command\n");
        return dln2_i2c_master_enable(slot, true);
        break;
    case DLN2_I2C_MASTER_DISABLE:
        LOG2("Received I2C_MASTER_DISABLE command\n");
        return dln2_i2c_master_enable(slot, false);
        break;
    case DLN_I2C_MASTER_IS_ENABLED:
        LOG2("Received I2C_MASTER_IS_ENABLED command\n");
        return dln2_i2c_master_is_enabled(slot);
        break;
    case DLN_I2C_MASTER_SET_FREQUENCY:
        LOG2("Received I2C_MASTER_SET_FREQUENCY command\n");
        break;
    case DLN_I2C_MASTER_GET_FREQUENCY:
        LOG2("Received I2C_MASTER_GET_FREQUENCY command\n");
        break;
    case DLN2_I2C_MASTER_WRITE:
        LOG2("Received I2C_MASTER_WRITE command\n");
        return dln2_i2c_master_write(slot);
        break;
    case DLN2_I2C_MASTER_READ:
        LOG2("Received I2C_MASTER_READ command\n");
        return dln2_i2c_master_read(slot);
        break;
    case DLN_I2C_MASTER_SCAN_DEVICES:
        LOG2("Received I2C_MASTER_SCAN_DEVICES command\n");
        break;
    case DLN_I2C_MASTER_PULLUP_ENABLE:
        LOG2("Received I2C_MASTER_PULLUP_ENABLE command\n");
        break;
    case DLN_I2C_MASTER_PULLUP_DISABLE:
        LOG2("Received I2C_MASTER_PULLUP_DISABLE command\n");
        break;
    case DLN_I2C_MASTER_PULLUP_IS_ENABLED:
        LOG2("Received I2C_MASTER_PULLUP_IS_ENABLED command\n");
        break;
    default:
        LOG1("I2C: unknown command 0x%02x\n", hdr->id);
        return dln2_response_error(slot, DLN2_RES_COMMAND_NOT_SUPPORTED);
    }

    return false;
}

void dln2_i2c_master_init(struct dln2_peripherials *peripherals)
{
    _i2c_master_driver = (peripherals->i2c_master);
    _gpio_driver = peripherals->gpio;
}
