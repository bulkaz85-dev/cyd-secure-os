# CYD Secure OS (Phase 1)

A mini launcher-style firmware for the **ESP32-2432S028 "Cheap Yellow Display" (CYD)**:
lock screen with a derived AES-256 key, an encrypted notes vault on the SD card,
a Snake game, and placeholder screens for Gallery / Settings (Phase 2).

## Read this first: what this is and isn't

This is a genuinely encrypted personal-storage device, not a secure phone.
Being upfront about the gap matters more than the marketing:

| Claim from "ultra-secure phone" spec | Reality on ESP32-2432S028 |
|---|---|
| Cellular calls/SMS, E2EE messaging (Signal etc.) | **Not possible.** No cellular modem. This board only has Wi-Fi/BLE. Phase 2 could add a LAN-only encrypted chat between two CYDs, nothing like real Signal infrastructure. |
| Hardware kill switches for camera/mic/radio | **Not present.** No camera/mic on this board at all, and no physical switches wired to the radio. |
| Tamper-resistant, self-wiping secure element | **Not present.** No secure enclave chip. Keys live in software (derived at runtime, held in RAM while unlocked) and in NVS flash, which is readable if someone dumps the flash with the right tools. |
| "No holes in security" | No embedded system can honestly claim this, especially one built from a hobbyist dev board and community libraries. Treat this as **hardened casual privacy**, not nation-state-grade security. |
| Antivirus | Doesn't map onto a single-firmware microcontroller — there's no OS for malware to run *on* the way there is on Android/iOS. The real attack surface here is: someone reflashing the device, or someone extracting the SD card / flash for offline analysis. |

**What you get instead, and it's real:**
- AES-256-GCM (authenticated encryption — confidentiality *and* tamper detection) for every file the vault writes to the SD card
- A passphrase-derived key (PBKDF2-HMAC-SHA256, 100k iterations) — nothing is stored on disk that lets someone recover your passphrase without brute-forcing it
- A believable "someone glances at my SD card or borrows my CYD" threat model — not "a forensics lab has my device for a week"

If your actual requirement is closer to the original spec (journalist/executive-grade), you want GrapheneOS on a Pixel, or a Librem 5 / Bittium device — not a $10 devboard. Happy to help you configure one of those instead if that's the real goal.

## Hardware

