from dataclasses import dataclass, field
from pathlib import Path
import subprocess
from typing import Optional, Dict
import hashlib
import json
import shutil
import logging

from ci.os import OS, ARCH, getOS, getArch

logger = logging.getLogger("ci")

class BuildCache:
    def __init__(self, cache_file: Path):
        self.cache_file = cache_file
        self.hashes: Dict[str, str] = {}
        self.load()

    def load(self):
        if self.cache_file.exists():
            try:
                self.hashes = json.loads(self.cache_file.read_text())
            except Exception:
                print(f"Warning: Failed to load build cache from {self.cache_file}")
                self.hashes = {}

    def save(self):
        self.cache_file.write_text(json.dumps(self.hashes, indent=4, ensure_ascii=False))

    def get(self, target: Path) -> str | None:
        return self.hashes.get(str(target))

    def update(self, target: Path, hash_value: str):
        self.hashes[str(target)] = hash_value

    def is_up_to_date(self, target: Path, hash_value: str) -> bool:
        if not target.exists():
            return False
        return self.get(target) == hash_value

@dataclass
class Toolchain:
    Compiler_C: Optional[str] = None
    Compiler_C_Dependency_Args: list[str] = None
    Compiler_C_Flags: list[str] = field(default_factory=list)

    Compiler_CPP: Optional[str] = None
    Compiler_CPP_Dependency_Args: list[str] = None
    Compiler_CPP_Flags: list[str] = field(default_factory=list)

    Linker: Optional[str] = None
    Linker_Flags: list[str] = field(default_factory=list)

    Strip: Optional[str] = None
    Strip_Flags: list[str] = field(default_factory=list)

def parse_gcc_dep_file(dep_path: Path) -> list[str]:
    if not dep_path.exists():
        return []
    
    content = dep_path.read_text()
    content = content.replace('\\\n', ' ')
    parts = content.split()
    if not parts: return []
    
    # first part is target
    return parts[1:]

def hash_files(files: list[Path]) -> str:
    hasher = hashlib.new("sha256")

    for file in sorted(files, key=lambda f: str(f)):
        with file.open("rb") as f:
            while chunk := f.read(8192):
                hasher.update(chunk)
                
    return hasher.hexdigest()

def build_src(debug: bool, toolchain: Toolchain, buildCache: BuildCache, build_dir: Path, source_dir: Path, out: Path):
    patterns = ["*.c", "*.cpp"]

    files: list[Path] = []
    for pattern in patterns:
        files.extend(source_dir.rglob(pattern))

    objects: list[Path] = []
    for file in files:
        rel_path = file.relative_to(source_dir)

        target_path = build_dir / rel_path.with_suffix(".o")
        target_path.parent.mkdir(parents=True, exist_ok=True)
        dep_path = target_path.with_suffix(".d")

        objects.append(target_path)

        compiler: str
        flags: list[str]
        dep_args: list[str]
        if file.suffix == ".c":
            compiler = toolchain.Compiler_C
            flags = toolchain.Compiler_C_Flags
            dep_args = toolchain.Compiler_C_Dependency_Args

        elif file.suffix == ".cpp":
            compiler = toolchain.Compiler_CPP
            flags = toolchain.Compiler_CPP_Flags
            dep_args = toolchain.Compiler_CPP_Dependency_Args

        else:
            raise ValueError(f"Unknown source extension: {file.suffix}")

        try:
            deps = parse_gcc_dep_file(dep_path)
            all_deps = [file, *map(Path, deps)]
            content_hash = hash_files(all_deps)

            if not buildCache.is_up_to_date(target_path, content_hash):
                print(f"Compiling {file} -> {target_path}")
                subprocess.run([compiler, *flags, *dep_args, str(dep_path), "-c", str(file), "-o", str(target_path)], check=True)

                new_deps = parse_gcc_dep_file(dep_path)
                new_all_deps = [file, *map(Path, new_deps)]
                new_content_hash = hash_files(new_all_deps)
                buildCache.update(target_path, new_content_hash)

        except subprocess.CalledProcessError as e:
            print(f"Error: Compilation failed for {file}")
            raise e

    linker = toolchain.Linker
    flags = toolchain.Linker_Flags

    out.parent.mkdir(parents=True, exist_ok=True)

    try:
        content_hash = hash_files(sorted(objects, key=str))

        if not buildCache.is_up_to_date(out, content_hash):
            print(f"Linking {out}")
            subprocess.run([linker, *flags, *map(str, objects), "-o", str(out)], check=True)

            buildCache.update(out, content_hash)
    except subprocess.CalledProcessError as e:
        print(f"Error: Linking failed for {out}")
        raise e
    
    if not debug:
        strip = toolchain.Strip
        flags = toolchain.Strip_Flags
        
        try:
            subprocess.run([strip, *flags, str(out)], check=True)
        except subprocess.CalledProcessError as e:
            print(f"Error: Stripping failed for {out}")
            raise e

