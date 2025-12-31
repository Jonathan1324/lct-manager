---
name: Bug Report
about: Report a reproducible bug in the assembler/linker
title: "[Bug]: "
labels: bug
---

# Bug Report

Thank you for reporting a bug in the assembler/linker toolchain!  
Please provide as much detail as possible to help us reproduce and fix the issue.

## Environment

- **Host OS & Version:**  
  _e.g. Ubuntu 24.04, macOS 14, Windows 11, WSL2_

- **Tool Version / Commit Hash:**  
  _e.g. v1.2.3, commit abc1234_

- **Target Architecture & Format:**  
  _e.g. x86_64 ELF, ARM64 Mach-O, RISC-V flat binary_

## Bug Description

Describe the problem clearly and concisely. What did you try to do, and what went wrong?

## Steps to Reproduce

Provide a minimal example or commands to reproduce the issue:

```

    1. Write this assembly code:
    
        ```
        mov eax, 1
        int 0x80
        ```
    2. Assemble: `asm -o out.o test.asm`
    3. Link: `linker out.o -o test`
    4. Computer starts burning

```

## Expected Behavior

What did you expect to happen?

## Actual Behavior

What actually happened? Include full error messages if any.

## Source Files (if applicable)

Paste relevant code snippets or attach files to help us reproduce the bug.

## Logs and Output

Include any relevant logs, console output, or screenshots.

## Additional Context

Add any other context about the problem here.

---

Thanks for helping improve the project!