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


- Single IDs or ranges without options are interpreted as standard if <= 0x7FF, extended if <= 0x1FFFFFFF.
- Hex numbers are supported (prefix `0x`).
- Ranges are interpreted as extended if lower or upper bound is an extended ID.
- Repeating -v up to three times results in more verbose output.

## Examples

Add a single standard ID and show in text:

```
canfilter 0x100 -o bxcan
```

Add multiple IDs and ranges for bxCAN:

```
canfilter 0x100-0x1FF 0x1ABCDE -o bxcan
```
## FDCAN and BXCAN comparison

An example to illustrate the difference between FDCAN and BXCAN filtering.

FDCAN has native range filtering hardware. The range 0x101-0x1fe is converted to a single filter bank:

```
$ ./canfilter -v -o fdcan_g0 0x101-0x1fe
fdcan debug
sf[ 0]: range 0x101 0x1fe fifo0
```

BXCAN filters are masks, not ranges. The range 0x101-0x1fe is converted to 2 id's and 12 masks:

```
$ canfilter -v -o bxcan 0x101-0x1fe
bxcan std list id 0x101
bxcan std mask id 0x102 mask 7fe
bxcan std mask id 0x104 mask 7fc
bxcan std mask id 0x108 mask 7f8
bxcan std mask id 0x110 mask 7f0
bxcan std mask id 0x120 mask 7e0
bxcan std mask id 0x140 mask 7c0
bxcan std mask id 0x180 mask 7c0
bxcan std mask id 0x1c0 mask 7e0
bxcan std mask id 0x1e0 mask 7f0
bxcan std mask id 0x1f0 mask 7f8
bxcan std mask id 0x1f8 mask 7fc
bxcan std mask id 0x1fc mask 7fe
bxcan std list id 0x1fe
Device features: 0xFB, filter support: YES
operation completed successfully
```

By comparison, filtering the range 0x100-0x1ff on bxcan uses only half a filter bank:

```
$ canfilter 0x100-0x1ff -v
bxcan std mask id 0x100 mask 700
Device features: 0xFB, filter support: YES
operation completed successfully
```

BXCAN is very efficient if a range is a power of two wide, and the range begins on a multiple of a power of two. If you run out of filter banks on bxcan, try rewriting your filters to align with powers of two.

Now comparing FDCAN on STM32G0 and STM32H7:

| filters  | standard | extended |
| -------- | -------- | -------- |
| fdcan_g0 | 28       | 8        |
| fdcan_h7 | 128      | 64       |

The difference between FDCAN on STM32G0 and STM32H7 is the number of filters available.

## Building

Prerequisites:

- C++20 compiler
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
