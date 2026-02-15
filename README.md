# ascii-memories

# Asciify - ASCII Art Image Converter

A command-line tool to convert images into ASCII art with multiple output format options.

## Features

- **Multiple Format Support**: JPEG, PNG, WEBP, TIFF
- **Adjustable Size**: Control output width (8-80 characters)
- **Multiple Output Formats**: Display in terminal, save as text, or save as HTML
- **Character Density Mapping**: Uses visual density to create accurate representations

## Installation

### Requirements

- Python 3.7+
- pip

### Setup

```bash
# Install dependencies
pixi init
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
# Use extended character set for more detail
python asciify.py image.png --extended

# Adjust contrast enhancement (values > 1.0 increase contrast)
python asciify.py image.png --contrast 1.5
```

## How It Works

### 1. Image Validation
Checks that the image has a supported extension (JPEG/JPG, PNG, WEBP, TIFF/TIF).

### 2. Size Clamping
Ensures the output size is between 8 and 80 characters wide (default: 80).

### 3. Image Loading
Reads the image using PIL/Pillow library.

### 4. Dimension Calculation
Calculates output dimensions while maintaining aspect ratio. Compensates for the fact that ASCII characters are taller than they are wide (~2:1 ratio).

### 5. Image Resizing (Pixelation)
Resizes the image to the target resolution using high-quality LANCZOS resampling.

### 6. Grayscale Conversion
Converts the image to grayscale for brightness-based character mapping.

### 7. ASCII Art Generation
Maps each pixel's brightness (0-255) to an ASCII character from the character set:
- Darker pixels → denser characters (e.g., @, #, %)
- Lighter pixels → lighter characters (e.g., ., :, space)

### 8. Output
Displays the ASCII art or saves it to the specified format.

## Character Set

The tool uses a carefully ordered character set based on visual density:

```
@%#*+=-:. 
```

Characters are ordered from darkest (most dense) to lightest (least dense), for best representation of image brightness.

## Output Formats

### Terminal Display
Prints ASCII art directly to the console for immediate viewing.

### Text File (.txt)
Saves ASCII art as plain text, preserving all characters and formatting.

### HTML File (.html)
Saves ASCII art as an HTML file with:
- Monospace font (Courier New)
- Black background with green text (classic terminal aesthetic)
- Proper character spacing and line height
- Responsive layout that centers the art

## Architecture

### Modules

1. **src/ascii_converter.py**: Core conversion logic
   - `ASCIIConverter` class handles all image processing
   - Modular methods for each step of the conversion
   - Configurable character sets

2. **src/ascii_output.py**: Output handling
   - `ASCIIOutputHandler` class manages file operations
   - Supports multiple output formats
   - Automatic directory creation

3. **asciify.py**: CLI interface
   - Argument parsing
   - User interaction
   - Error handling

## Examples

### Input Image → ASCII Art

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

### "Unsupported image format"
Ensure your image is in JPEG, PNG, WEBP, or TIFF format.

### "Image file not found"
Check that the file path is correct and the file exists.

### Output looks distorted
Try adjusting the size or using the `--extended` flag for more character options.

### Characters don't align properly
Ensure your terminal or text editor is using a monospace font.

## Performance

- Small images (< 500KB): Near-instant conversion
- Medium images (500KB - 5MB): < 1 second
- Large images (> 5MB): 1-3 seconds

Processing time scales with input image resolution, not output ASCII size.

## License

This project is provided as-is for educational and personal use.

## Contributing

Contributions are welcome! Areas for improvement:
- Color ASCII art support
- Animation support for videos
- Custom character set configuration
- Batch processing multiple images
- Interactive preview mode

## Acknowledgments

Inspired by the ascii-image-converter project and classic ASCII art traditions.