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
: Output mode: `bxcan`, `fdcan_g0`, `fdcan_h7` (default: `bxcan`)

**-v**, **--verbose**  
: Enable verbose output

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

Program a standard ID range to bxCAN hardware:

```bash
canfilter 0x100-0x1FF -o bxcan
```

Program mixed standard and extended IDs to FDCAN:

```bash
canfilter 0x100 0x200-0x2FF 0x1000 -o fdcan_g0 -v
```

Perform a dry run without programming hardware:

```bash
canfilter 0x101-0x1FE -o bxcan --dry-run -v
```

## HARDWARE SUPPORT

- **bxCAN**  
    - STM32F1, STM32F4 series  
    - 14 filter banks  
    - Filters are masks, not ranges  
    - Automatic conversion from ranges to masks
- **FDCAN Small**  
    - STM32G0 series  
    - 28 standard + 8 extended filters  
    - Native range filter support
- **FDCAN Large**  
    - STM32H7 series  
    - 128 standard + 64 extended filters  
    - Native range filter support

## EXIT STATUS

0 if successful, non-zero on error.

## BUGS

Report bugs at: [https://github.com/koendv/canfilter/](https://github)

## AUTHORS

Koen De Vleeschauwer

## COPYRIGHT

CC0 Public Domain — see [https://github.com/koendv/canfilter/LICENSE.md](LICENSE) for details

