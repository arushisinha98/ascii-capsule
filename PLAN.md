# ASCII Memories — Nintendo DSi Application Plan

## Assumption Corrections

### 1. "Must I write the code in C++ for it to be executable on the Nintendo DSi?"

**Partially correct.** You must use C or C++ — but C++ is not *required*. The devkitARM toolchain (from devkitPro) compiles both C and C++ to native ARM machine code for the DSi's ARM9 processor. Our implementation uses **C** for simplicity and smaller binary size. There is no interpreter, VM, or scripting language runtime available on the DSi.

### 2. "Anything that requires user input can only be numeric?"

**Incorrect.** The Nintendo DSi has a **full touchscreen** (256×192 pixels on the bottom screen) that allows implementing a complete on-screen keyboard with letters, numbers, symbols, and spaces. Additionally, the DSi has physical buttons: D-pad, A/B/X/Y, L/R shoulder, Start, Select. Our application implements an on-screen keyboard supporting full alphanumeric input (A–Z, 0–9, space) via both **touch taps** and **D-pad navigation**.

### 3. "The application must be containerized. But I can use the bluetooth, wifi, or any other special DSI capabilities."

**Partially incorrect — key corrections:**

- **"Containerized"**: Docker-style containerization does not apply to the Nintendo DSi. The application compiles to a **`.nds` ROM file**, which is a self-contained binary. All data files (ASCII art, letters, manifest) are embedded directly into this ROM using **NitroFS** (a read-only filesystem embedded within the .nds file). In this sense, the `.nds` file IS the "container" — a single file with code + data. Progress is saved to the SD card via FAT filesystem.

- **Bluetooth**: The Nintendo DSi **does NOT have Bluetooth**. This is a common misconception.

- **Wi-Fi**: The DSi has 802.11b/g Wi-Fi, but Nintendo Wi-Fi Connection services were **discontinued on May 20, 2014**. Local ad-hoc Wi-Fi is theoretically possible but not practical for this use case.

- **Primary Input**: The **touchscreen** is the DSi's most powerful input method. Physical buttons handle navigation and scrolling.

---

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

The existing ASCII art is **80 characters wide**. The DSi console displays **32 characters per line**. The application implements:
- **Horizontal scrolling** (D-pad Left/Right) to view the full 80-char width
- **Vertical scrolling** (D-pad Up/Down) for tall content
- **Page scrolling** (L/R shoulder buttons) for quick navigation

Alternative: Art can be regenerated at 32-char width using the included `asciify.py` tool, but this significantly reduces detail.

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
                          WRONG_CODE   LETTER (memory)
                                ↓         ↓
                              (retry)    MENU
                                    
                    MENU → INSTRUCTIONS (SELECT button)
                    MENU → LETTER (X button, if unlocked)
```

### Screen Layout

| State | Top Screen | Bottom Screen |
|-------|-----------|--------------|
| SPLASH | Welcome ASCII art | App title + loading |
| WELCOME | Scrollable letter | Controls help |
| MENU | Item list with status | Navigation help |
| CHALLENGE | Scrollable ASCII art | Challenge instructions |
| CODE_ENTRY | ASCII art (reference) | On-screen keyboard |
| WRONG_CODE | ASCII art (reference) | Error + retry prompt |
| LETTER | Scrollable memory text | Unlock confirmation |
| INSTRUCTIONS | Scrollable how-to guide | Navigation help |

### Module Design

```
dsi/
├── Makefile                 # devkitARM build configuration
├── source/
│   ├── main.c               # Entry point, state machine, NDS drawing
│   ├── content.h / .c       # Content loading (portable, testable)
│   ├── viewer.h / .c        # Text viewer + scrolling (portable, testable)
│   └── keyboard.h / .c      # On-screen keyboard (NDS-specific)
├── nitrofiles/              # Embedded in .nds ROM via NitroFS
│   ├── manifest.txt         # Content manifest (name|code|art|letter)
│   ├── welcome_art.txt      # Startup ASCII art
│   ├── welcome_letter.txt   # Welcome letter
│   ├── instructions.txt     # How to extend the app
│   ├── art/                 # ASCII art files with concealed codes
│   │   ├── 2SHOO.txt
│   │   ├── BAYB.txt
│   │   └── ... (9 files)
│   └── letters/             # Memory/letter text files
│       ├── 2SHOO.txt
│       ├── BAYB.txt
│       └── ... (9 files)
├── tests/
│   ├── Makefile             # Host-side test compilation
│   └── test_content.c       # Tests for content + viewer logic
└── tools/
    └── prepare_content.sh   # Script to copy/regenerate content
