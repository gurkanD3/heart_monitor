#!/usr/bin/env python3
import os
import shutil
import subprocess
import sys
from pathlib import Path


PROJECT_ROOT = Path(os.environ.get("PROJECT_ROOT", "/work/Heart_monitor"))
NCS_ROOT = Path(os.environ.get("NCS_ROOT", "/opt/ncs"))
MODULE_ROOT = Path(
    os.environ.get("MODULE_ROOT", str(PROJECT_ROOT / "zephyr_circular_buffer"))
)
APP_DIR = Path(os.environ.get("APP_DIR", str(PROJECT_ROOT)))
BUILD_DIR = Path(os.environ.get("BUILD_DIR", "/work/build"))
TEST_BUILD_ROOT = Path(os.environ.get("TEST_BUILD_ROOT", "/tmp/zephyr_tests"))
BOARD = os.environ.get("BOARD", "custom_discovery/nrf52833")
ARTIFACTS_DIR = Path(
    os.environ.get("ARTIFACTS_DIR", str(PROJECT_ROOT / "out"))
)
TEST_BOARD = os.environ.get("TEST_BOARD", "native_sim/native/64")
TEST_APPS = [
    MODULE_ROOT / "circular_buffer" / "tests" / "static_buffer_unit_test",
    MODULE_ROOT / "circular_buffer" / "tests" / "linked_list_buffer_unit_test",
]


def run(cmd: list[str], cwd: Path | None = None) -> None:
    print("+", " ".join(cmd), flush=True)
    subprocess.run(cmd, check=True, cwd=cwd)


def ensure_environment() -> None:
    if not PROJECT_ROOT.is_dir():
        raise SystemExit(f"Project root not found: {PROJECT_ROOT}")
    if not NCS_ROOT.is_dir():
        raise SystemExit(f"NCS root not found: {NCS_ROOT}")
    if not (NCS_ROOT / ".west").is_dir():
        raise SystemExit(f"NCS workspace marker missing: {NCS_ROOT / '.west'}")
    if shutil.which("west") is None:
        raise SystemExit("'west' is not available in PATH")
    if not APP_DIR.is_dir():
        raise SystemExit(f"Application directory not found: {APP_DIR}")
    if not MODULE_ROOT.is_dir():
        raise SystemExit(f"Module root not found: {MODULE_ROOT}")
    for test_app in TEST_APPS:
        if not test_app.is_dir():
            raise SystemExit(f"Test app not found: {test_app}")


def build(pristine: bool = True) -> None:
    BUILD_DIR.mkdir(parents=True, exist_ok=True)

    cmd = [
        "west",
        "build",
        "-b",
        BOARD,
        str(APP_DIR),
        "-d",
        str(BUILD_DIR),
        "--",
        f"-DBOARD_ROOT={PROJECT_ROOT}",
    ]
    if pristine:
        cmd.insert(2, "always")
        cmd.insert(2, "-p")

    run(cmd, cwd=PROJECT_ROOT)


def run_unit_tests() -> None:
    TEST_BUILD_ROOT.mkdir(parents=True, exist_ok=True)

    for test_app in TEST_APPS:
        test_name = test_app.name
        build_dir = TEST_BUILD_ROOT / test_name / "native_sim" / "native" / "64"
        test_bin = build_dir / test_name / "zephyr" / "zephyr.exe"

        if build_dir.exists():
            shutil.rmtree(build_dir)
        build_dir.mkdir(parents=True, exist_ok=True)

        env = os.environ.copy()
        env["CCACHE_DISABLE"] = "1"

        cmd = [
            "west",
            "build",
            "-p",
            "always",
            "-b",
            TEST_BOARD,
            str(test_app),
            "-d",
            str(build_dir),
            "--",
            f"-DZEPHYR_EXTRA_MODULES={MODULE_ROOT}",
        ]

        print(f"building unit test: {test_name}", flush=True)
        print("+", " ".join(cmd), flush=True)
        subprocess.run(cmd, check=True, env=env, cwd=NCS_ROOT)

        if not test_bin.is_file():
            raise SystemExit(f"Test executable not found: {test_bin}")

        print(f"running unit test: {test_name}", flush=True)
        completed = subprocess.run(
            [str(test_bin)],
            check=True,
            text=True,
            capture_output=True,
        )
        if completed.stdout:
            print(completed.stdout, end="", flush=True)
        if completed.stderr:
            print(completed.stderr, end="", file=sys.stderr, flush=True)
        if "PROJECT EXECUTION SUCCESSFUL" not in completed.stdout:
            raise SystemExit(f"Unit test did not report success: {test_name}")


def artifact_candidates() -> list[Path]:
    return [
        BUILD_DIR / "merged.hex",
        BUILD_DIR / "Heart_monitor" / "zephyr" / "zephyr.hex",
        BUILD_DIR / "zephyr" / "zephyr.hex",
        BUILD_DIR / "Heart_monitor" / "zephyr" / "zephyr.bin",
        BUILD_DIR / "zephyr" / "zephyr.bin",
        BUILD_DIR / "Heart_monitor" / "zephyr" / "zephyr.elf",
        BUILD_DIR / "zephyr" / "zephyr.elf",
    ]


def export_artifacts() -> None:
    ARTIFACTS_DIR.mkdir(parents=True, exist_ok=True)
    copied = 0

    for path in artifact_candidates():
        if path.is_file():
            destination = ARTIFACTS_DIR / path.name
            shutil.copy2(path, destination)
            print(f"exported {path} -> {destination}", flush=True)
            copied += 1

    if copied == 0:
        raise SystemExit(
            f"No firmware artifacts found under build directory: {BUILD_DIR}"
        )


def print_usage() -> None:
    print(
        "Usage: docker-entrypoint.py "
        "[test|build|export|build-and-export|test-build-export|shell]",
        flush=True,
    )


def main() -> None:
    ensure_environment()

    command = sys.argv[1] if len(sys.argv) > 1 else "test-build-export"

    if command == "test":
        run_unit_tests()
    elif command == "build":
        build(pristine=True)
    elif command == "export":
        export_artifacts()
    elif command == "build-and-export":
        build(pristine=True)
        export_artifacts()
    elif command == "test-build-export":
        run_unit_tests()
        build(pristine=True)
        export_artifacts()
    elif command == "shell":
        os.execvp("bash", ["bash"])
    else:
        print_usage()
        raise SystemExit(f"Unknown command: {command}")


if __name__ == "__main__":
    main()
