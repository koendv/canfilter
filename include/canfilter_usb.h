#ifndef CANFILTER_USB_H
#define CANFILTER_USB_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>    // for uint32_t, uint8_t, etc.
#include <libusb-1.0/libusb.h>  // libusb header file

// USB Vendor and Product IDs
#define GS_USB_VENDOR_ID 0x1D50
#define GS_USB_PRODUCT_ID 0x606F
#define GS_USB_BREQ_SET_USER_ID 9

// Function to send configuration to the USB device
bool canfilter_send_usb(const void *config, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif  // CANFILTER_USB_H
