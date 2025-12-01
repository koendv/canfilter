# CanFilter

Note: at the moment this is still in testing.

**CanFilter** is a command-line tool for managing hardware filters on patched [candlelight](https://github.com/koendv/candleLight_fw) USB-CAN adapters.

_canfilter_ takes human-readable CAN bus ID's and ID ranges and converts them into CAN bus hardware filter configurations.

_canfilter_ is a cross-platform command-line tool and C library for managing CAN bus hardware filters. _canfilter_ supports Linux and Windows.

## Features

- Add single CAN IDs or ranges (standard 11-bit or extended 29-bit).
- Output filter configurations in multiple formats:
    - bxCAN (STM32F0/STM32F4 hardware)
    - FDCAN (STM32G0/STM32H7 hardware)
- Verbose output mode.
- Cross-platform support (Linux and Windows).

## Usage

```
canfilter [options] [IDs and ranges]
```
IDs: Single CAN IDs (0x100, 256, 0x1000)

RANGES: CAN ID ranges (0x100-0x1FF, 256-511, 0x1000-0x1FFF)

| Short   | Long          | Description                                |
| ------- | ------------- | ------------------------------------------ |
| -o MODE | --output MODE | Set output mode: bxcan, fdcan_g0, fdcan_h7 |
| -a      | --allow-all   | Allow all packets                          |
| -v      | --verbose     | Enable verbose output                      |
| -h      | --help        | Show this help                             |


- Single IDs are interpreted as standard if <= 0x7FF, extended if <= 0x1FFFFFFF.
- Hex numbers are supported (prefix `0x`).
- Ranges are interpreted as extended if lower or upper bound is an extended ID.
- Repeating -v up to three times results in more verbose output.

## Examples

Add a single standard ID:

```
canfilter 0x100 -o bxcan
```

Add multiple IDs and ranges for bxCAN:

```
canfilter 0x100-0x1FF 0x1ABCDE -o bxcan
```

## FDCAN and BXCAN comparison

FDCAN (Flexible CAN) and BXCAN (Basic CAN) are two types of CAN hardware with different approaches to message filtering. This comparison highlights how each handles filtering ranges and their respective resource usage, as well as a comparison of the FDCAN implementation on the STM32G0 and STM32H7 microcontrollers.

### FDCAN Filtering

FDCAN supports native range filtering, allowing you to filter a continuous range of CAN IDs using a single filter bank. For example, the range 0x101-0x1fe can be filtered with just one filter bank:

```
$ canfilter -o fdcan_g0 0x101-0x1fe
Filter usage: 1/28 standard (4%), 0/8 extended (0%)
```

This method is efficient and uses minimal resources.

### BXCAN Filtering

In contrast, BXCAN uses mask-based filtering, meaning you cannot directly filter a range of CAN IDs. Instead, BXCAN breaks the range into a set of IDs with corresponding masks. This results in more filters being needed for the same range. For instance, filtering the range 0x101-0x1fe involves multiple filter IDs and masks:

```
$ canfilter -o bxcan 0x101-0x1fe
Filter usage: 7/14 (50%)
```

As seen, BXCAN requires more filter banks for this operation compared to FDCAN.

### Resource Usage Comparison

Filtering the same range on BXCAN and FDCAN shows a significant difference in resource usage. FDCAN uses a single filter bank for a range, while BXCAN requires multiple filters, especially when the range is not aligned to powers of two.

For example:

- Filtering the range 0x101-0x1fe on BXCAN uses 50% of the filter capacity, while FDCAN uses only 4%.
- Filtering the range 0x100-0x1ff on BXCAN uses 7%, whereas FDCAN uses 4%.

|range       |bxcan |fdcan_g0 |
| ---------- | ---- | ------- |
|0x101-0x1fe |50%   |4%       |
|0x100-0x1ff |7%    |4%       |

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

- C++ compiler, e.g. g++
- libusb-1.0 development libraries
- CMake, gnu make

Build:

```bash
mkdir build && cd build
cmake ..
make
```

## Notes

The key insight behind the _canfilter_ tool is that CANBUS hardware filters (ID + mask) are mathematically equivalent to IP network blocks (network + prefix).

This means IP networking code can be applied to CANBUS. CIDR aggregation is a mature IP networking algorithm that converts IP addresses and address ranges into a list of IP networks. Applied to CANBUS, the CIDR algorithm rewrites a list of CANBUS addresses and address ranges into a list of hardware filters.

## Bugs

[Bug report procedure](BUGS.md)

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
