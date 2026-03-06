<div align="center">
   <h1>ascii-capsule</h1>
   <p><b>Turn images into ASCII art. Conceal codes to unlock messages on your Nintendo DSi!</b></p>
   <p>
      <img src="https://img.shields.io/badge/python-3.7%2B-blue" alt="Python 3.7+">
      <img src="https://img.shields.io/github/actions/workflow/status/arushisinha98/ascii-capsule/.github%2Fworkflows%2Ftest.yml" alt="Build Status">
   </p>
</div>

---

<details open>
<summary><b>Table of Contents</b></summary>

- [About](#about)
- [Features](#features)
- [Getting Started](#getting-started)
   - [Installation](#installation)
   - [Usage](#usage)
   - [Testing](#testing)
- [Architecture](#architecture)
- [Performance](#performance)
- [Contributing](#contributing)
- [License](#license)
- [Acknowledgments](#acknowledgments)

</details>

---

## About

**ascii-capsule** is a dual-component project:

- **Asciify**: Python CLI tool to convert images into ASCII art, with advanced features and output options.
- **ASCII Capsule**: Nintendo DSi homebrew app for viewing and unlocking ASCII art challenges, designed for the DSi's hardware constraints.

---

## Features

#### ASCII Image Creation

- **Multiple Format Support to Asciify**: JPEG, PNG, WEBP, TIFF
- **Adjustable Size**: Control output width (8-80 characters)
- **Multiple Output Formats**: Terminal, text, or HTML
- **Character Density Mapping**: Visual density for realistic ASCII
- **Text Concealing**: Hide messages in the art with adjustable difficulty

#### DSi Design Decisions

1. **C over C++**: Simpler, smaller binary
2. **NitroFS for content**: All data embedded in .nds
3. **FAT for save data**: Progress saved to SD card
4. **Console text mode**: Uses libnds PrintConsole
5. **Software scrolling**: Redraws visible window on scroll
6. **D-pad + touch keyboard**: Full alphanumeric input
7. **Hashed codes**: Codes stored as djb2-xor hashes
8. **Free Entry**: Bonus codes hidden in the physical world

---

## Getting Started

### Installation

**Requirements:**

- Python 3.7+
- pip

**Setup:**

```bash
pixi install
```

---

### Usage

#### Example

```
Original Image (landscape.jpg)     ASCII Art (--size 40)
┌─────────────────┐                                   ...                  
│                 │                                 ..   .          
│    Mountain     │        →                      ..  +*. ....       
│   Landscape     │                           ....  :#@%%:    .      
│                 │                          .    .*@@@.+@-+=. ..     
└─────────────────┘                        .  .=:-#*@@#  *@@%#=  ..   
                                         .  .=%%@%. -@%..:#@#:##-  .. 
                                          .=#*: -%*. -#.. .#@- =%*:        
                                       ..=*+:  . .*=. .. ...*#. .+#+...        
```

#### Basic Usage

```bash
# Convert image and display in terminal (default 80 characters wide)
python asciify.py PATH-TO-IMAGE.png

# Specify custom size
python asciify.py PATH-TO-IMAGE.png --size 50
# Size is clamped between 8 and 80
```

#### Saving Output

```bash
# Save as text file
python asciify.py PATH-TO-IMAGE.png --save-txt PATH-TO-OUTPUT.txt

# Save as HTML file
python asciify.py PATH-TO-IMAGE.png --save-html PATH-TO-OUTPUT.html
# Save directory will be created if it doesn't already exist

# Combine size and save options
python asciify.py PATH-TO-IMAGE.jpg --size 60 --save-html PATH-TO-OUTPUT.html
```

#### Advanced Options

```bash
# Specify your own character set
python asciify.py image.png --charset "*: .  "

# Hide a message (defaults to hard)
python asciify.py image.png --conceal "Hidden Message" --difficulty "hard"
```

#### DSi App Prerequisites

1. <b>Install devkitPro</b> (devkitARM toolchain + libnds)
2. <b>Set environment variables</b>:
    ```bash
    export DEVKITPRO=/opt/devkitpro
    export DEVKITARM=$DEVKITPRO/devkitARM
    export PATH=$DEVKITPRO/tools/bin:$DEVKITARM/bin:$PATH
    ```
3. <b>Build</b>
   ```bash
   cd dsi
   make clean # clean
   make
   ```
   Produces `asciicapsule.nds` (the complete ROM).
4. <b>Load onto Nintendo DSi</b>
   - Build the app: `make` in `dsi/` → `asciicapsule.nds`
   - Copy to SD card: Place `.nds` file on SD card
   - Insert SD card into DSi
   - Launch TWiLight Menu++
   - Navigate and launch the app
   Progress is saved to `ascii_capsule_save.txt` on the SD card root.

---

### Testing

#### Host-Side Tests

```bash
export PYTHONPATH=src
pixi run pytest -v .
```

```bash
cd dsi/tests
make test
```
Runs 34 tests for content, code matching, viewer, save/load, etc.

#### On-Device Testing

Use the melonDS emulator:
```bash
melonDS dsi/asciicapsule.nds
```

---

## Architecture

#### ASCII Image Creation

- `src/ascii_converter.py`: Core conversion logic (`ASCIIConverter` class)
- `src/ascii_output.py`: Output handling (`ASCIIOutputHandler` class)
- `src/text_concealer.py`: Text concealing logic (`conceal` function)
- `asciify.py`: CLI interface

#### DSi State Machine

```
SPLASH → WELCOME → MENU
                              ↓
                     CHALLENGE → CODE_ENTRY
                              ↓         ↓
                     WRONG_CODE   MESSAGE (unlocked)
                              ↓         ↓
                           (retry)    MENU

            MENU → FREE_ENTRY (bonus codes)
                           ↓         ↓
                     WRONG_CODE   MESSAGE (unlocked)
                           ↓         ↓
                        (retry)    MENU

            MENU → INSTRUCTIONS
            MENU → MESSAGE (if unlocked)
```

#### Screen Layout

| State         | Top Screen         | Bottom Screen                |
|---------------|-------------------|------------------------------|
| SPLASH        | Welcome ASCII art | App title + loading          |
| WELCOME       | Scrollable letter | Controls help                |
| MENU          | Item list/status  | Navigation help + progress   |
| CHALLENGE     | ASCII art         | Challenge instructions       |
| CODE_ENTRY    | ASCII art         | On-screen keyboard           |
| WRONG_CODE    | ASCII art         | Error + retry prompt         |
| FREE_ENTRY    | Bonus info        | On-screen keyboard           |
| MESSAGE       | Message text      | Unlock confirmation          |
| INSTRUCTIONS  | How-to guide      | Navigation help              |

#### Module Design

```
dsi/
├── Makefile                 # devkitARM build
├── source/
│   ├── main.c               # State machine, NDS drawing
│   ├── content.h/.c         # Content loading, code checking
│   ├── viewer.h/.c          # Text viewer, scrolling
│   └── keyboard.h/.c        # On-screen keyboard
├── nitrofiles/              # Embedded content
│   ├── manifest.txt         # Content manifest
│   ├── welcome_art.txt      # Startup ASCII art
│   ├── welcome_letter.txt   # Welcome letter
│   ├── instructions.txt     # How-to
│   ├── art/                 # ASCII art files
│   └── messages/            # Message text files
├── tests/                   # Host-side tests
└── tools/                   # Content prep scripts
```

#### Content Manifest Format

Regular items:
```
Display Name|HASH|art_file.txt|message_file.txt
```
Bonus codes:
```
+HASH|message_file.txt
```
Hash function (djb2-xor, case-insensitive):
```python
h=5381
for c in input('Code: ').upper(): h=((h<<5)+h)^ord(c); h&=0xFFFFFFFF
print(h)
```

---

## Performance

#### DSi Hardware Constraints

| Spec         | Value                                 |
|--------------|---------------------------------------|
| Screens      | 2 × TFT-LCD, 256×192 pixels each      |
| Console text | 32 columns × 24 rows per screen (8×8) |
| CPU          | ARM9 @ 133 MHz + ARM7 @ 33 MHz        |
| RAM          | 16 MB                                 |
| Storage      | SD card (up to 32 GB) + 256 MB flash  |
| Input        | D-pad, A/B/X/Y, L/R, touch            |
| Audio        | AAC, speakers/headphone               |

#### Troubleshooting

- **"Unsupported image format"**: Use JPEG, PNG, WEBP, or TIFF.
- **"Image file not found"**: Check the file path.
- **"Characters don't align"**: Use a monospace font.

ASCII art is 60 characters wide by default, but the DSi displays 32 characters per line. The app implements horizontal/vertical/page scrolling for navigation.

---

## Contributing

Contributions are welcome! Please open issues or pull requests for:
- Batch processing multiple images
- More output formats (SVG, PDF)
- Enhanced DSi app features
- Any other ideas!

---

## License

This project is provided as-is for educational and personal use.

---

## Acknowledgments

Inspired by the ascii-image-converter project.