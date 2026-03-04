# ASCII Capsule — Nintendo DSi Application Plan

## DSi Hardware Constraints

| Spec | Value |
|------|-------|
| Screens | 2 × TFT-LCD, 256×192 pixels each |
| Console text | 32 columns × 24 rows per screen (8×8 font) |
| CPU | ARM9 @ 133 MHz + ARM7 @ 33 MHz |
| RAM | 16 MB |
| Storage | SD card (up to 32 GB) + 256 MB internal flash |
| Input | D-pad, A/B/X/Y, L/R, Start/Select, touchscreen |
| Audio | AAC playback, speakers + headphone jack |

### Screen Size vs. ASCII Art Width

The existing ASCII art is **60 characters wide**. The DSi console displays **32 characters per line**. The application implements:
- **Horizontal scrolling** (D-pad Left/Right) to view the full width
- **Vertical scrolling** (D-pad Up/Down) for tall content
- **Page scrolling** (L/R shoulder buttons) for quick navigation

Art can be regenerated at 32-char width using the included `asciify.py` tool, but this significantly reduces detail.

---

## Application Architecture

### State Machine

```
SPLASH (3s) → WELCOME (letter) → MENU (item list)
                                    ↓
                              CHALLENGE (ASCII art)
                                    ↓
                              CODE_ENTRY (keyboard)
                                ↓         ↓
                          WRONG_CODE   MESSAGE (unlocked)
                                ↓         ↓
                              (retry)    MENU

                    MENU → FREE_ENTRY (keyboard, bonus codes)
                              ↓         ↓
                          WRONG_CODE   MESSAGE (unlocked)
                              ↓         ↓
                            (retry)    MENU

                    MENU → INSTRUCTIONS (SELECT button)
                    MENU → MESSAGE (X button, if unlocked)
```

### Screen Layout

| State | Top Screen | Bottom Screen |
|-------|-----------|--------------|
| SPLASH | Welcome ASCII art | App title + loading |
| WELCOME | Scrollable letter | Controls help |
| MENU | Item list with status | Navigation help + progress |
| CHALLENGE | Scrollable ASCII art | Challenge instructions |
| CODE_ENTRY | ASCII art (reference) | On-screen keyboard |
| WRONG_CODE | ASCII art (reference) | Error + retry prompt |
| FREE_ENTRY | Bonus info + progress | On-screen keyboard |
| MESSAGE | Scrollable message text | Unlock confirmation |
| INSTRUCTIONS | Scrollable how-to guide | Navigation help |

### Module Design

```
dsi/
├── Makefile                 # devkitARM build configuration
├── source/
│   ├── main.c               # Entry point, state machine, NDS drawing
│   ├── content.h / .c       # Content loading + hash-based code checking (portable)
│   ├── viewer.h / .c        # Text viewer + scrolling (portable, testable)
│   └── keyboard.h / .c      # On-screen keyboard (NDS-specific)
├── nitrofiles/              # Embedded in .nds ROM via NitroFS
│   ├── manifest.txt         # Content manifest (name|hash|art|message + bonus codes)
│   ├── welcome_art.txt      # Startup ASCII art
│   ├── welcome_letter.txt   # Welcome letter
│   ├── instructions.txt     # How to extend the app
│   ├── art/                 # ASCII art files with concealed codes
│   │   ├── Bread.txt
│   │   ├── Chef.txt
│   │   └── ... (6 files total)
│   └── messages/            # Message text files
│       ├── Bread.txt
│       ├── Chef.txt
│       ├── One.txt        # Bonus message (free entry)
│       └── ... (13 files total: 6 regular + X bonus)
├── tests/
│   ├── Makefile             # Host-side test compilation
│   └── test_content.c       # Tests for content + viewer logic (34 tests)
└── tools/
    └── prepare_content.sh   # Script to copy/regenerate content
```

### Key Design Decisions

1. **C over C++**: Simpler, smaller binary, fewer dependencies on NDS
2. **NitroFS for content**: All data embedded in .nds — single file deployment
3. **FAT for save data**: Progress saved to SD card as `ascii_capsule_save.txt`
4. **Console text mode**: Uses libnds PrintConsole for text rendering — reliable, well-tested
5. **Software scrolling**: Redraws visible 32×24 window from content buffer on scroll
6. **D-pad + touch keyboard**: Full alphanumeric input, accessible both ways
7. **Hashed codes**: Codes stored as djb2-xor hashes in manifest — not readable from ROM hex dump
8. **Free Entry**: Single menu item for bonus codes hidden in the physical world (not in art)

---

## Build Instructions

### Prerequisites

1. **Install devkitPro** (provides devkitARM toolchain + libnds):

   **macOS:**
   ```bash
   # Install devkitPro pacman
   # Download from: https://github.com/devkitPro/pacman/releases/latest
   # Install the .pkg file, then reboot
   
   # Install NDS development tools
   sudo dkp-pacman -S nds-dev

   # Enter a selection (default=all):
   # Proceed with installation? [Y/n]
   ```

   **Linux (Debian/Ubuntu):**
   ```bash
   wget https://apt.devkitpro.org/install-devkitpro-pacman
   chmod +x ./install-devkitpro-pacman
   sudo ./install-devkitpro-pacman
   sudo dkp-pacman -S nds-dev
   ```

   **Windows:**
   ```bash
   # Download the graphical installer from https://github.com/devkitPro/installer/releases
   ```