```

### Key Design Decisions

1. **C over C++**: Simpler, smaller binary, fewer dependencies on NDS
2. **NitroFS for content**: All data embedded in .nds — single file deployment
3. **FAT for save data**: Progress saved to SD card as `ascii_memories_save.txt`
4. **Console text mode**: Uses libnds PrintConsole for text rendering — reliable, well-tested
5. **Software scrolling**: Redraws visible 32×24 window from content buffer on scroll
6. **D-pad + touch keyboard**: Full alphanumeric input, accessible both ways

---

## Build Instructions

### Prerequisites

1. **Install devkitPro** (provides devkitARM toolchain + libnds):

   **macOS:**
   ```bash
   # Install Xcode command line tools
   xcode-select --install
   
   # Install devkitPro pacman
   # Download from: https://github.com/devkitPro/pacman/releases/latest
   # Install the .pkg file, then reboot
   
   # Install NDS development tools
   sudo dkp-pacman -S nds-dev
   ```

   **Linux (Debian/Ubuntu):**
   ```bash
   wget https://apt.devkitpro.org/install-devkitpro-pacman
   chmod +x ./install-devkitpro-pacman
   sudo ./install-devkitpro-pacman
   sudo dkp-pacman -S nds-dev
   ```

   **Windows:**
   Download the graphical installer from https://github.com/devkitPro/installer/releases

2. **Set environment variables** (add to shell profile):
   ```bash
   export DEVKITPRO=/opt/devkitpro
   export DEVKITARM=$DEVKITPRO/devkitARM
   export PATH=$DEVKITPRO/tools/bin:$DEVKITARM/bin:$PATH
   ```

### Building

```bash
cd dsi
make
```

This produces `dsi.nds` — the complete application ROM with all content embedded.

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

1. **Build the application**: Run `make` in the `dsi/` directory to produce `dsi.nds`
2. **Copy to SD card**: Place `dsi.nds` anywhere on the SD card (e.g., root or a `homebrew/` folder)
3. **Insert SD card** into the Nintendo DSi
4. **Launch TWiLight Menu++** (boots automatically with Unlaunch, or select from system menu)
5. **Navigate** to `dsi.nds` and launch it

The application will:
- Initialize and show the welcome splash screen
- Save/load unlock progress to `ascii_memories_save.txt` on the SD card root

### Alternative: Flashcard

If using a DS flashcard (R4, etc.):
1. Copy `dsi.nds` to the flashcard's microSD card
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

## How to Extend (Add Your Own Content)

### Adding a New Memory Item

1. **Create ASCII art** from an image:
   ```bash
   python asciify.py your_image.png --size 80 --conceal "YOUR CODE" --difficulty hard --save-txt outputs/YOURITEM.txt
   ```

2. **Write a letter/memory** as a plain text file. For best display on DSi (no horizontal scrolling for text), keep lines under 32 characters:
   ```
   Dear friend,

   This is a memory about
   the time we...
   
   (wrap at 32 characters)
   ```

3. **Copy files** into the NitroFS content directories:
   ```bash
   cp outputs/YOURITEM.txt dsi/nitrofiles/art/YOURITEM.txt
   # Create/edit dsi/nitrofiles/letters/YOURITEM.txt with your letter
   ```

4. **Update the manifest** (`dsi/nitrofiles/manifest.txt`):
   ```
   Your Item|YOUR CODE|YOURITEM.txt|YOURITEM.txt
   ```
   Format: `Display Name|Hidden Code|Art Filename|Letter Filename`

5. **Rebuild**:
   ```bash
   cd dsi && make clean && make
   ```

6. **Copy new `dsi.nds`** to your SD card

### Tips

- **Codes** are case-insensitive (user can enter upper or lower case)
- **Difficulty** affects how well the code blends into the art (`easy`/`medium`/`hard`)
- **Art width**: 80 chars requires horizontal scrolling on DSi; 32 chars fits without scrolling
- **Letter width**: Keep lines ≤32 chars for best readability without horizontal scrolling
- Maximum items: 16 (adjustable in `content.h` via `MAX_ITEMS`)
- Maximum text lines: 512 per file (adjustable in `viewer.h`)

---

## Testing

### Host-Side Tests

Core logic (content parsing, code matching, viewer scrolling) is portable C with no NDS dependencies. Tests compile and run on the development machine:

```bash
cd dsi/tests
make test
```

### On-Device Testing

Use the **DeSmuME** or **melonDS** emulator for rapid testing:
```bash
# melonDS (recommended, better DSi emulation)
melonDS dsi/dsi.nds

# DeSmuME
desmume dsi/dsi.nds
```

---

## Implementation Status

- [x] PLAN.md created
- [x] Assumption corrections documented
- [x] Project structure created
- [x] Makefile (devkitARM/libnds)
- [x] content.h/c — manifest loading, code checking
- [x] viewer.h/c — text loading, scrolling
- [x] keyboard.h/c — on-screen keyboard (D-pad + touch)
- [x] main.c — state machine, screen drawing
- [x] Content files (manifest, welcome, instructions, art, letters)
- [x] Host-side tests
- [x] Build and deployment documentation
