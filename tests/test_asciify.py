#!/usr/bin/env python3
"""
Test script for ASCII Art Converter

This script creates test images and verifies all functionality.
"""


import os
import shutil
import subprocess
import tempfile
import pytest
from PIL import Image, ImageDraw, ImageFont



def create_test_images(directory):
    """Create various test images for conversion in the given directory."""
    # Test image 1: Simple gradient
    img1 = Image.new('L', (200, 200))
    draw = ImageDraw.Draw(img1)
    for y in range(200):
        brightness = int((y / 200) * 255)
        draw.rectangle([0, y, 200, y+1], fill=brightness)
    img1.save(os.path.join(directory, 'test_gradient.png'))

    # Test image 2: Circle
    img2 = Image.new('RGB', (300, 300), 'white')
    draw = ImageDraw.Draw(img2)
    draw.ellipse([50, 50, 250, 250], fill='black', outline='black')
    img2.save(os.path.join(directory, 'test_circle.png'))

    # Test image 3: Text
    img3 = Image.new('RGB', (400, 200), 'white')
    draw = ImageDraw.Draw(img3)
    try:
        font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 60)
    except:
        font = ImageFont.load_default()
    draw.text((50, 70), "ASCII", fill='black', font=font)
    img3.save(os.path.join(directory, 'test_text.png'))

    # Test image 4: Checkerboard pattern
    img4 = Image.new('RGB', (200, 200), 'white')
    draw = ImageDraw.Draw(img4)
    square_size = 25
    for i in range(0, 200, square_size):
        for j in range(0, 200, square_size):
            if (i // square_size + j // square_size) % 2 == 0:
                draw.rectangle([i, j, i+square_size, j+square_size], fill='black')
    img4.save(os.path.join(directory, 'test_checkerboard.png'))

    # Test image 5: Landscape-like
    img5 = Image.new('RGB', (400, 300), 'skyblue')
    draw = ImageDraw.Draw(img5)
    draw.rectangle([0, 200, 400, 300], fill='green')
    draw.polygon([100, 200, 200, 100, 300, 200], fill='gray')
    draw.ellipse([320, 30, 370, 80], fill='yellow')
    img5.save(os.path.join(directory, 'test_landscape.png'))



def run_test(description, command, cwd=None):
    """Run a test command and display results, capturing output for debugging."""
    print(f"\n{'='*60}")
    print(f"Test: {description}")
    print(f"Command: {command}")
    print(f"{'='*60}")
    result = subprocess.run(command, shell=True, cwd=cwd, capture_output=True, text=True)
    print("[STDOUT]\n" + (result.stdout or ""))
    print("[STDERR]\n" + (result.stderr or ""))
    if result.returncode == 0:
        print(f"✓ {description} - PASSED")
    else:
        print(f"✗ {description} - FAILED (exit code {result.returncode})")
    return result.returncode == 0



@pytest.fixture(scope="module")
def temp_test_dir():
    """Create a temporary directory for test images and outputs, and clean up after tests."""
    orig_cwd = os.getcwd()
    temp_dir = tempfile.mkdtemp(prefix="ascii_test_")
    os.chdir(temp_dir)
    try:
        yield temp_dir
    finally:
        os.chdir(orig_cwd)
        shutil.rmtree(temp_dir, ignore_errors=True)

@pytest.mark.order(1)
def test_ascii_art_converter(temp_test_dir):
    """Run all tests in a temp directory, ensuring cleanup."""
    # Copy all required source files into temp dir for subprocess calls
    import shutil as _shutil
    src_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), '../src'))
    for fname in ['asciify.py', 'ascii_converter.py', 'ascii_output.py']:
        _shutil.copy(os.path.join(src_dir, fname), temp_test_dir)
    # Create test images
    create_test_images(temp_test_dir)
    # Test cases
    tests = [
        ("Basic conversion (default size)",
         f"python asciify.py test_gradient.png"),
        ("Custom size (50 characters)",
         f"python asciify.py test_circle.png --size 50"),
        ("Size clamping (too small)",
         f"python asciify.py test_text.png --size 5"),
        ("Size clamping (too large)",
         f"python asciify.py test_text.png --size 100"),
        ("Save as text file",
         f"python asciify.py test_gradient.png --save-txt output_test.txt"),
        ("Save as HTML file",
         f"python asciify.py test_circle.png --save-html output_test.html"),
        ("Save to subdirectory (auto-create)",
         f"python asciify.py test_landscape.png --save-html test_output/landscape.html"),
        ("Combined options (size + save)",
         f"python asciify.py test_checkerboard.png --size 40 --save-txt test_output/checkerboard.txt"),
        ("Extended character set",
         f"python asciify.py test_text.png --size 60 --extended"),
    ]
    passed = 0
    failed = 0
    for description, command in tests:
        if run_test(description, command, cwd=temp_test_dir):
            passed += 1
        else:
            failed += 1
    # Check created files
    expected_files = [
        'output_test.txt',
        'output_test.html',
        os.path.join('test_output', 'landscape.html'),
        os.path.join('test_output', 'checkerboard.txt')
    ]
    for filepath in expected_files:
        fullpath = os.path.join(temp_test_dir, filepath)
        if not os.path.exists(fullpath):
            print(f"\n[DEBUG] Contents of temp_test_dir: {os.listdir(temp_test_dir)}")
            for root, dirs, files in os.walk(temp_test_dir):
                for name in files:
                    print(f"[DEBUG] File: {os.path.join(root, name)}")
                for name in dirs:
                    print(f"[DEBUG] Dir: {os.path.join(root, name)}")
        assert os.path.exists(fullpath), f"{filepath} not found"
        assert os.path.getsize(fullpath) > 0, f"{filepath} is empty"