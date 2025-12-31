# CanFilter

**canfilter** is a cross-platform command-line tool and C++ library for managing hardware filters on patched [candlelight](https://github.com/koendv/candleLight_fw) USB-CAN adapters.
**canfilter** accepts human-readable CAN bus IDs and ID ranges and converts them into valid hardware filter configurations. **canfilter** runs on both Linux and Windows.

## Features

- Add single CAN IDs or ranges (standard 11-bit or extended 29-bit).
- Output filter configurations in multiple formats:
    - bxCAN (STM32F0/F3/F4 hardware)
    - FDCAN (STM32G0/H7 hardware)
- Verbose output mode.
- Cross-platform support (Linux and Windows).

## Usage

```
canfilter [options] [IDs and ranges]
```
IDs: Single CAN IDs (0x100, 256, 0x1000)

RANGES: CAN ID ranges (0x100-0x1FF, 256-511, 0x1000-0x1FFF)

| Short               | Long                   | Description                                                   |
| ------------------- | ---------------------- | ------------------------------------------------------------- |
| -o MODE             | --output MODE          | Set output mode: auto, bxcan_f0, bxcan_f4, fdcan_g0, fdcan_h7 |
| -a                  | --allow-all            | Allow all packets                                             |
| -v                  | --verbose              | Enable verbose output                                         |
| -u VID:PID[@SERIAL] | --usb VID:PID[@SERIAL] | Vendor id, product id, and serial of usb adapter              |
| -h                  | --help                 | Show this help                                                |

- Single IDs are interpreted as standard if <= 0x7FF, extended if <= 0x1FFFFFFF.
- Hex numbers are supported (prefix `0x`).
- Ranges are interpreted as extended if lower or upper bound is an extended ID.
- Repeating -v up to three times increases verbosity.
- CAN controllers for --output:
    - auto: ask CAN controller (default)
    - bxcan_f0: single classic CAN controller, 14 filter banks.
    - bxcan_f4: dual classic CAN controller, 28 filter banks.
    - fdcan_g0: CAN FD controller, 28 standard filters, 8 extended filters.
    - fdcan_h7: CAN FD controller, 128 standard filters, 28 extended filters.

## Examples

Allow a single standard ID:

```
canfilter 0x100
```

Program a standard ID range:

```bash
canfilter 0x100-0x1FF
```

Program mixed standard and extended IDs:

```bash
canfilter 0x100 0x200-0x2FF 0x1000 -v
```

Print bxcan registers without programming hardware:

```
canfilter -o bxcan_f0 -d -v -v -v 0x1000-0x1fff
```

## Client side

On the client side, canfilter requires a [patched candleLight](https://github.com/koendv/candleLight_fw) USB-CAN adapter.

The patch adds two capabilities:

- Allow the PC to inquire whether a hardware filter is available, and if so, which type.
- Allow the PC to send a hardware filter configuration to the adapter.

## FDCAN and BXCAN comparison

FDCAN (Flexible CAN) and BXCAN (Basic CAN) are two types of CAN hardware with different approaches to message filtering. This comparison highlights how each handles filtering ranges and their respective resource usage, as well as a comparison of the FDCAN implementation on the STM32G0 and STM32H7 microcontrollers.

### FDCAN Filtering

FDCAN supports native range filtering, allowing you to filter a continuous range of CAN IDs using a single filter bank. For example, the range 0x101-0x1fe can be filtered with just one filter bank:

```
$ canfilter -d -o fdcan_g0 0x101-0x1fe
Filter usage: 1/28 standard (4%), 0/8 extended (0%)
```

This method is efficient and uses minimal resources.

### BXCAN Filtering

In contrast, BXCAN uses mask-based filtering, meaning you cannot directly filter a range of CAN IDs. Instead, BXCAN breaks the range into a set of IDs with corresponding masks. This results in more filters being needed for the same range. For instance, filtering the range 0x101-0x1fe involves multiple filter IDs and masks:

```
$ canfilter -d -o bxcan_f0 0x101-0x1fe
Filter usage: 7/14 (50%)
```

As seen, BXCAN requires more filter banks for this operation compared to FDCAN.

### Resource Usage Comparison

Filtering the same range on BXCAN and FDCAN shows a significant difference in resource usage. FDCAN uses a single filter bank for a range, while BXCAN requires multiple filters, especially when the range is not aligned to powers of two.

For example:

- Filtering the range 0x101-0x1fe on BXCAN uses 50% of the filter capacity, while FDCAN uses only 4%.
- Filtering the range 0x100-0x1ff on BXCAN uses 7%, whereas FDCAN uses 4%.

|range       |bxcan_f0 |fdcan_g0 |
| ---------- | ------- | ------- |
|0x101-0x1fe |50%      |4%       |
|0x100-0x1ff |7%       |4%       |

BXCAN performs best when the range is a power of two and begins on a power-of-two boundary. If you find yourself running out of filter banks on BXCAN, consider adjusting your filters to fit these optimal conditions.

### FDCAN on STM32G0 vs. FDCAN on STM32H7

The number of available filters differs significantly between the STM32G0 and STM32H7 microcontrollers:

|                 |standard |extended |
| --------------- | ------- | ------- |
|FDCAN on STM32G0 |28       |8        |
|FDCAN on STM32H7 |128      |64       |

The STM32H7 offers significantly more filters than the STM32G0, making it better suited for applications with more complex CAN filtering needs.

## Building

Prerequisites to build _canfilter_:

- C++11 compiler(e.g. g++)
- libusb-1.0 development libraries
- make (e.g. gnu make)

To build, simply run _make_:

```bash
make
```

## Notes

The core idea behind **canfilter** is that CAN bus hardware filters (ID + mask) are mathematically equivalent to IP network blocks (network + prefix).

Because of this equivalence, we can reuse well-established IP networking logic for CAN bus filtering. In particular, **CIDR aggregation**—a mature algorithm for converting IP addresses and ranges into minimal sets of network blocks—can be applied directly to CAN identifiers. When used with CAN bus data, the CIDR algorithm transforms lists of CAN IDs and ID ranges into an optimized set of hardware filter entries.

The masks generated by **canfilter** follow the same structure as IP network masks:

- A contiguous run of 1-bits from the most significant bit, marking the portion of the CAN ID that must match.
- Followed by a contiguous run of 0-bits, representing “don’t care” bits that may vary.
- A mask of all zeroes matches **any** CAN ID.

## Use with other applications

Cangaroo is a CAN bus monitoring and analysis tool. On Linux, it accesses candleLight USB-CAN adapters through SocketCAN.

canfilter is a command-line utility used to configure hardware filters on a USB-CAN adapter. Unlike Cangaroo, canfilter communicates with candleLight devices directly over libusb.

It is often useful to use Cangaroo to analyze CAN traffic while using canfilter to reduce the number of packets reaching the host.

However, when canfilter applies a hardware filter, it temporarily detaches the USB-CAN device from the kernel driver. If the USB-CAN device is currently in use by Cangaroo, this interrupts the connection that Cangaroo is using.

To avoid canfilter disconnecting Cangaroo, the following workaround:

- First *Stop measurement* in Cangaroo
- Set the new hardware filter using canfilter
- Then *Start measurement* in Cangaroo.

A long-term solution would involve two improvements:

- A linux gs_usb ioctl() to set hardware filters from user space, without having to detach the kernel driver first.
- Integrating canfilter into Cangaroo. A demo of [adding a hardware-filter](http://github.com/koendv/cangaroo) configuration option to the Cangaroo _Setup_ window.

## Bugs

[Bug report procedure](BUGS.md)

## Links

- candlelight [hardware filtering](https://github.com/candle-usb/candleLight_fw/pull/204) pull request
- candlelight [binaries](https://github.com/koendv/candleLight_fw) for STM32F072 with hardware filtering
- candlelight for [WeAct USB2CANFD V1](https://github.com/koendv/candleLight_fw_weact_usb2canfdv1) STM32G0B1 with hardware filtering
- [cangaroo for linux](https://github.com/koendv/cangaroo) with hardware filtering

## License

This project (including all software, hardware designs, documentation, and other creative content) is dedicated to the public domain under the Creative Commons Zero ([CC0](https://creativecommons.org/publicdomain/zero/1.0/)) 1.0 Universal Public Domain Dedication.

### What this means

- Use this project for **any purpose**, personal or commercial
- Modify, adapt, and build upon the work
- Distribute your modifications **without any requirements** to:
    - Provide attribution
    - Share your changes
    - Use the same license
- No permission is needed - **just use it!**

### No Warranty

This project is provided **"as is"** without any warranties. The author(s) accept no liability for any damages resulting from its use.
