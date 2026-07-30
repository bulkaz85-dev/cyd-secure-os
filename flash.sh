#!/usr/bin/env bash
# One-command build + flash for the CYD, if you already have PlatformIO
# installed locally. If you don't, use the browser flasher instead:
# docs/install.html (see FLASHING.md).
set -e
if ! command -v pio &> /dev/null; then
    echo "PlatformIO not found. Installing it now (pip install platformio)..."
    pip install --upgrade platformio
fi
echo "Building and uploading to the CYD..."
pio run -e cyd -t upload
echo "Done. Opening serial monitor (Ctrl+C to exit)..."
pio device monitor
