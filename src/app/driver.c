// SPDX-License-Identifier: CC0-1.0
/*
 * Written in 2021 by Noralf Trønnes <noralf@tronnes.org>
 *
 * To the extent possible under law, the author(s) have dedicated all copyright
 * and related and neighboring rights to this software to the public domain
 * worldwide. This software is distributed without any warranty.
 *
 * You should have received a copy of the CC0 Public Domain Dedication along
 * with this software. If not, see
 * <http://creativecommons.org/publicdomain/zero/1.0/>.
 */
#include "device/usbd_pvt.h"
#include "dln2.h"
#include "dln2_log.h"
#include "esp_log.h"
#include "tusb_option.h"

#define LOG1 // printf
#define LOG2 // printf

static const char *TAG = "DLN_DRIVER";

static uint8_t _bulk_in;
static uint8_t _bulk_out;

static void driver_init(void) {
  LOG_INFO("=== driver_init() called ===");
  LOG_INFO("Initializing DLN2 USB driver");
}

static void driver_reset(uint8_t rhport) {
  LOG_INFO("=== driver_reset() called ===");
  LOG_INFO("rhport: %u", rhport);
  LOG_INFO("Resetting bulk_in: 0x%02x, bulk_out: 0x%02x", _bulk_in, _bulk_out);
  (void)rhport;
}

static void driver_disable_endpoint(uint8_t rhport, uint8_t *ep_addr) {
  LOG_INFO("=== driver_disable_endpoint() called ===");
  LOG_INFO("rhport: %u, ep_addr: 0x%02x", rhport, *ep_addr);

  if (*ep_addr) {
    LOG_INFO("Closing endpoint 0x%02x", *ep_addr);
    usbd_edpt_close(rhport, *ep_addr);
    *ep_addr = 0;
    LOG_INFO("Endpoint closed and cleared");
  } else {
    LOG_INFO("Endpoint already disabled (NULL)");
  }
}

static uint16_t driver_open(uint8_t rhport,
                            tusb_desc_interface_t const *itf_desc,
                            uint16_t max_len) {
  LOG_INFO("=== driver_open() called ===");
  LOG_INFO("rhport: %u", rhport);
  LOG_INFO("bInterfaceNumber: %u", itf_desc->bInterfaceNumber);
  LOG_INFO("bInterfaceClass: 0x%02x", itf_desc->bInterfaceClass);
  LOG_INFO("bNumEndpoints: %u", itf_desc->bNumEndpoints);
  LOG_INFO("max_len: %u", max_len);

  if (TUSB_CLASS_VENDOR_SPECIFIC != itf_desc->bInterfaceClass) {
    LOG_ERROR("Invalid interface class! Expected 0x%02x, got 0x%02x",
             TUSB_CLASS_VENDOR_SPECIFIC, itf_desc->bInterfaceClass);
    return 0;
  }
  LOG_INFO("Interface class verified: VENDOR_SPECIFIC");

  uint16_t const len = sizeof(tusb_desc_interface_t) +
                       itf_desc->bNumEndpoints * sizeof(tusb_desc_endpoint_t);
  LOG_INFO("Calculated descriptor length: %u", len);

  if (max_len < len) {
    LOG_ERROR("Insufficient descriptor length! Need %u, got %u", len, max_len);
    return 0;
  }
  LOG_INFO("Descriptor length verified");

  uint8_t const *p_desc = tu_desc_next(itf_desc);
  LOG_INFO("Opening endpoint pair...");
  LOG_INFO("Previous bulk_out: 0x%02x, bulk_in: 0x%02x", _bulk_out, _bulk_in);

  if (!usbd_open_edpt_pair(rhport, p_desc, 2, TUSB_XFER_BULK, &_bulk_out,
                           &_bulk_in)) {
    LOG_ERROR("Failed to open endpoint pair!");
    return 0;
  }
  LOG_INFO("Endpoint pair opened successfully");
  LOG_INFO("New bulk_out: 0x%02x, bulk_in: 0x%02x", _bulk_out, _bulk_in);

  LOG_INFO("Initializing DLN2 subsystem...");
  if (!dln2_init(rhport, _bulk_out, _bulk_in)) {
    LOG_ERROR("DLN2 initialization failed!");
    return 0;
  }
  LOG_INFO("DLN2 initialized successfully");

  LOG_WARN("\n\n=== DLN2 Driver Open Complete ===\n");
  return len;
}

