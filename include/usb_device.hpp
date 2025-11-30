#ifndef USB_DEVICE_H
#define USB_DEVICE_H

// usb_device
//
// Provides a cross-platform abstraction over libusb for enumerating and
// connecting to USB devices. Handles initialization of the libusb context,
// device opening by VID/PID or from a list, and safe cleanup on close.
//
// Key features:
//   • open_vid_pid() – open a device using a specific vendor and product ID
//   • open_from_list() – attempt to open a device from a list of VID/PID pairs
//   • close() – cleanly release the device handle and libusb context
//
// Platform-specific behavior (e.g., detaching the kernel driver on Linux) is
// managed internally. The class stores opaque pointers to the libusb context
// and device handle, but does not perform any higher-level device-specific
// operations; derived classes implement protocol-specific logic.

#include <cstdint>
#include <libusb-1.0/libusb.h>
#include <string>
#include <vector>

#ifdef USBDEVICE_ENABLE_LOGGING
#define USBDEVICE_LOG(msg)                                                                                             \
    do {                                                                                                               \
        std::cerr << "[usb_device] " << msg << std::endl;                                                              \
    } while (0)
#else
#define USBDEVICE_LOG(msg)                                                                                             \
    do {                                                                                                               \
    } while (0)
#endif

class usb_device {
  public:
    usb_device();
    virtual ~usb_device();

    bool open_vid_pid(uint16_t vid, uint16_t pid, const std::string &serial = "");
    bool open_from_list(const std::vector<std::pair<uint16_t, uint16_t>> &list);

    void close();

  protected:
    void *context_ = nullptr;      // libusb_context*
    void *handle_ = nullptr;       // libusb_device_handle*
    bool driver_detached_ = false; // Linux only
};

#endif
