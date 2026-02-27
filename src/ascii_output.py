"""
ASCII Art Output Module

This module handles saving ASCII art to various formats including
plain text and HTML with monospace formatting.
"""

import os
from pathlib import Path
from typing import Optional


class ASCIIOutputHandler:
    """
    Handles saving ASCII art to different output formats.
    """
    
    @staticmethod
    def save_as_text(ascii_art: str, filepath: str) -> None:
        """
        Save ASCII art as plain text file.
        
        Args:
            ascii_art: ASCII art string
            filepath: Path where to save the file (including filename)
            
        Raises:
            IOError: If file cannot be written
        """
        # Create directory if it doesn't exist
        directory = os.path.dirname(filepath)
        if directory:
            os.makedirs(directory, exist_ok=True)
        
        # Write ASCII art to file
        try:
            with open(filepath, 'w', encoding='utf-8') as f:
                f.write(ascii_art)
        except IOError as e:
            raise IOError(f"Error saving text file: {str(e)}")
    
    @staticmethod
    def save_as_html(ascii_art: str, filepath: str, title: str = "ASCII Art") -> None:
        """
        Save ASCII art as HTML file with proper formatting.
        
        The HTML uses monospace font and preserves whitespace to ensure
        the ASCII art displays correctly in web browsers.
        
        Args:
            ascii_art: ASCII art string
            filepath: Path where to save the file (including filename)
            title: HTML page title
            
        Raises:
            IOError: If file cannot be written
        """
        # Create directory if it doesn't exist
        directory = os.path.dirname(filepath)
        if directory:
            os.makedirs(directory, exist_ok=True)
        
        # Colorize alphanumeric text in black
        def colorize_line(line):
            out = []
            for c in line:
                if c.isalnum():
                    out.append(f'<span style="color:#FFF">{c}</span>')
                else:
                    out.append(c)
            return ''.join(out)
        html_lines = [colorize_line(line) for line in ascii_art.split('\n')]
        html_content = f"""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>{title}</title>
    <style>
        body {{
            background-color: #000000;
            color: #00ff00;
            font-family: 'Courier New', Courier, monospace;
            padding: 20px;
            margin: 0;
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
        }}
        pre {{
            font-size: 12px;
            line-height: 1.0;
            white-space: pre;
            margin: 0;
            letter-spacing: 0.05em;
        }}
        .container {{
            max-width: 100%;
            overflow-x: auto;
        }}
    </style>
</head>
<body>
    <div class="container">
        <pre>{'\n'.join(html_lines)}</pre>
    </div>
</body>
</html>"""
        
        # Write HTML to file
        try:
            with open(filepath, 'w', encoding='utf-8') as f:
                f.write(html_content)
        except IOError as e:
            raise IOError(f"Error saving HTML file: {str(e)}")
    
    @staticmethod
    def get_output_path(input_path: str, output_dir: Optional[str], extension: str) -> str:
        """
        Generate output file path based on input image path.
        
        Args:
            input_path: Path to input image
            output_dir: Directory where to save output (or None for same directory)
            extension: File extension (e.g., 'txt', 'html')
            
        Returns:
            Full path for output file
        """
        input_file = Path(input_path)
        filename = input_file.stem  # filename without extension
        
        if output_dir:
            output_path = Path(output_dir) / f"{filename}.{extension}"
        else:
            output_path = input_file.parent / f"{filename}.{extension}"
        
        return str(output_path)
