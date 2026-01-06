from enum import Enum

class OS(Enum):
    Windows = 1
    macOS = 2
    Linux = 3

class ARCH(Enum):
    x86_64 = 1
    ARM64 = 2

def getOS(os: OS) -> str:
    if (os == OS.Windows): return "windows"
    if (os == OS.macOS): return "macos"
    if (os == OS.Linux): return "linux"
    raise ValueError("Unknown operating system")

def getArch(arch: ARCH) -> str:
    if (arch == ARCH.x86_64): return "x86_64"
    if (arch == ARCH.ARM64): return "arm64"
    raise ValueError("Unknown architecture")
