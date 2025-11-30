# CANFILTER

## NAME

**canfilter** — CAN bus hardware filter generator

## SYNOPSIS

**canfilter** [*OPTIONS*] [*IDs/RANGES*]

## DESCRIPTION

**canfilter** generates and programs hardware CAN filters for various CAN controllers.

The tool supports individual CAN IDs and ID ranges, automatically converting ranges to optimal hardware filter configurations.

## OPTIONS

**-o**, **--output** *MODE*
: Output mode: `auto`, `bxcan_f0`, `bxcan_f4`,  `fdcan_g0`, `fdcan_h7` (default: `auto`)

**-a**, **--allow-all**
: Allow all packets

**-v**, **--verbose**
: Enable verbose output

**-u**, **--usb** *VID:PID[@SERIAL]
: Vendor id, product id and optional serial number of CAN adapter. Vendor id and product id in hex. 

**-d**, **--dry-run**
: Parse and display filter configuration without programming hardware

**-h**, **--help**
: Show this help message

## ID/RANGE FORMAT

CAN IDs and ranges can be specified in hexadecimal (`0x100`) or decimal (`256`) format. Multiple IDs/ranges can be separated by commas or spaces.

*Single IDs*
: `0x100`, `256`, `0x1FFFFFFF`

*Ranges*
: `0x100-0x1FF`, `256-511`, `0x1000-0x1FFF`

*Mixed*
: `0x100,0x200-0x2FF,0x1000` or `0x100 0x200-0x2FF 0x1000`

## EXAMPLES

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

## HARDWARE

STM32 with support for CANBUS classic or CAN FD.

- CANBUS classic:
    - bxcan_f0 (STM32F0, F1, F3, L1, L4: single CAN with 14 filter registers)
    - bxcan_f4 (STM32F4, F7: dual CAN with 28 filter registers)
- CAN FD:
    - fdcan_g0 (STM32G0: 28 standard + 8 extended filters)
    - fdcan_h7 (STM32H7: 128 standard + 64 extended filters)

## ERROR CODES

**CANFILTER_SUCCESS** (0)
: Operation completed successfully

**CANFILTER_ERROR_PARAM** (1)
: Invalid parameter (ID out of range or invalid range)

**CANFILTER_ERROR_FULL** (2)
: No more filter banks available

**CANFILTER_ERROR_PLATFORM** (3)
: USB communication failed or hardware not found

## BUGS

Report bugs at: [https://github.com/koendv/canfilter/](github)

## AUTHORS

Koen De Vleeschauwer

## COPYRIGHT

CC0 Public Domain — see LICENSE for details