- Board: ESP32-2432S028 (2.8" ILI9341 320x240 TFT + XPT2046 resistive touch)
- Add a microSD card (your 32GB card is plenty) in the board's SD slot
- No extra wiring needed — pins are pre-mapped in `platformio.ini` / `board_pins.h`

**Check your board revision.** There are a couple of known CYD PCB variants with
slightly different pin assignments (some use the SD card on VSPI and touch on
HSPI, or vice versa). If the touchscreen or SD card doesn't respond, search
"CYD [your exact model sticker] pinout" and adjust `board_pins.h` / `platformio.ini`.

## Build & flash

**See [FLASHING.md](FLASHING.md) for full instructions.** Short version:
you don't need to install anything — push this project to a free GitHub
repo, GitHub's servers compile it automatically, and a one-click page in
your browser (Chrome/Edge) flashes it over USB. A local PlatformIO
one-command script (`flash.sh` / `flash.bat`) is also included if you
prefer that route.

## Setting up your 32GB SD card (wipe it first)

The firmware expects specific folders on the card. Since you're wiping it,
here's the exact process:

1. **Format the card as FAT32** (not exFAT, not NTFS — the ESP32 SD library
   needs FAT32/FAT16). This erases everything on it.
   - Windows: right-click the drive in File Explorer → Format → File
     system: FAT32 → Start. If Windows refuses FAT32 on a 32GB card
     (it sometimes only offers exFAT above 32GB), use the free tool
     [guiformat](https://ridgecrop.co.uk/index.htm?guiformat.htm) or
     `diskpart` from an admin command prompt.
   - macOS: Disk Utility → select the card → Erase → Format:
     "MS-DOS (FAT)" → Erase.
   - Linux: `sudo mkfs.vfat -F 32 /dev/sdX` (replace `sdX` with your
     actual card device — check with `lsblk` first, this is destructive).

2. **Copy the `wiki` folder onto the card.** This project includes one
   at `sdcard-content/wiki/` with a working survival guide already
   written (water, fire, shelter, first aid, signaling, navigation &
   weather, hypothermia/heat illness, food/foraging caution). Copy that
   whole `wiki` folder to the **root** of the SD card, so you end up with:
   ```
   (SD card root)
   ├── wiki/
   │   ├── water.txt
   │   ├── fire.txt
   │   ├── shelter.txt
   │   ├── first_aid.txt
   │   ├── signaling.txt
   │   ├── navigation_weather.txt
   │   ├── hypothermia_heat.txt
   │   └── food_caution.txt
   ```
   Add your own `.txt` files here any time (first line = title, rest =
   body) — the Wiki app picks up new files automatically, no re-flash
   needed. Just plug the card into a computer, drop files in, put it back.

3. **Don't create a `vault` folder yourself** — the firmware creates
   `/vault` automatically on first boot once you've set your passphrase.
   That's where encrypted notes live; leave it alone otherwise.

4. **Insert the card into the CYD's SD slot** before powering it on.

That's the entire card setup — two folders, one you copy once, one the
firmware manages for you.

## Solar / battery power for field use

The CYD has no built-in battery charging or solar input — you add this
externally:

1. **Solar panel**: a small 5-6V panel (a few hundred mA) is enough to
   trickle-charge a phone-sized battery over a day.
2. **Solar charge controller**: a TP4056-based "solar LiPo charger" board
   (cheap, widely available) sits between the panel and the battery — it
   handles safe charging and stops overcharging. Panel → charger input,
   charger output → battery.
3. **Battery**: a single-cell LiPo/Li-ion, 1000-2000mAh is a reasonable
   size. Connect it to the CYD's battery header if your board revision has
   one, otherwise to the 5V/GND pins via the charge controller's output
   (check your specific board's power input — some CYD revisions accept
   5V directly on a JST or pin header, some need the onboard USB-C only;
   confirm before wiring anything semi-permanently).
4. **Battery voltage sensing (optional but recommended)**: wire two 100kΩ
   resistors in series from the battery's + terminal to GND, and tap the
   midpoint into GPIO35 (already wired up in `power.h`). This lets the
   firmware show a rough battery percentage in the status bar. Without
   this, the device still works fine — the battery% just won't display
   correctly (harmless, ignore it).

**Power-saving behavior already built in:** after 30 seconds idle the
backlight turns off; after 2 minutes idle the device enters deep sleep
(microamps of draw) and wakes instantly when you touch the screen. There's
also a "Sleep Now" tile on the home screen to force this immediately after
you're done using it — worth making a habit of, since deep sleep is what
makes a small solar setup viable at all. Realistic expectation: even with
this, a single small panel + a modest battery gives you "check it a few
times a day" runtime, not "always-on always-listening" — be honest with
yourself about your actual usage pattern when sizing the panel/battery.



The XPT2046 min/max values in `board_pins.h` (`TOUCH_MIN_X/MAX_X/MIN_Y/MAX_Y`) are
generic defaults. If taps land in the wrong spot, run a calibration sketch (search
"XPT2046 TFT_eSPI calibration sketch") and update those four constants.

## First run

- You'll be prompted to set a passphrase (numeric keypad, 4+ digits shown; edit
  `lock_screen.h` to add a full QWERTY keyboard widget for a real alphanumeric
  passphrase — strongly recommended, a 4-digit PIN is not meaningfully secure
  against someone with your SD card and time).
- After that, every boot asks for the same passphrase to derive the AES key
  and unlock the launcher.
- Repeated wrong attempts add an increasing on-device delay (2s → 30s cap).
  This only slows down someone typing on the touchscreen — it does **not**
  protect against someone reading the flash/SD card directly and brute-forcing
  offline. Passphrase strength is what actually protects you.

## What's implemented (Phase 1 + field-use additions)

- `crypto.h` — AES-256-GCM encrypt/decrypt + PBKDF2 key derivation (mbedtls, built into the ESP32 Arduino core, no extra library needed)
- `lock_manager.h` — passphrase provisioning/verification, session key held in RAM only while unlocked, wiped on lock
- `vault.h` — encrypted file read/write/list/delete on the SD card
- `notes_app.h` — minimal encrypted notes list/view/create (**text entry is a placeholder** — see extension points)
- `games/snake.h` — fully playable Snake with swipe controls
- `wiki_app.h` — offline reference library: search (via the on-screen keyboard), article list, paginated word-wrapped viewer, reading plain `.txt` files from `/wiki` on the SD card
- `keyboard.h` — on-screen QWERTY keyboard, used by the Wiki search bar (also usable anywhere else you need text input)
- `power.h` — battery voltage sensing, backlight dimming after idle, deep sleep after longer idle with wake-on-touch, "Sleep Now" launcher tile
- `launcher.h` — home-screen icon grid (Notes, Wiki, Gallery placeholder, Snake, Settings placeholder, Sleep Now), lock button, battery % in status bar
- `lock_screen.h` — numeric PIN entry / first-run provisioning
- `.github/workflows/build-firmware.yml` + `docs/install.html` — CI auto-build and one-click browser flashing (see FLASHING.md)

## Extension points for Phase 2 (not yet built)

1. **Encrypted gallery** — decrypt JPEGs from `/vault` on the fly for viewing.
   You'll want a JPEG decoder (TJpg_Decoder library pairs well with TFT_eSPI)
   fed from a buffer you've decrypted with `Vault::readFile`, not from a
   plaintext file directly.
2. **LAN encrypted messenger** — ESP-NOW or a local Wi-Fi socket between two
   CYDs, AES-GCM encrypted payloads using a pre-shared key or a simple
   Diffie-Hellman exchange (mbedtls has `mbedtls_ecdh` if you want to do this
   properly instead of a static shared key).
3. **Photo import** — since there's no camera, photos would need to be copied
   onto the SD card pre-encrypted from a PC (a small companion script using
   the same AES-GCM scheme) or encrypted on-device after transfer via USB-mass-storage mode.
4. **Panic wipe** — `LockManager::factoryReset()` exists but isn't wired to any
   UI trigger. Think carefully before binding it to something like "5 failed
   attempts" — a mistyped passphrase permanently destroying your notes is a
   real, easy-to-trigger downside.
5. **Filename confidentiality** — currently filenames on the SD card (e.g. `note_myfile.enc`) are visible even though contents are encrypted. An encrypted index mapping opaque names to display names would close this gap.
6. **Alphanumeric passphrase on the lock screen** — the lock screen still uses
   a numeric keypad; swap it for the `Keyboard` widget (already built, used by
   Notes and the Wiki search bar) to allow a real passphrase instead of just
   digits. Strongly recommended — see the security note in `crypto.h`.
7. **PWM backlight dimming** — `power.h` currently does a hard on/off toggle
   for "dimming" (documented in the code as a simplification). Wiring
   `ledcWrite` to the backlight pin would give smooth, less jarring dimming.

## Library dependencies (auto-installed by PlatformIO)

- `bodmer/TFT_eSPI` — display driver
- `paulstoffregen/XPT2046_Touchscreen` — touch driver
- `bblanchon/ArduinoJson` — reserved for Phase 2 settings/config storage
- `mbedtls` — bundled with the ESP32 Arduino core, no install needed
