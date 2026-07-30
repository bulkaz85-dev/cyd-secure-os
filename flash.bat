@echo off
where pio >nul 2>nul
if %errorlevel% neq 0 (
    echo PlatformIO not found. Installing it now...
    pip install --upgrade platformio
)
echo Building and uploading to the CYD...
pio run -e cyd -t upload
echo Done. Opening serial monitor (Ctrl+C to exit)...
pio device monitor