static bool driver_control_xfer_cb(uint8_t rhport, uint8_t stage,
                                   tusb_control_request_t const *req) {
  LOG_INFO("=== driver_control_xfer_cb() called ===");
  LOG_INFO("rhport: %u, stage: %u", rhport, stage);
  LOG_INFO("bmRequestType: 0x%02x", req->bmRequestType);
  LOG_INFO("bRequest: 0x%02x", req->bRequest);
  LOG_INFO("wValue: 0x%04x", req->wValue);
  LOG_INFO("wIndex: 0x%04x", req->wIndex);
  LOG_INFO("wLength: %u", req->wLength);
  LOG_WARN("Control transfer not handled, returning false");
  return false;
}

static bool driver_xfer_cb(uint8_t rhport, uint8_t ep_addr,
                           xfer_result_t result, uint32_t xferred_bytes) {
  LOG_INFO("=== driver_xfer_cb() called ===");
  LOG_INFO("rhport: %u", rhport);
  LOG_INFO("ep_addr: 0x%02x", ep_addr);
  LOG_INFO("result: %u (%s)", result,
           result == XFER_RESULT_SUCCESS   ? "SUCCESS"
           : result == XFER_RESULT_FAILED  ? "FAILED"
           : result == XFER_RESULT_STALLED ? "STALLED"
                                           : "UNKNOWN");
  LOG_INFO("xferred_bytes: %lu", xferred_bytes);

  if (result != XFER_RESULT_SUCCESS) {
    LOG_ERROR("Transfer failed with result: %u", result);
    return false;
  }

  if (!xferred_bytes) {
    LOG_WARN("Zero-length packet (ZLP) received");
  }

  if (ep_addr == _bulk_out) {
    LOG_INFO("Processing BULK OUT transfer (0x%02x)", ep_addr);
    bool ret = dln2_xfer_out(xferred_bytes);
    LOG_INFO("dln2_xfer_out() returned: %s", ret ? "true" : "false");
    return ret;
  } else if (ep_addr == _bulk_in) {
    LOG_INFO("Processing BULK IN transfer (0x%02x)", ep_addr);
    bool ret = dln2_xfer_in(xferred_bytes);
    LOG_INFO("dln2_xfer_in() returned: %s", ret ? "true" : "false");
    return ret;
  } else {
    LOG_WARN("Unknown endpoint: 0x%02x (expected 0x%02x or 0x%02x)", ep_addr,
             _bulk_out, _bulk_in);
  }

  return true;
}

static usbd_class_driver_t const _driver_driver[] = {
    {
#if CFG_TUSB_DEBUG >= 2
        .name = "io-board",
#endif
        .init = driver_init,
        .reset = driver_reset,
        .open = driver_open,
        .control_xfer_cb = driver_control_xfer_cb,
        .xfer_cb = driver_xfer_cb,
        .sof = NULL},
};

usbd_class_driver_t const *usbd_app_driver_get_cb(uint8_t *driver_count) {
  LOG_INFO("=== usbd_app_driver_get_cb() called ===");
  LOG_INFO("Current driver_count: %u", *driver_count);
  *driver_count += TU_ARRAY_SIZE(_driver_driver);
  LOG_INFO("New driver_count: %u", *driver_count);
  LOG_INFO("Returning %u driver(s)", TU_ARRAY_SIZE(_driver_driver));
  return _driver_driver;
}

void sanity_test(void) {
  LOG_DEBUG("Sanity test: dln2_xfer_out returns %s\n",
            dln2_xfer_out(0) ? "true" : "false");
}
