# CanFilter

**CanFilter** is a command-line tool for managing hardware filters on patched [candlelight](https://github.com/koendv/candleLight_fw) USB-CAN adapters.

_canfilter_ takes human-readable CAN bus ID's and ID ranges and converts them into CAN bus hardware filter configurations.

_canfilter_ is a cross-platform command-line tool and C library for managing CAN bus hardware filters. It supports Linux and Windows.

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

| Short   | Long          | Description                                      |
| ------- | ------------- | ------------------------------------------------ |
| -o MODE | --output MODE | Set output mode: bxcan, fdcan_g0, fdcan_h7 |
| -v      | --verbose     | Enable verbose output                            |
| -h      | --help        | Show this help                                   |

- Single IDs or ranges without options are interpreted as standard if <= 0x7FF, extended if <= 0x1FFFFFFF.
- Hex numbers are supported (prefix `0x`).
- Ranges are interpreted as extended if lower or upper bound is an extended ID.

## Examples

Add a single standard ID and show in text:

```
canfilter -s 0x100 -o bxcan
```

Add multiple IDs and ranges for bxCAN:

```
canfilter -S 0x100-0x1FF -e 0x1ABCDE -o bxcan
```

An example to illustrate the difference between FDCAN and BXCAN filtering. FDCAN has native range filtering hardware. The range 0x101-0x1fe is converted to a single filter bank:

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
bxcan debug:
bank [ 0]: std mask 0x102-0x103, 0x104-0x107
bank [ 1]: std mask 0x108-0x10f, 0x110-0x11f
bank [ 2]: std mask 0x120-0x13f, 0x140-0x17f
bank [ 3]: std mask 0x180-0x1bf, 0x1c0-0x1df
bank [ 4]: std mask 0x1e0-0x1ef, 0x1f0-0x1f7
bank [ 5]: std mask 0x1f8-0x1fb, 0x1fc-0x1fd
bank [ 6]: std list 0x101, 0x1fe, 0x101, 0x101
Device features: 0xFB, filter support: YES
operation completed successfully

```

## Notes

The key insight behind the _canfilter_ tool is that CANBUS hardware filters (ID + mask) are mathematically equivalent to IP network blocks (network + prefix).

This means IP networking code can be applied to CANBUS. CIDR aggregation is a mature IP networking algorithm to convert IP addresses and address ranges into a list of IP networks. Applied to CANBUS, the CIDR algorithm rewrites a list of CANBUS addresses and address ranges into a list of hardware filters.

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