def build(debug: bool, os: OS, arch: ARCH) -> bool:
    logger.info("Building the project")

    cache: BuildCache = BuildCache(Path(".buildcache.json"))

    toolchain: Toolchain = Toolchain()

    # Tools
    toolchain.Strip = "strip"

    toolchain.Compiler_C_Dependency_Args = ["-MMD", "-MF"]
    toolchain.Compiler_CPP_Dependency_Args = ["-MMD", "-MF"]

    if os == OS.macOS:
        toolchain.Compiler_C = "clang"
        toolchain.Compiler_CPP = "clang++"
        toolchain.Linker = "clang++"
    
    else:
        toolchain.Compiler_C = "gcc"
        toolchain.Compiler_CPP = "g++"
        toolchain.Linker = "g++"


    Warning_Flags = ["-Wall", "-Wextra"]
    Optimize_Flags = ["-O2"]
    Debug_Flags = ["-g", "-DDEBUG_BUILD"]
    Release_Flags = ["-DNDEBUG"]
    Security_Flags = ["-fstack-protector-strong", "-D_FORTIFY_SOURCE=2", "-fPIC"]
    Static_Flags = ["-static", "-static-libgcc", "-static-libstdc++"]

    # FLAGS
    toolchain.Compiler_C_Flags.extend(Warning_Flags)

    toolchain.Compiler_CPP_Flags.append("-std=c++17")
    toolchain.Compiler_CPP_Flags.extend(Warning_Flags)

    if os == OS.Linux:
        toolchain.Linker_Flags.append("-Wl,--gc-sections")

        toolchain.Compiler_C_Flags.append("-ffunction-sections")
        toolchain.Compiler_C_Flags.append("-fdata-sections")

        toolchain.Compiler_CPP_Flags.append("-ffunction-sections")
        toolchain.Compiler_CPP_Flags.append("-fdata-sections")

        toolchain.Linker_Flags.append("-lpthread")
        toolchain.Linker_Flags.append("-ldl")
        toolchain.Linker_Flags.append("-lm")

        toolchain.Strip_Flags.append("--strip-unneeded")
    
    elif os == OS.macOS:
        toolchain.Linker_Flags.append("-lpthread")
        toolchain.Linker_Flags.append("-lm")
        toolchain.Linker_Flags.append("-lc++")

        toolchain.Strip_Flags.append("-x")
        toolchain.Strip_Flags.append("-S")

    elif os == OS.Windows:
        toolchain.Linker_Flags.append("-lws2_32")
        toolchain.Linker_Flags.append("-luser32")
        toolchain.Linker_Flags.append("-lkernel32")
        toolchain.Linker_Flags.append("-lwsock32")
        toolchain.Linker_Flags.append("-lntdll")
        toolchain.Linker_Flags.append("-luserenv")


    if debug:
        toolchain.Compiler_C_Flags.extend(Debug_Flags)

        toolchain.Compiler_CPP_Flags.extend(Debug_Flags)

    else:
        toolchain.Compiler_C_Flags.extend(Optimize_Flags)
        toolchain.Compiler_C_Flags.extend(Release_Flags)

        toolchain.Compiler_CPP_Flags.extend(Optimize_Flags)
        toolchain.Compiler_CPP_Flags.extend(Release_Flags)
        toolchain.Compiler_CPP_Flags.extend(Security_Flags)

        if os != OS.macOS:
            toolchain.Linker_Flags.extend(Static_Flags)


    build_dir = Path("build") / ("debug" if debug else "release")
    source_dir = Path("src")

    binaries_txt = build_dir / "binaries.txt"
    binaries_txt.parent.mkdir(parents=True, exist_ok=True)

    binaries_txt.write_text("")

    executable = build_dir / "lct"
    if os == OS.Windows:
        executable = executable.with_suffix(".exe")

    try:
        build_src(debug, toolchain, cache, build_dir, source_dir, executable)

        with binaries_txt.open("a", encoding="utf-8") as f:
            f.write(str(executable) + "\n")

    except Exception as e:
        cache.save()
        print(f"Build failed: {e}")
        return False

    cache.save()

    return True

def clean(debug: bool, os: OS, arch: ARCH) -> bool:
    logger.info("Cleaning")

    build_dir = Path("build") / ("debug" if debug else "release")
    
    if build_dir.exists(): shutil.rmtree(str(build_dir), ignore_errors=True)

    return True
