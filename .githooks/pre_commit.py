import os
import platform
from pathlib import PurePosixPath
import shutil
import subprocess
import sys


CPP_EXTENSIONS = {
    ".c",
    ".cc",
    ".cpp",
    ".cxx",
    ".cppm",
    ".h",
    ".hpp",
    ".hxx",
    ".cu",
    ".cuh",
}


EXCLUDED_DIRS = (
    PurePosixPath("zmkernel/vendor"),
    PurePosixPath("zmviewer/vendor"),
)


def is_excluded(path):
    posix_path = PurePosixPath(path.replace("\\", "/"))
    return any(excluded_dir in posix_path.parents for excluded_dir in EXCLUDED_DIRS)


def find_visual_studio_installation():
    vs_install_dir = os.environ.get("VSINSTALLDIR")
    if vs_install_dir and os.path.isdir(vs_install_dir):
        return vs_install_dir

    program_files_x86 = os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)")
    vswhere = os.path.join(program_files_x86, "Microsoft Visual Studio", "Installer", "vswhere.exe")
    if not os.path.isfile(vswhere):
        return None

    result = subprocess.run(
        [
            vswhere,
            "-latest",
            "-products",
            "*",
            "-requires",
            "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
            "-property",
            "installationPath",
        ],
        capture_output=True,
        text=True,
        encoding="mbcs",
        errors="replace",
        check=False,
    )
    installation_path = result.stdout.strip()
    return installation_path if installation_path and os.path.isdir(installation_path) else None


def find_windows_clang_format():
    installation_path = find_visual_studio_installation()
    if not installation_path:
        return None

    candidate_directories = (
        os.path.join(installation_path, "VC", "Tools", "Llvm", "x64", "bin"),
        os.path.join(installation_path, "VC", "Tools", "Llvm", "bin"),
        os.path.join(installation_path, "Common7", "IDE", "CommonExtensions", "Microsoft", "LLVM", "bin"),
    )
    for directory in candidate_directories:
        candidate = os.path.join(directory, "clang-format.exe")
        if os.path.isfile(candidate):
            return candidate

    return None


def main():
    configured_clang_format = os.environ.get("CLANG_FORMAT")
    clang_format = shutil.which(configured_clang_format or "clang-format")
    if clang_format is None and configured_clang_format is None and platform.system() == "Windows":
        clang_format = find_windows_clang_format()

    if clang_format is None:
        system = platform.system()

        print("error: clang-format not found.")

        if system == "Windows":
            print(
                "Install Visual Studio's 'C++ Clang tools for Windows' component. "
                "The hook searches common Visual Studio installations automatically; "
                "set CLANG_FORMAT to override the detected path."
            )
        elif system == "Linux":
            print(
                "On Ubuntu/Debian, install clang-format with:\n"
                "  sudo apt update\n"
                "  sudo apt install clang-format"
            )
        elif system == "Darwin":
            print(
                "On macOS, install clang-format with Homebrew:\n"
                "  brew install clang-format"
            )
        else:
            print(
                "Please install clang-format and ensure it is available in PATH, "
                "or set the CLANG_FORMAT environment variable."
            )

        return 1

    try:
        result = subprocess.run(
            ["git", "diff", "--cached", "--name-only", "--diff-filter=ACMR", "-z"],
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
    except subprocess.CalledProcessError as exc:
        print("error: failed to query staged files from git diff --cached.", file=sys.stderr)
        if exc.stderr:
            print(os.fsdecode(exc.stderr).strip(), file=sys.stderr)
        return exc.returncode or 1

    filepaths = []
    for path in result.stdout.split(b"\0"):
        if path:
            filepaths.append(os.fsdecode(path))

    for path in filepaths:
        if is_excluded(path):
            continue

        _, ext = os.path.splitext(path)
        if ext.lower() not in CPP_EXTENSIONS:
            continue

        if not os.path.isfile(path):
            continue

        print(f"Formatting: {path}")

        try:
            subprocess.run(
                [clang_format, "-i", "-style=file", path],
                check=True,
                stderr=subprocess.PIPE,
            )
        except subprocess.CalledProcessError as exc:
            print(f"error: clang-format failed for '{path}'.", file=sys.stderr)
            if exc.stderr:
                print(os.fsdecode(exc.stderr).strip(), file=sys.stderr)
            return exc.returncode or 1

        try:
            subprocess.run(
                ["git", "add", "--", path],
                check=True,
                stderr=subprocess.PIPE,
            )
        except subprocess.CalledProcessError as exc:
            print(f"error: failed to restage formatted file '{path}'.", file=sys.stderr)
            if exc.stderr:
                print(os.fsdecode(exc.stderr).strip(), file=sys.stderr)
            return exc.returncode or 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
