#ifdef __cplusplus
extern "C" {
#endif

#include "canfilter_usb.h"
#include <libusb-1.0/libusb.h>
#include <stdio.h>

// Define the device capability structure
typedef struct {
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
} __attribute__((packed)) gs_device_capability;

// Check if the GS_USB device supports hardware filtering
static bool canfilter_check_hw_support(libusb_device_handle *dev_handle) {
    gs_device_capability cap;

    if (!dev_handle) {
        fprintf(stderr, "ERROR: NULL device handle for feature check\n");
        return false;
    }

    int ret = libusb_control_transfer(dev_handle,
                                      0xC1,    // device-to-host, vendor, interface
                                      4, 0, 0, // wValue, wIndex
                                      (unsigned char *)&cap, sizeof(cap), 1000);

    if (ret != sizeof(cap)) {
        fprintf(stderr, "Failed to read device capabilities: %s\n", libusb_error_name(ret));
        return false;
    }

    bool has_filter_support = (cap.feature & (1 << 6)) != 0; // GS_CAN_FEATURE_USER_ID
    printf("Device features: 0x%X, filter support: %s\n", cap.feature, has_filter_support ? "YES" : "NO");

    return has_filter_support;
}

// Send configuration data to the USB device
bool canfilter_send_usb(const void *config, uint32_t size) {
    libusb_context *ctx = NULL;
    libusb_device_handle *dev_handle = NULL;
    int ret;

    // Initialize libusb
    ret = libusb_init(&ctx);
    if (ret < 0) {
        fprintf(stderr, "Failed to initialize libusb: %s\n", libusb_error_name(ret));
        return false;
    }

    // Find and open GS_USB device
    dev_handle = libusb_open_device_with_vid_pid(ctx, GS_USB_VENDOR_ID, GS_USB_PRODUCT_ID);
    if (!dev_handle) {
        fprintf(stderr, "Failed to find GS_USB device\n");
        libusb_exit(ctx);
        return false;
    }

// Detach kernel driver if active (Linux-specific)
#ifdef __linux__
    if (libusb_kernel_driver_active(dev_handle, 0) == 1) {
        ret = libusb_detach_kernel_driver(dev_handle, 0);
        if (ret < 0) {
            fprintf(stderr, "Warning: Failed to detach kernel driver: %s\n", libusb_error_name(ret));
        }
    }
#endif

    // Claim interface
    ret = libusb_claim_interface(dev_handle, 0);
    if (ret < 0) {
        fprintf(stderr, "Failed to claim interface: %s\n", libusb_error_name(ret));
        libusb_close(dev_handle);
        libusb_exit(ctx);
        return false;
    }

    // Check hardware support
    if (!canfilter_check_hw_support(dev_handle)) {
        fprintf(stderr, "Error: Hardware filter support not available.\n");
        libusb_release_interface(dev_handle, 0);
        libusb_close(dev_handle);
        libusb_exit(ctx);
        return false;
    }

    // Send configuration data
    ret = libusb_control_transfer(dev_handle, 0x41, // host-to-device, vendor, interface
                                  GS_USB_BREQ_SET_USER_ID, 0, 0, (unsigned char *)config, size, 1000);

    // Cleanup
    libusb_release_interface(dev_handle, 0);
#ifdef __linux__
    if (libusb_kernel_driver_active(dev_handle, 0) == 0) {
        libusb_attach_kernel_driver(dev_handle, 0);
    }
#endif
    libusb_close(dev_handle);
    libusb_exit(ctx);

    if (ret != (int)size) {
        fprintf(stderr, "Error sending hardware configuration: sent %d of %u bytes\n", ret, size);
        return false;
    }

    return true;
}

#ifdef __cplusplus
}
#endif
