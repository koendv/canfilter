/*
 * usb_device.cpp
 *
 * Provides a lightweight USB device abstraction using libusb.
 *
 * Responsibilities:
 * - Initialize and clean up libusb context.
 * - Open USB devices by VID:PID, optionally matching serial number.
 * - Handle Linux kernel driver detachment and reattachment if necessary.
 * - Claim/release USB interface and manage handle lifecycle.
 * - Provide functions to close devices safely and free resources.
 *
 * Notes:
 * - Designed for synchronous USB operations.
 * - Used as a base class for canfilter_usb to abstract USB device handling.
 */

#include "usb_device.hpp"

#ifdef __linux__
#define USE_LINUX_KERNEL_DRIVER
#endif

usb_device::usb_device() {
    libusb_context *ctx = nullptr;
    if (libusb_init(&ctx) == 0) {
        context_ = ctx;
        USBDEVICE_LOG("libusb initialized");
    } else {
        USBDEVICE_LOG("libusb_init failed");
    }
}

usb_device::~usb_device() {
    close();
    if (context_) {
        libusb_exit((libusb_context *)context_);
        USBDEVICE_LOG("libusb exited");
    }
}

void usb_device::close() {
    if (handle_) {
        USBDEVICE_LOG("Closing device");

        libusb_release_interface((libusb_device_handle *)handle_, 0);

#ifdef USE_LINUX_KERNEL_DRIVER
        if (driver_detached_) {
            libusb_attach_kernel_driver((libusb_device_handle *)handle_, 0);
            USBDEVICE_LOG("Kernel driver reattached");
            driver_detached_ = false;
        }
#endif

        libusb_close((libusb_device_handle *)handle_);
        handle_ = nullptr;
    }
}

bool usb_device::open_vid_pid(uint16_t vid, uint16_t pid, const std::string &serial) {
    close();
    if (!context_)
        return false;

    libusb_device **devs = nullptr;
    ssize_t cnt = libusb_get_device_list((libusb_context *)context_, &devs);
    if (cnt < 0)
        return false;

    for (ssize_t i = 0; i < cnt; ++i) {
        libusb_device *dev = devs[i];
        libusb_device_descriptor desc{};

        if (libusb_get_device_descriptor(dev, &desc) != 0)
            continue;

        if (desc.idVendor != vid || desc.idProduct != pid)
            continue;

        libusb_device_handle *h = nullptr;
        if (libusb_open(dev, &h) != 0)
            continue;

        if (!serial.empty() && desc.iSerialNumber > 0) {
            unsigned char buf[256];
            int len = libusb_get_string_descriptor_ascii(h, desc.iSerialNumber, buf, sizeof(buf));
            buf[len] = '\0';
            if (len < 0 || serial != (char *)buf) {
                libusb_close(h);
                continue;
            }
        }

#ifdef USE_LINUX_KERNEL_DRIVER
        driver_detached_ = false;
        if (libusb_kernel_driver_active(h, 0) == 1) {
            if (libusb_detach_kernel_driver(h, 0) == 0) {
                driver_detached_ = true;
                USBDEVICE_LOG("Kernel driver detached");
            }
        }
#endif

        if (libusb_claim_interface(h, 0) != 0) {
            USBDEVICE_LOG("Failed to claim interface 0");
            libusb_close(h);
            continue;
        }

        USBDEVICE_LOG("Opened device VID=0x" << std::hex << vid << " PID=0x" << pid);

        handle_ = h;
        break;
    }

    libusb_free_device_list(devs, 1);
    return handle_ != nullptr;
}

bool usb_device::open_from_list(const std::vector<std::pair<uint16_t, uint16_t>> &list) {
    for (auto &vp : list)
        if (open_vid_pid(vp.first, vp.second))
            return true;

    return false;
}