2. **Set environment variables** (add to shell profile):
   ```bash
   export DEVKITPRO=/opt/devkitpro
   export DEVKITARM=$DEVKITPRO/devkitARM
   export PATH=$DEVKITPRO/tools/bin:$DEVKITARM/bin:$PATH
   ```

3. **Tell VSCode where to find devkitPro headers** (edit includePath in C/C++ extension settings)
   ```bash
   echo $DEVKITPRO # find installation path
   ```
   Create or edit `.vscode/c_cpp_properties.json` in your project root.
   Add or update the file with:
   ```json
      {
   "configurations": [
      {
         "name": "devkitARM",
         "includePath": [
         "${workspaceFolder}/**",
         "/opt/devkitpro/devkitARM/include",
         "/opt/devkitpro/libnds/include",
         "/opt/devkitpro/libfat-nds/include"
         ],
         "defines": [],
         "compilerPath": "/opt/devkitpro/devkitARM/bin/arm-none-eabi-gcc",
         "cStandard": "c99",
         "cppStandard": "c++11",
         "intelliSenseMode": "gcc-x64"
      }
   ],
   "version": 4
   }
   ```

### Building

```bash
cd dsi
make
```

This produces `asciicapsule.nds` — the complete application ROM with all content embedded.

### Cleaning

```bash
cd dsi
make clean
```

---

## Loading onto Nintendo DSi

### Requirements

- Nintendo DSi with **custom firmware** (Unlaunch + TWiLight Menu++)
- SD card (up to 32 GB, FAT32 formatted)
- Follow [dsi.cfw.guide](https://dsi.cfw.guide/) to install CFW if not already done

### Steps

1. **Build the application**: Run `make` in the `dsi/` directory to produce `asciicapsule.nds`
2. **Copy to SD card**: Place `asciicapsule.nds` anywhere on the SD card (e.g., root or a `homebrew/` folder)
3. **Insert SD card** into the Nintendo DSi
4. **Launch TWiLight Menu++** (boots automatically with Unlaunch, or select from system menu)
5. **Navigate** to `asciicapsule.nds` and launch it

The application will:
- Initialize and show the welcome splash screen
- Save/load unlock progress to `ascii_capsule_save.txt` on the SD card root

### Alternative: Flashcard

If using a DS flashcard (R4, etc.):
1. Copy `asciicapsule.nds` to the flashcard's microSD card
2. Launch from the flashcard's menu
3. Note: DLDI patching may be required for SD card save functionality

---

## Output Format

The compiled output is a **`.nds` (Nintendo DS ROM)** file. This is a binary format containing:
- ARM9 executable code (the application)
- NitroFS filesystem (embedded content files)
- ROM header with icon and metadata

The `.nds` format is the standard for both commercial and homebrew NDS/DSi software. TWiLight Menu++ and flashcard kernels natively load this format.

---

## To Rebuild

```bash
cd dsi && make clean && make
```
**Copy new `asciicapsule.nds`** to your SD card

## Testing

### Host-Side Tests

Core logic (content parsing, hash function, code matching, bonus codes, viewer scrolling) is portable C with no NDS dependencies. Tests compile and run on the development machine:

```bash
cd dsi/tests
make test
```

Currently **34 tests** covering:
- Hash function (determinism, case-insensitivity, known values, edge cases)
- Content loading (manifest parsing, bonus items, malformed input, overflow)
- Code checking (exact match, case-insensitive, wrong codes, bonus codes)
- Save/load progress (regular items, bonus items, missing files, full unlock)
- Viewer (load, scroll clamping, wide/tall content, empty files, reload reset)

### On-Device Testing

Use the **melonDS** emulator for testing:
```bash
# melonDS (recommended, better DSi emulation)
# Download from: https://melonds.kuribo64.net/downloads.php
melonDS dsi/asciicapsule.nds
```

---

## Content Manifest Format

### Regular items (art + message)

```
Display Name|HASH|art_file.txt|message_file.txt
```

### Bonus codes (free entry, no art)

```
+HASH|message_file.txt
```

### Computing a hash

```bash
python3 -c "
h=5381
for c in input('Code: ').upper(): h=((h<<5)+h)^ord(c); h&=0xFFFFFFFF
print(h)"
```

The hash function is djb2-xor applied to the uppercased code. This ensures:
- Codes are case-insensitive for the user
- The actual code text never appears in the ROM binary
- Anyone extending the app can compute hashes with the Python one-liner above

---

## Implementation Status

- [x] PLAN.md created
- [x] Project structure created
- [x] Makefile (devkitARM/libnds) — output: `asciicapsule.nds`
- [x] content.h/c — manifest loading, hash-based code checking, bonus codes
- [x] viewer.h/c — text loading, scrolling
- [x] keyboard.h/c — on-screen keyboard (D-pad + touch)
- [x] main.c — state machine (9 states incl. FREE_ENTRY), screen drawing
- [x] Content files (manifest with hashed codes, welcome, instructions, art, messages)
- [x] Free Entry feature — bonus codes hidden in the physical world
- [x] Code obfuscation — codes stored as djb2 hashes, not plain text
- [x] Host-side tests (34 tests, all passing)
- [x] Build and deployment documentation
