/*
 * canfilter_usb.cpp
 *
 * Implements USB interaction for programming CAN filters on compatible devices.
 *
 * Responsibilities:
 * - Discover and open devices using VID:PID (with optional serial number).
 * - Query device capabilities and determine hardware filter availability.
 * - Program filter configuration to the device via USB control transfers.
 *
 * Notes:
 * - Uses libusb-1.0 API for cross-platform USB communication.
 * - Supports vendor-specific CAN filter USB requests defined in gs_usb_breq.
 * - Dependent on device firmware supporting gs_usb SET_FILTER.
 */

#include "canfilter_usb.hpp"
#include <libusb-1.0/libusb.h>

#define CANDLE_USB_CTRL_IN (LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE | LIBUSB_ENDPOINT_IN)
#define CANDLE_USB_CTRL_OUT (LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE | LIBUSB_ENDPOINT_OUT)

#define GS_CAN_FEATURE_FILTER (1 << 16)

// structures and enums (same as in candlelight_fw)
enum gs_usb_breq {
    GS_USB_BREQ_HOST_FORMAT = 0,
    GS_USB_BREQ_BITTIMING,
    GS_USB_BREQ_MODE,
    GS_USB_BREQ_BERR,
    GS_USB_BREQ_BT_CONST,
    GS_USB_BREQ_DEVICE_CONFIG,
    GS_USB_BREQ_TIMESTAMP,
    GS_USB_BREQ_IDENTIFY,
    GS_USB_BREQ_GET_USER_ID, // not implemented
    GS_USB_BREQ_SET_USER_ID, // not implemented
    GS_USB_BREQ_DATA_BITTIMING,
    GS_USB_BREQ_BT_CONST_EXT,
    GS_USB_BREQ_SET_TERMINATION,
    GS_USB_BREQ_GET_TERMINATION,
    GS_USB_BREQ_GET_STATE,
    GS_USB_BREQ_SET_FILTER,
    GS_USB_BREQ_GET_FILTER,
    __GS_USB_BREQ_PLACEHOLDER_17,
    __GS_USB_BREQ_PLACEHOLDER_18,
    __GS_USB_BREQ_PLACEHOLDER_19,
    GS_USB_BREQ_ELM_GET_BOARDINFO = 20,
    GS_USB_BREQ_ELM_SET_FILTER,
    GS_USB_BREQ_ELM_GET_LASTERROR,
    GS_USB_BREQ_ELM_SET_BUSLOADREPORT,
    GS_USB_BREQ_ELM_SET_PINSTATUS,
    GS_USB_BREQ_ELM_GET_PINSTATUS,
};

struct gs_device_capability {
    uint32_t feature;
    uint32_t fclk_can;
    uint32_t tseg1_min;
    uint32_t tseg1_max;
    uint32_t tseg2_min;
    uint32_t tseg2_max;
    uint32_t sjw_max;
    uint32_t brp_min;
    uint32_t brp_max;
    uint32_t brp_inc;
} __attribute__((packed));

struct gs_filter_info {
    uint8_t dev;
    uint8_t reserved[3];
} __attribute__((packed)) __attribute__((aligned(4)));

bool canfilter_usb::open() {
    USBDEVICE_LOG("Scanning CAN filter VIDs/PIDs");
    return open_from_list(default_vid_pid_list_);
}

bool canfilter_usb::open(uint16_t vid, uint16_t pid, std::string serial) {
    return open_vid_pid(vid, pid, serial);
}

bool canfilter_usb::hasHardwareFilter() {
    if (!handle_ && !open())
        return false;

    gs_device_capability cap{};
    int ret = libusb_control_transfer((libusb_device_handle *)handle_, CANDLE_USB_CTRL_IN, GS_USB_BREQ_BT_CONST, 0, 0,
                                      (unsigned char *)&cap, sizeof(cap), 1000);

    return ret == sizeof(cap) && (cap.feature & GS_CAN_FEATURE_FILTER);
}

uint32_t canfilter_usb::getFilterInfo() {
    if (!handle_ && !open())
        return 0;

    gs_filter_info finfo{};
    int ret = libusb_control_transfer((libusb_device_handle *)handle_, CANDLE_USB_CTRL_IN, GS_USB_BREQ_GET_FILTER, 0, 0,
                                      (unsigned char *)&finfo, sizeof(finfo), 1000);

    if (ret != sizeof(finfo))
        return 0;

    return finfo.dev;
}

bool canfilter_usb::programFilter(const void *config, uint32_t size) {
    if (!handle_ && !open())
        return false;

    int ret = libusb_control_transfer((libusb_device_handle *)handle_, CANDLE_USB_CTRL_OUT, GS_USB_BREQ_SET_FILTER, 0,
                                      0, (unsigned char *)config, size, 1000);

    return ret == (int)size;
}
