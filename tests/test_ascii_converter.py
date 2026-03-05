import os
import tempfile
import numpy as np
import pytest
from PIL import Image
from ascii_converter import ASCIIConverter

def test_default_charset():
	converter = ASCIIConverter()
	assert converter.chars == ASCIIConverter.ASCII_CHARS

def test_custom_charset():
	custom = "#- "
	converter = ASCIIConverter(charset=custom)
	assert converter.chars == custom

@pytest.mark.parametrize("filename,expected", [
	("test.jpg", True),
	("test.jpeg", True),
	("test.png", True),
	("test.webp", True),
	("test.tiff", True),
	("test.tif", True),
	("test.bmp", False),
	("test.txt", False),
])
def test_validate_image(filename, expected):
	converter = ASCIIConverter()
	assert converter.validate_image(filename) == expected

@pytest.mark.parametrize("input_size,expected", [
	(None, 80),
	(100, 80),
	(8, 8),
	(7, 8),
	(50, 50),
	(0, 8),
	(-10, 8),
])
def test_clamp_size(input_size, expected):
	converter = ASCIIConverter()
	assert converter.clamp_size(input_size) == expected

def test_calculate_dimensions_aspect():
	converter = ASCIIConverter()
	img = Image.new("RGB", (200, 100))  # width=200, height=100
	width, height = converter.calculate_dimensions(img, 40)
	# Height should be less than width due to aspect correction
	assert width == 40
	assert height == int(40 * (100/200) * 0.55)

def test_resize_image():
	converter = ASCIIConverter()
	img = Image.new("RGB", (10, 10))
	resized = converter.resize_image(img, 5, 3)
	assert resized.size == (5, 3)

def test_to_grayscale():
	converter = ASCIIConverter()
	img = Image.new("RGB", (5, 5), color=(100, 150, 200))
	gray = converter.to_grayscale(img)
	assert gray.mode == 'L'

def test_pixels_to_ascii_simple():
	converter = ASCIIConverter()
	# Create a 2x2 grayscale image with known values
	arr = np.array([[0, 255], [128, 64]], dtype=np.uint8)
	img = Image.fromarray(arr, mode='L')
	ascii_art = converter.pixels_to_ascii(img)
	# Should map darkest to first char, lightest to last
	chars = converter.chars
	expected = f"{chars[0]}{chars[-1]}\n{chars[len(chars)//2]}{chars[1]}"
	assert isinstance(ascii_art, str)
	assert ascii_art.count('\n') == 1

def test_convert_image_to_ascii_full_pipeline():
	converter = ASCIIConverter()
	# Create a temp image file
	arr = np.linspace(0, 255, 100).reshape(10, 10).astype(np.uint8)
	img = Image.fromarray(arr, mode='L')
	with tempfile.NamedTemporaryFile(suffix='.png', delete=False) as tmp:
		img.save(tmp.name)
		tmp_path = tmp.name
	try:
		ascii_art = converter.convert_image_to_ascii(tmp_path, size=10)
		assert isinstance(ascii_art, str)
		assert ascii_art.count('\n') > 0
	finally:
		os.remove(tmp_path)

def test_convert_image_to_ascii_invalid_format():
	converter = ASCIIConverter()
	with pytest.raises(ValueError):
		converter.convert_image_to_ascii("file.unsupported")

def test_convert_image_to_ascii_file_not_found():
	converter = ASCIIConverter()
	with pytest.raises(FileNotFoundError):
		converter.convert_image_to_ascii("not_a_real_file.png")

def test_convert_image_to_ascii_corrupt_file():
	converter = ASCIIConverter()
	# Create a temp file that is not an image
	with tempfile.NamedTemporaryFile(suffix='.png', delete=False) as tmp:
		tmp.write(b'notanimage')
		tmp_path = tmp.name
	try:
		with pytest.raises(ValueError):
			converter.convert_image_to_ascii(tmp_path)
	finally:
		os.remove(tmp_path)
