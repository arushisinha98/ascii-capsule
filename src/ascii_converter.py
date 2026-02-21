"""
ASCII Art Image Converter Module

This module handles the core logic for converting images to ASCII art.
It processes images by resizing, converting to grayscale, and mapping
pixel brightness to ASCII characters.
"""

from PIL import Image
import numpy as np
from typing import Tuple, Optional


class ASCIIConverter:
    """
    Converts images to ASCII art representation.
    
    The converter uses a character set ordered by visual density (darkness)
    to map pixel brightness values to appropriate ASCII characters.
    """
    
    # ASCII characters ordered from darkest to lightest based on visual density
    ASCII_CHARS = "@%#*+=-:. "

    # Extended ASCII characters
    EXTENDED_CHARS = ASCII_CHARS
    
    def __init__(self, use_extended: bool = False):
        """
        Initialize the converter with character set choice.
        
        Args:
            use_extended: If True, uses extended character set for more detail
        """
        if use_extended:
            self.chars = self.EXTENDED_CHARS
        else:
            self.chars = self.ASCII_CHARS
    
    def validate_image(self, image_path: str) -> bool:
        """
        Validate that the image has a supported extension.
        
        Args:
            image_path: Path to the image file
            
        Returns:
            True if extension is supported, False otherwise
        """
        supported_extensions = {'.jpg', '.jpeg', '.png', '.webp', '.tiff', '.tif'}
        extension = image_path.lower().split('.')[-1]
        return f'.{extension}' in supported_extensions
    
    def clamp_size(self, size: Optional[int]) -> int:
        """
        Clamp the size value between 8 and 80.
        
        Args:
            size: Desired output width in characters (or None for default)
            
        Returns:
            Clamped size value between 8 and 80
        """
        if size is None:
            return 80
        return max(8, min(80, size))
    
    def calculate_dimensions(self, img: Image.Image, target_width: int) -> Tuple[int, int]:
        """
        Calculate output dimensions maintaining aspect ratio.
        
        ASCII characters are typically taller than wide (roughly 2:1 ratio),
        so we adjust the height to compensate and maintain image proportions.
        
        Args:
            img: PIL Image object
            target_width: Desired width in characters
            
        Returns:
            Tuple of (width, height) in characters
        """
        aspect_ratio = img.height / img.width
        # Characters are ~2x taller than wide, so we compensate
        target_height = int(target_width * aspect_ratio * 0.55)
        return target_width, target_height
    
    def resize_image(self, img: Image.Image, new_width: int, new_height: int) -> Image.Image:
        """
        Resize image to target dimensions.
        
        Args:
            img: PIL Image object
            new_width: Target width in pixels (characters)
            new_height: Target height in pixels (characters)
            
        Returns:
            Resized PIL Image object
        """
        return img.resize((new_width, new_height), Image.Resampling.LANCZOS)
    
    def to_grayscale(self, img: Image.Image) -> Image.Image:
        """
        Convert image to grayscale.
        
        Args:
            img: PIL Image object
            
        Returns:
            Grayscale PIL Image object
        """
        return img.convert('L')
    
    def pixels_to_ascii(self, img: Image.Image) -> str:
        """
        Convert image pixels to ASCII characters.
        
        Each pixel's brightness (0-255) is mapped to a character from
        the character set, with darker pixels mapping to denser characters.
        
        Args:
            img: Grayscale PIL Image object
            
        Returns:
            String containing ASCII art with newlines
        """
        # Convert image to numpy array for efficient processing
        pixels = np.array(img)
        
        # Normalize pixel values to character index range
        # 0 (black) maps to densest char, 255 (white) maps to lightest char
        char_indices = ((pixels / 255) * (len(self.chars) - 1)).astype(int)
        
        # Build ASCII art string row by row
        ascii_art = []
        for row in char_indices:
            ascii_row = ''.join(self.chars[idx] for idx in row)
            ascii_art.append(ascii_row)
        
        return '\n'.join(ascii_art)
    
    def convert_image_to_ascii(self, image_path: str, size: Optional[int] = None) -> str:
        """
        Main conversion function that orchestrates the entire process.
        
        Args:
            image_path: Path to input image
            size: Desired width in characters (None for default 80)
            
        Returns:
            ASCII art as string
            
        Raises:
            ValueError: If image format is not supported
            FileNotFoundError: If image file doesn't exist
        """
        # Step 1: Validate image extension
        if not self.validate_image(image_path):
            raise ValueError(
                f"Unsupported image format. "
                f"Supported formats: JPEG, JPG, PNG, WEBP, TIFF, TIF"
            )
        
        # Step 2: Clamp size to valid range
        width = self.clamp_size(size)
        
        # Step 3: Read image
        try:
            img = Image.open(image_path)
        except FileNotFoundError:
            raise FileNotFoundError(f"Image file not found: {image_path}")
        except Exception as e:
            raise ValueError(f"Error reading image: {str(e)}")
        
        # Step 4: Calculate dimensions maintaining aspect ratio
        new_width, new_height = self.calculate_dimensions(img, width)
        
        # Step 5: Pixelate (resize) image to target resolution
        img = self.resize_image(img, new_width, new_height)
        
        # Step 6: Convert to grayscale
        img = self.to_grayscale(img)
        
        # Step 7: Convert pixels to ASCII art
        ascii_art = self.pixels_to_ascii(img)
        
        return ascii_art