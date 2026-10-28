#!/usr/bin/env python3
"""Build the PICO-BK release matrix (no third-party Python dependencies)."""
import argparse
import csv
import hashlib
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TARGETS = {
    "m1p1": ("pico", "rp2040", None, "m1", "p1"),
    "m1p2": ("pico2", "rp2350-arm-s", None, "m1", "p2"),
    "m2p2": ("pico2", "rp2350-arm-s", "MURM2", "m2", "p2"),
    "PCp1": ("pico", "rp2040", "PICO_PC", "PC", "p1"),
    "PCp2": ("pico2", "rp2350-arm-s", "PICO_PC", "PC", "p2"),
    "z0p1": ("pico", "rp2040", "ZERO", "z0", "p1"),
    "z0p2": ("waveshare_rp2350_pizero", "rp2350-arm-s", "ZERO2", "z0", "p2"),
}
AUDIO = ("PWM", "I2S", "HWAY")
VIDEO = ("VGA-HDMI", "VGA", "TV-SOFT")


def selected(value, names):
    if value.lower() == "all":
        return list(names)
    parts = [part.strip() for part in value.split(",") if part.strip()]
    if not parts or any(part not in names for part in parts):
        raise ValueError("Invalid selection %r; use all or comma-separated values from %s" %
                         (value, ", ".join(names)))
    return list(dict.fromkeys(parts))


def find_tool(tool, env_var, fallback_pattern):
    override = os.environ.get(env_var)
    if override:
        path = Path(override)
        if path.exists():
            return str(path)
        found = shutil.which(override)
        if found:
            return found
        raise RuntimeError("%s=%s is not executable" % (env_var, override))
    found = shutil.which(tool)
    if found:
        return found
    # Raspberry Pi Pico VS Code extension's installed binaries on Windows.
    for path in sorted((Path.home() / ".pico-sdk").glob(fallback_pattern), reverse=True):
        if path.is_file():
            return str(path)
    raise RuntimeError("Cannot find %s; install it or set %s" % (tool, env_var))


def expected_filename(target, audio, video, version):
    _, platform, _, prefix, pver = TARGETS[target]
    if video == "TV-SOFT":
        display = "TV-319.2MHz"
    elif video == "VGA":
        display = "VGA-390MHz"
    else:
        display = "VGA-HDMI-%sMHz" % ("390" if platform == "rp2040" else "400")
    return "%s%s-bk-%s-%s-%s.uf2" % (prefix, pver, display, audio, version)


def get_version():
    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    match = re.search(r'^set\(BUILD_VERSION\s+"([0-9][^"\s]*)"\)', cmake, re.M)
    if not match:
        raise RuntimeError("BUILD_VERSION missing from CMakeLists.txt")
    return match.group(1)


