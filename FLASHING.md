# Flashing the CYD — the easy way

You have two options. **Option A needs no software installed at all** —
GitHub compiles it for you and your browser does the flashing. Option B is
for people who already use PlatformIO and just want a one-command shortcut.

## Option A: Browser flashing (recommended, nothing to install)

1. **Create a free GitHub account** if you don't have one (github.com).
2. **Create a new repository** (top right → "New repository"), any name,
   set it to Public (GitHub Pages, used in step 5, needs Public on the free
   tier).
3. **Upload this whole project folder** into that repository. Easiest way:
   on the repo page, click "Add file" → "Upload files", drag in everything
   from this `cyd-secure-os` folder (keep the folder structure — including
   the hidden `.github` folder; if your drag-and-drop hides it, use
   `git push` from a terminal instead, or GitHub Desktop, which shows
   hidden folders).
4. **Wait about 3-5 minutes.** Go to the "Actions" tab in your repo — you'll
   see "Build CYD Firmware" running automatically. When it finishes with a
   green checkmark, the compiled firmware has been placed in
   `docs/firmware/` in your repo automatically. You did not need to install
   anything for this step — GitHub's servers did the compiling.
5. **Turn on GitHub Pages**: repo → Settings → Pages → under "Build and
   deployment", set Source to "Deploy from a branch", branch = `main`,
   folder = `/docs`, then Save.
6. **Wait ~1 minute**, then visit the URL GitHub shows you there — it'll
   look like `https://yourusername.github.io/your-repo-name/install.html`.
7. **Open that link in Chrome or Edge** (WebSerial flashing only works in
   these browsers, desktop only — not Safari, not Firefox, not mobile).
8. **Plug the CYD in via USB-C** (a real data cable, not a charge-only one).
9. **Click "Connect & Install"**, pick the CYD's port when your browser
   asks, and follow the on-screen prompts. It erases and writes the new
   firmware, then the CYD reboots straight into CYD Secure OS.

That's the whole process — after the initial GitHub setup, re-flashing
after any code change is just: push your changed files, wait for the green
checkmark, reload the install page, click the button again.

## Option B: Local one-command script (if you already use PlatformIO)

If you have PlatformIO installed (or are fine installing it once via
`pip install platformio`):

- **macOS/Linux**: open a terminal in this folder, run `./flash.sh`
- **Windows**: double-click `flash.bat` (or run it from a terminal)

Either builds the firmware and uploads it over USB in one step, then opens
a serial monitor so you can see boot logs.

## Troubleshooting

- **No port shows up when flashing**: install the USB-to-serial driver for
  your OS — search "CP2102 driver" or "CH340 driver" (check which chip is
  printed on your specific CYD board near the USB connector), then
  reconnect the cable.
- **Upload fails partway / times out**: some CYD boards need you to hold
  the "BOOT" button on the board while the upload starts, then release it
  once you see "Connecting..." turn into progress dots. This is a known
  quirk on some CYD units and cables.
- **GitHub Action fails (red X)**: click into it to view the error log —
  usually a typo introduced in a file you edited. Compare against the
  original files if unsure.
