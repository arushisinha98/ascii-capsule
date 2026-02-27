#!/usr/bin/env python3
"""
Asciify - ASCII Art Image Converter

Command-line tool to convert images to ASCII art.

Usage:
    asciify IMAGE.png --size 80
    asciify IMAGE.png --save-txt output.txt
    asciify IMAGE.png --save-html output.html
    asciify IMAGE.png --size 50 --save-html output/ascii.html
    asciify IMAGE.png --charset "%*:. "
    asciify IMAGE.png --conceal "Hidden Message" --difficulty hard

Author: ASCII Art Converter
"""

import argparse
import sys
from pathlib import Path
from src import ASCIIConverter, ASCIIOutputHandler, conceal


def parse_arguments():
    """
    Parse command-line arguments.
    
    Returns:
        Parsed arguments object
    """
    parser = argparse.ArgumentParser(
        description='Convert images to ASCII art',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s image.png --size 80
  %(prog)s photo.jpg --save-html output.html
  %(prog)s picture.png --size 50 --save-txt result.txt
  %(prog)s image.png --save-html output/image.html
  %(prog)s image.png --charset "%*:. "
  %(prog)s image.png --conceal "Hidden Message" --difficulty hard
        """
    )
    
    parser.add_argument(
        'image',
        help='Path to input image (JPEG, PNG, WEBP, TIFF)'
    )
    
    parser.add_argument(
        '--size',
        type=int,
        default=None,
        help='Output width in characters (8-80, default: 80)'
    )
    
    parser.add_argument(
        '--save-txt',
        dest='save_txt',
        metavar='FILEPATH',
        help='Save ASCII art as text file to specified path'
    )
    
    parser.add_argument(
        '--save-html',
        dest='save_html',
        metavar='FILEPATH',
        help='Save ASCII art as HTML file to specified path'
    )
    
    parser.add_argument(
        '--charset',
        type=str,
        default=None,
        help='Character set in order of visual density (dark to light)'
    )
    
    parser.add_argument(
        '--conceal',
        type=str,
        default=None,
        help='Conceal a message within the ASCII art'
    )
    
    parser.add_argument(
        '--difficulty',
        type=str,
        default="hard",
        choices=["easy", "medium", "hard"],
        help='Difficulty level for concealing text (default: hard)'
    )
    
    return parser.parse_args()


def main():
    """
    Main entry point for the CLI application.
    """
    # Parse command-line arguments
    args = parse_arguments()
    
    try:
        # Initialize converter
        converter = ASCIIConverter(charset=args.charset.replace('"\'', '') if args.charset else None)
        
        # Convert image to ASCII art
        ascii_art = converter.convert_image_to_ascii(args.image, args.size)
        
        # Conceal text if requested
        if args.conceal:
            ascii_art = conceal(ascii_art, args.conceal.replace('"\'', ''), difficulty=args.difficulty)
        
        # Initialize output handler
        output_handler = ASCIIOutputHandler()
        saved_files = []
        
        if args.save_txt:
            output_handler.save_as_text(ascii_art, args.save_txt)
            saved_files.append(args.save_txt)
            print(f"Saved as text: {args.save_txt}", file=sys.stderr)
        
        if args.save_html:
            image_name = Path(args.image).stem
            output_handler.save_as_html(ascii_art, args.save_html, title=f"{image_name}")
            saved_files.append(args.save_html)
            print(f"Saved as HTML: {args.save_html}", file=sys.stderr)
        
        # Display to console if not saving to files
        if not saved_files:
            print(ascii_art)

    except FileNotFoundError as e:
        print(f"Error: {str(e)}", file=sys.stderr)
        sys.exit(1)
    
    except ValueError as e:
        print(f"Error: {str(e)}", file=sys.stderr)
        sys.exit(1)
    
    except IOError as e:
        print(f"Error: {str(e)}", file=sys.stderr)
        sys.exit(1)
    
    except KeyboardInterrupt:
        print("\nOperation cancelled by user", file=sys.stderr)
        sys.exit(130)
    
    except Exception as e:
        print(f"Unexpected error: {str(e)}", file=sys.stderr)
        sys.exit(1)


if __name__ == '__main__':
    main()