def run(command, dry_run):
    print("+", subprocess.list2cmdline(list(map(str, command))), flush=True)
    if not dry_run:
        subprocess.run(list(map(str, command)), cwd=str(ROOT), check=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--targets", default="all", help="all or comma-separated: " + ", ".join(TARGETS))
    parser.add_argument("--audio", default="all", help="all (default) or comma-separated: " + ", ".join(AUDIO))
    parser.add_argument("--video", default="all", help="all (default) or comma-separated: " + ", ".join(VIDEO))
    parser.add_argument("--clean", action="store_true", help="remove each selected build directory before configuring")
    parser.add_argument("--jobs", type=int, default=4, help="parallel compile jobs per build (default 4)")
    parser.add_argument("--dry-run", action="store_true", help="print the build matrix and commands; do not build")
    args = parser.parse_args()
    if args.jobs < 1:
        parser.error("--jobs must be at least 1")
    try:
        targets = selected(args.targets, TARGETS)
        audios = selected(args.audio, AUDIO)
        videos = selected(args.video, VIDEO)
        version = get_version()
        cmake = find_tool("cmake", "CMAKE", "cmake/*/bin/cmake.exe")
        ninja = find_tool("ninja", "NINJA", "ninja/*/ninja.exe")
    except (ValueError, RuntimeError) as ex:
        parser.error(str(ex))

    firmware = ROOT / "firmware" / version
    manifest = []
    failures = []
    total = len(targets) * len(audios) * len(videos)
    print("PICO-BK %s: %d variant(s)" % (version, total), flush=True)
    if not args.dry_run:
        # A release directory represents exactly this invocation's matrix.
        # Do not accidentally ship UF2 files from an earlier selection.
        firmware.mkdir(parents=True, exist_ok=True)
        for old in firmware.glob("*.uf2"):
            old.unlink()
        for old in (firmware / "manifest.csv", firmware / "SHA256SUMS", firmware / "failures.csv"):
            old.unlink(missing_ok=True)
    for target in targets:
        board, platform, flag, _, _ = TARGETS[target]
        for audio in audios:
            for video in videos:
                label = "%s-%s-%s" % (target, video, audio)
                build_dir = ROOT / "build-release" / label
                uf2 = ROOT / "bin" / "Release" / expected_filename(target, audio, video, version)
                output = firmware / uf2.name
                if args.clean and not args.dry_run and build_dir.exists():
                    shutil.rmtree(build_dir)
                conf = [cmake, "-S", str(ROOT), "-B", str(build_dir), "-G", "Ninja",
                        "-DCMAKE_MAKE_PROGRAM=" + ninja,
                        "-DCMAKE_BUILD_TYPE=Release", "-DPICO_BOARD=" + board,
                        "-DPICO_PLATFORM=" + platform,
                        "-DMURM2=" + ("ON" if flag == "MURM2" else "OFF"),
                        "-DPICO_PC=" + ("ON" if flag == "PICO_PC" else "OFF"),
                        "-DZERO=" + ("ON" if flag == "ZERO" else "OFF"),
                        "-DZERO2=" + ("ON" if flag == "ZERO2" else "OFF"),
                        "-DAY_TYPE=" + audio,
                        "-DTV_OUT=" + ("ON" if video == "TV-SOFT" else "OFF"),
                        "-DVGA_ONLY=" + ("ON" if video == "VGA" else "OFF"),
                        "-Dm1p2launcher=OFF"]
                print("\n=== %s -> %s ===" % (label, uf2.name), flush=True)
                if not args.dry_run:
                    uf2.parent.mkdir(parents=True, exist_ok=True)
                    # Keep the artifact: incremental Ninja builds do not necessarily
                    # repeat POST_BUILD uf2 conversion when only that file is removed.
                try:
                    run(conf, args.dry_run)
                    run([cmake, "--build", str(build_dir), "--parallel", str(args.jobs)], args.dry_run)
                    if args.dry_run:
                        continue
                    if not uf2.is_file() or uf2.stat().st_size == 0:
                        raise RuntimeError("Expected UF2 not produced: " + str(uf2))
                    firmware.mkdir(parents=True, exist_ok=True)
                    shutil.copy2(uf2, output)
                    sha = hashlib.sha256(output.read_bytes()).hexdigest()
                    manifest.append((output.name, target, board, platform, video, audio,
                                     output.stat().st_size, sha))
                except (RuntimeError, subprocess.CalledProcessError) as ex:
                    print("BUILD FAILED: %s: %s" % (label, ex), file=sys.stderr)
                    failures.append((target, video, audio, str(ex)))
                    continue
    if not args.dry_run:
        with (firmware / "manifest.csv").open("w", encoding="utf-8", newline="") as f:
            writer = csv.writer(f)
            writer.writerow(("file", "target", "board", "platform", "video", "audio", "bytes", "sha256"))
            writer.writerows(manifest)
        (firmware / "SHA256SUMS").write_text(
            "".join("%s  %s\n" % (row[-1], row[0]) for row in manifest), encoding="ascii")
        if failures:
            with (firmware / "failures.csv").open("w", encoding="utf-8", newline="") as f:
                writer = csv.writer(f)
                writer.writerow(("target", "video", "audio", "error"))
                writer.writerows(failures)
        print("\nBuilt %d/%d UF2 files into %s (%d failed)" %
              (len(manifest), total, firmware, len(failures)), flush=True)
        if failures:
            print("See %s for failed variants" % (firmware / "failures.csv"), file=sys.stderr)
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
