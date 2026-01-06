import ci.build as build
import ci.artifacts as artifacts
import ci.archive as archive

from ci.os import OS, ARCH, getOS, getArch
from pathlib import Path
import argparse
import sys
import subprocess
import logging
import shutil
import platform

log_path = Path("logs/ci.log")
logger = logging.getLogger("ci")
logger.setLevel(logging.DEBUG)

# Argument parser
parser = argparse.ArgumentParser(description="CI/CD Script")
parser.add_argument(
    "-d", "--debug",
    dest="debug",
    action="store_true",
    help="Enable debug build"
)
parser.add_argument(
    "-c", "--clean",
    dest="clean",
    action="store_true",
    help="Clean builds"
)
parser.add_argument(
    "-a", "--archive",
    dest="archive",
    action="store_true",
    help="Archive the projekt"
)

parser.add_argument(
    "--no-build",
    dest="build",
    action="store_false",
    help="Stop before building the project"
)
parser.add_argument(
    "--no-test",
    dest="test",
    action="store_false",
    help="Stop before testing the project"
)
parser.add_argument(
    "--no-log",
    dest="log",
    action="store_false",
    help="Stops logging while building"
)

parser.add_argument(
    "-A", "--archive-name",
    dest="archive_name",
    metavar="NAME",
    type=str,
    help="Name of the archive to create"
)

parser.add_argument(
    "--arch",
    dest="arch_name",
    metavar="NAME",
    type=str,
    help="x86_64 or arm64"
)

parser.add_argument(
    "--os",
    dest="os_name",
    metavar="NAME",
    type=str,
    help="windows, macos or linux"
)

def main(args, os: OS, arch: ARCH) -> bool:
    if (os == OS.Windows and arch == ARCH.ARM64):
        print("Windows ARM isn't supported")
        exit(1)

    if (args.clean):
        build.clean(debug=args.debug, os=os, arch=arch)
        shutil.rmtree("dist", ignore_errors=True)
        shutil.rmtree("archives", ignore_errors=True)
        subprocess.run([sys.executable, "tests/run.py", "-c"])
        log_path.unlink(missing_ok=True)

    if (not args.build):
        logger.debug("Stopping before building")
        return False

    result: bool = build.build(debug=args.debug, os=os, arch=arch)
    if (not result):
        logger.error("Building failed")
        return False

    result = artifacts.stage(debug=args.debug)
    if (not result):
        logger.error("Staging artifacts failed")
        return False

    if (not args.test):
        logger.debug("Stopping before testing")
        return True

    #logger.info("Starting tests")
    #ret = subprocess.run([sys.executable, "tests/run.py"])
    #if ret.returncode != 0:
    #    logger.error("Tests failed")
    #    return False
    #logger.info("Finished tests")

    return True

if __name__ == "__main__":
    help = False
    if any(arg.startswith("-h") or arg in ("--h", "--he", "--hel", "--help") for arg in sys.argv[1:]):
        help = True
    args = parser.parse_args()

    if args.log:
        Path("logs").mkdir(exist_ok=True)

        file_handler = logging.FileHandler(str(log_path), mode="w", encoding="utf-8")
        file_handler.setLevel(logging.DEBUG)
        file_formatter = logging.Formatter("%(asctime)s [%(levelname)s] %(message)s")
        file_handler.setFormatter(file_formatter)
        logger.addHandler(file_handler)

        console_handler = logging.StreamHandler()
        console_handler.setLevel(logging.INFO)
        console_formatter = logging.Formatter("[%(levelname)s] %(message)s")
        console_handler.setFormatter(console_formatter)
        logger.addHandler(console_handler)

    logger.debug(f"Debug: {args.debug}, Clean: {args.clean}, Build: {args.build}, Test: {args.test}, Archive: {args.archive}")
    archive_name = args.archive_name or "lct"
    logger.debug(f"Archive name: {archive_name}")

    os: OS
    arch: ARCH
    os_uname = platform.system().lower()
    if (os_uname == "windows"): os = OS.Windows
    elif (os_uname == "darwin"): os = OS.macOS
    elif (os_uname == "linux"): os = OS.Linux
    else: raise ValueError("Unknown os")
    cpu_arch = platform.machine().lower()
    if (cpu_arch in ["x86_64", "amd64"]): arch = ARCH.x86_64
    elif (cpu_arch in ["arm64", "aarch64"]): arch = ARCH.ARM64
    else: raise ValueError("Unknown architecture")
    if (args.os_name):
        if (args.os_name == "windows"): os = OS.Windows
        elif (args.os_name == "macos"): os = OS.macOS
        elif (args.os_name == "linux"): os = OS.Linux
    if (args.arch_name):
        if (args.arch_name == "x86_64"): arch = ARCH.x86_64
        elif (args.arch_name == "arm64"): arch = ARCH.ARM64
    logger.debug(f"OS: {getOS(os)}, ARCH: {getArch(arch)}")

    result: bool = main(args, os, arch)
    if not result:
        sys.exit(1)

    if args.archive:
        Path("archives").mkdir(parents=True, exist_ok=True)
        archive.archive("dist", f"archives/{archive_name}")
