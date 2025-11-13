# canfilter

This is canfilter, a tool to generate and verify optimal CAN bus hardware filters for CAN bus controllers.
The tool runs on Windows, linux and embedded, in the usb to CAN adapter itself.

_canfilter_ takes human-readable ID ranges and converts them into efficient hardware filter configurations.

## AI

I would like to comment on the use of AI in software.

The _canfilter_ tool was developed with assistance from [deepseek](https://deepseek.com) AI.

The key insight behind the _canfilter_ tool is that CANBUS hardware filters (ID + mask) are mathematically equivalent to IP network blocks (network + prefix).

This means IP networking code can be applied to CANBUS. [CIDR aggregation](https://github.com/koendv/canfilter/tree/main/cidr) is a mature IP networking algorithm to group IP addresses and address ranges into a minimal list of IP networks. Applied to CANBUS, the CIDR aggregation algorithm rewrites a list of CANBUS addresses and address ranges into a minimal list of hardware filters. Minimizing the number of hardware filters is useful, because hardware resources cost.

Usually an idea stays just that, an idea. AI enables rapid prototyping, transforming an idea into working code. I consider this project a 'proof of concept' that demonstrates the algorithm's practical application.

For production industrial use, I would recommend code optimized by hand, using the YAML file as reference.

## Command Options

_canfilter_ is a command line tool with the following options:

```text
Generate optimal CAN bus hardware filters using CIDR route aggregation

Filter Definition Options:
  --std          Use 11-bit standard IDs (default)
  --ext          Use 29-bit extended IDs
  --data         Filter data frames only (default)
  --rtr          Filter remote frames only

Output Control Options:
  --output FORMAT  Output format: stm, slcan, hal, embedded
  --mask          Force mask mode for all filters
  --list          Enable list mode optimization (default)
  --max N         Maximum number of filters (default: platform dependent)
  --verbose       Verbose output showing algorithm details

Testing and Verification Options:
  --test ID...    Test specific IDs against generated filters
  --selftest      Run built-in self-test
```

The [man page](canfilter.md) explains each option in detail; a concise overview follows.

## Desktop example

canfilter on linux:

```
$ canfilter --std --output stm 0x100 0x200-0x20F
STM32 Hardware Format:
ID=0x00000100 MASK=0x000007FF IDE=0 RTR=0 MODE=16BIT SCALE=16BIT FILTER=LIST
ID=0x00000200 MASK=0x000007F0 IDE=0 RTR=0 MODE=16BIT SCALE=32BIT FILTER=MASK
```

## Embedded example:

_canfilter_ as an rt-thread shell command, running on at32f405:
```
msh />canfilter --std --mask --output stm 0x100 0x200-0x20F
STM32 Hardware Format:
ID=0x00000100 MASK=0x000007FF IDE=0 RTR=0 MODE=16BIT SCALE=32BIT FILTER=MASK
ID=0x00000200 MASK=0x000007F0 IDE=0 RTR=0 MODE=16BIT SCALE=32BIT FILTER=MASK
msh />canfilter --std --mask --output embedded 0x100 0x200-0x20F
Successfully applied 2 filters to CAN hardware
```

## Testing

Selftest and test options allow pre-production testing.
Please test before use.

Self-test verifies algorithm correctness against known good results:
```
$ canfilter --selftest
Running canfilter self-test...
Self-test: 6/6 passed
All self-tests passed!
```

Test verifies filter correctness for specified IDs and ID ranges:
```
$ canfilter --std --output stm 0x200-0x2FF --test 0x200
STM32 Hardware Format:
ID=0x00000200 MASK=0x00000700 IDE=0 RTR=0 MODE=16BIT SCALE=32BIT FILTER=MASK

Test Results:
  ID 0x200: PASS
Test summary: 1/1 passed
```

These commands run in both desktop and embedded.
_canfilter_ is also available as C library.

## Output format

Output format is

- STM registers
- HAL C source
- SLCAN
- setting CAN controller hardware filters directly (embedded only).

The SLCAN format is an [SLCAN filter extension](canfilter.md#slcan-format).

## Algorithm

The algorithm used is CIDR aggregation, an IP networking algorithm.

- Single CAN IDs treated as host routes (/32 equivalent)
- CAN ID ranges decomposed into optimal CIDR blocks
- CIDR aggregation produces minimal filter count
- Subset elimition removes nested networks (any filter completely covered by a broader filter)
- CIDR output is divided into host routes and networks.
    - Host routes with a netmask of all 1's correspond to CAN bus filter lists.
    - Networks correspond to CAN bus filter masks

## Bug Report Procedure

If you find a bug, please follow these steps before opening a GitHub issue:

- Reproduce with Verbose Output

``` bash
canfilter --verbose --selftest
canfilter --verbose [your original command that shows the bug]
```

- Test with Different Output Formats

``` bash
canfilter --output stm [your ranges]
canfilter --output slcan [your ranges]
canfilter --output hal [your ranges]
```

- Use DeepSeek for Initial Analysis

    - Log in to DeepSeek Chat
    - Upload these files:
        - canfilter.c (source code)
        - canfilter.yaml (specification)
        - canfilter.md (man page)

- Provide:
    - The exact command that shows the bug
    - The verbose output from step 1
    - What you expected vs what you got

- If DeepSeek Cannot Solve It

    Open a GitHub issue with:

    - All the information from above
    - Your DeepSeek conversation summary
    - Any potential fixes you identified

- Bonus: Proposed Fix

    If you found a solution:

    - Create a pull request with the fix
    - Include a test case that would have caught the bug
    - Update documentation if needed

    Example DeepSeek Prompt:

```text
I found a bug in canfilter. Here are the source files.
Command: canfilter --std 0x100-0x10F
Expected: One filter covering 0x100-0x10F
Actual: Two filters with gaps
Verbose output: [paste here]
Can you help identify the issue in the CIDR aggregation?
```

Note deepseek is able to answer questions like:

```text
Please estimate stack size on arm32.
```

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
