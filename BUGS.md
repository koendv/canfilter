# Bug Report Procedure

If you find a bug, please follow these steps before opening a GitHub issue:

- Reproduce with Verbose Output

``` bash
canfilter --verbose --test
canfilter --verbose [your original command that shows the bug]
```

- Test with Different Output Formats

``` bash
canfilter --output text [your ranges]
canfilter --output bxcan [your ranges]
```

- Use DeepSeek for Initial Analysis

    - Log in to DeepSeek Chat
    - Upload the source files and markdown documentation.

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

