#!/usr/bin/env python3
"""
merge_firmware.py — Merge ESP32 build artifacts into firmware images.

Outputs
  firmware_ota.bin   App binary only — upload via the device web UI Upgrade tab
  firmware_full.bin  Full flash image — initial programming with esptool

Usage
  python merge_firmware.py [build_dir]
  Default build_dir: ./build

Requires esptool
  pip install esptool
"""

import sys
import os
import json
import shutil
import subprocess

BUILD_DIR = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else "build")
OUT_OTA   = "firmware_ota.bin"
OUT_FULL  = "firmware_full.bin"

# ── Read build metadata ───────────────────────────────────────────────────────
fa_path = os.path.join(BUILD_DIR, "flasher_args.json")
if not os.path.exists(fa_path):
    print(f"ERROR: {fa_path} not found. Run 'idf.py build' first.")
    sys.exit(1)

with open(fa_path) as f:
    fa = json.load(f)

flash_files    = fa.get("flash_files", {})       # {"0x0": "bootloader/...", ...}
flash_settings = fa.get("flash_settings", {})
chip           = fa.get("extra_esptool_args", {}).get("chip", "esp32s3")

if not flash_files:
    print("ERROR: No flash_files in flasher_args.json")
    sys.exit(1)

# ── Identify the app binary ───────────────────────────────────────────────────
# It is the largest file that is not the bootloader, partition table, or otadata.
SKIP_NAMES = ("bootloader", "partition", "ota_data")

app_addr = None
app_path = None
for addr, rel in sorted(flash_files.items(), key=lambda x: -int(x[0], 16)):
    base = os.path.basename(rel)
    if any(s in base for s in SKIP_NAMES):
        continue
    abs_path = os.path.join(BUILD_DIR, rel)
    if os.path.exists(abs_path) and os.path.getsize(abs_path) > 100_000:
        app_addr = addr
        app_path = abs_path
        break

if not app_path:
    print("ERROR: Could not identify app binary in flash_files.")
    print("       Files found:", list(flash_files.values()))
    sys.exit(1)

print(f"App binary : {os.path.relpath(app_path)}  "
      f"({os.path.getsize(app_path):,} bytes)  @ {app_addr}")

# ── OTA image = app binary as-is ─────────────────────────────────────────────
shutil.copy(app_path, OUT_OTA)
print(f"OTA image  : {OUT_OTA}  ({os.path.getsize(OUT_OTA):,} bytes)")

# ── Full flash image via esptool merge_bin ────────────────────────────────────
cmd = [
    sys.executable, "-m", "esptool",
    "--chip", chip,
    "merge_bin",
    "--output", OUT_FULL,
    "--flash_mode", flash_settings.get("flash_mode", "dio"),
    "--flash_freq", flash_settings.get("flash_freq", "80m"),
    "--flash_size", flash_settings.get("flash_size", "16MB"),
]

for addr, rel in sorted(flash_files.items(), key=lambda x: int(x[0], 16)):
    abs_path = os.path.join(BUILD_DIR, rel)
    if os.path.exists(abs_path):
        cmd += [addr, abs_path]
    else:
        print(f"  WARNING: skipping missing file: {rel}")

print()
print("Running esptool merge_bin ...")
result = subprocess.run(cmd)
if result.returncode != 0:
    print("ERROR: merge_bin failed")
    sys.exit(1)

print(f"Full image : {OUT_FULL}  ({os.path.getsize(OUT_FULL):,} bytes)")
print()
print("Done.")
print(f"  OTA upgrade  →  upload  {OUT_OTA}  via the device web UI (Upgrade tab)")
print(f"  Initial flash →  esptool.py --chip {chip} write_flash 0x0 {OUT_FULL}")
