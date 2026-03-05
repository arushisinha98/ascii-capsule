
# ascii-capsule

ascii-capsule is a dual-component project:

1. **Asciify** — a Python command-line tool to convert images into ASCII art, with advanced features and output options.
2. **ASCII Capsule** — a Nintendo DSi homebrew application for viewing and unlocking ASCII art challenges, designed for the DSi's hardware constraints.

---

# 1. Asciify — ASCII Art Image Converter (Python CLI)

A command-line tool to convert images into ASCII art with multiple output format options.

## Features

- **Multiple Format Support**: JPEG, PNG, WEBP, TIFF
- **Adjustable Size**: Control output width (8-80 characters)
- **Multiple Output Formats**: Display in terminal, save as text, or save as HTML
- **Character Density Mapping**: Uses visual density to create representations
- **Text Concealing**: Hide messages in the art with adjustable difficulty

## Installation

### Requirements

- Python 3.7+
- pip

### Setup

```bash
# Install dependencies
pixi install
```

## Usage

### Basic Usage

```bash
# Convert image and display in terminal (default 80 characters wide)
python asciify.py PATH-TO-IMAGE.png

# Specify custom size
python asciify.py PATH-TO-IMAGE.png --size 50
# Size is clamped between 8 and 80
```

### Saving Output

```bash
# Save as text file
python asciify.py PATH-TO-IMAGE.png --save-txt PATH-TO-OUTPUT.txt

# Save as HTML file
python asciify.py PATH-TO-IMAGE.png --save-html PATH-TO-OUTPUT.html
# Save directory will be created if it doesn't already exist

# Combine size and save options
python asciify.py PATH-TO-IMAGE.jpg --size 60 --save-html PATH-TO-OUTPUT.html
```

### Advanced Options

```bash
# Specify your own character set
python asciify.py image.png --charset "*:.  "

# Hide a message (defaults to hard)
python asciify.py image.png --conceal "Hidden Message" --difficulty "hard"
```

## Run Tests Locally

```bash
export PYTHONPATH=src
pixi run pytest -v .
```

## How It Works

1. **Image Validation**: Checks that the image has a supported extension (JPEG/JPG, PNG, WEBP, TIFF/TIF).
2. **Size Clamping**: Ensures the output size is between 8 and 80 characters wide (default: 80).
3. **Image Loading**: Reads the image using PIL/Pillow library.
4. **Dimension Calculation**: Maintains aspect ratio, compensating for ASCII character proportions (~2:1).
5. **Image Resizing**: Uses high-quality LANCZOS resampling.
6. **Grayscale Conversion**: For brightness-based character mapping.
7. **ASCII Art Generation**: Maps pixel brightness to ASCII characters:
   - Darker pixels → denser characters (e.g., @, #, %)
   - Lighter pixels → lighter characters (e.g., ., :, space)
8. **Text Concealing**: Hides messages in the art, with difficulty based on density.
9. **Output**: Displays or saves the ASCII art.

## Character Set

```
@%#*+=-:. 
```
Characters are ordered from darkest (most dense) to lightest (least dense).

## Output Formats

- **Terminal Display**: Prints ASCII art to the console.
- **Text File (.txt)**: Saves as plain text.
- **HTML File (.html)**: Monospace font, black background, green text, responsive layout.

## Architecture

- `src/ascii_converter.py`: Core conversion logic (`ASCIIConverter` class)
- `src/ascii_output.py`: Output handling (`ASCIIOutputHandler` class)
- `src/text_concealer.py`: Text concealing logic (`conceal` function)
- `asciify.py`: CLI interface

## Example

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

## Troubleshooting

- **"Unsupported image format"**: Use JPEG, PNG, WEBP, or TIFF.
- **"Image file not found"**: Check the file path.
- **Characters don't align**: Use a monospace font.

## Performance

- Small images (< 500KB): Near-instant
- Medium (500KB - 5MB): < 1 second
- Large (> 5MB): 1-3 seconds

---

# 2. ASCII Capsule — Nintendo DSi Application

ASCII Capsule is a homebrew application for the Nintendo DSi, designed to present ASCII art challenges and messages, with unlockable content and a custom UI tailored for the DSi's hardware.

## DSi Hardware Constraints

| Spec         | Value                                 |
|--------------|---------------------------------------|
| Screens      | 2 × TFT-LCD, 256×192 pixels each      |
| Console text | 32 columns × 24 rows per screen (8×8) |
| CPU          | ARM9 @ 133 MHz + ARM7 @ 33 MHz        |
| RAM          | 16 MB                                 |
| Storage      | SD card (up to 32 GB) + 256 MB flash  |
| Input        | D-pad, A/B/X/Y, L/R, touch            |
| Audio        | AAC, speakers/headphone               |

### Screen Size vs. ASCII Art Width

ASCII art is 60 characters wide by default, but the DSi displays 32 characters per line. The app implements horizontal/vertical/page scrolling for navigation.

---

## Application Architecture

### State Machine

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

### Screen Layout

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

### Module Design

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

### Key Design Decisions

1. **C over C++**: Simpler, smaller binary
2. **NitroFS for content**: All data embedded in .nds
3. **FAT for save data**: Progress saved to SD card
4. **Console text mode**: Uses libnds PrintConsole
5. **Software scrolling**: Redraws visible window on scroll
6. **D-pad + touch keyboard**: Full alphanumeric input
7. **Hashed codes**: Codes stored as djb2-xor hashes
8. **Free Entry**: Bonus codes hidden in the real world

---

## Building the DSi App

### Prerequisites

1. **Install devkitPro** (devkitARM toolchain + libnds)
2. **Set environment variables**:
   ```bash
   export DEVKITPRO=/opt/devkitpro
   export DEVKITARM=$DEVKITPRO/devkitARM
   export PATH=$DEVKITPRO/tools/bin:$DEVKITARM/bin:$PATH
   ```
3. **VSCode C/C++ config**: Add devkitPro include paths to `.vscode/c_cpp_properties.json`.

### Building

```bash
cd dsi
make
```
Produces `asciicapsule.nds` (the complete ROM).

### Cleaning

```bash
cd dsi
make clean
```

---

## Loading onto Nintendo DSi

1. **Build the app**: `make` in `dsi/` → `asciicapsule.nds`
2. **Copy to SD card**: Place `.nds` file on SD card
3. **Insert SD card** into DSi
4. **Launch TWiLight Menu++**
5. **Navigate and launch** the app

Progress is saved to `ascii_capsule_save.txt` on the SD card root.

---

## Testing

### Host-Side Tests

```bash
cd dsi/tests
make test
```
Runs 34 tests for content, code matching, viewer, save/load, etc.

### On-Device Testing

Use the melonDS emulator:
```bash
melonDS dsi/asciicapsule.nds
```

---

## Content Manifest Format

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

## License

This project is provided as-is for educational and personal use.

## Contributing

Contributions are welcome! Areas for improvement:
- Batch processing multiple images
- More interesting features

## Acknowledgments

Inspired by the ascii-image-converter project.