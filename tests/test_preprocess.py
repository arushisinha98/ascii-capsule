import os
import tempfile
import numpy as np
import pytest
from PIL import Image
import cv2
from preprocess import (
    load_image, convert_to_grayscale, remove_noise, threshold_image, close_gaps,
    filter_primary_component, detect_edges, save_intermediates, preprocess_image
)

def create_test_image(path, color=(255, 255, 255)):
    img = Image.new('RGB', (32, 32), color)
    img.save(path)

def test_load_image_reads_file():
    with tempfile.TemporaryDirectory() as tmpdir:
        img_path = os.path.join(tmpdir, 'test.png')
        create_test_image(img_path, color=(123, 222, 111))
        arr = load_image(img_path)
        assert isinstance(arr, np.ndarray)
        assert arr.shape == (32, 32, 3)
        # Check BGR order
        assert (arr[0,0] == [111, 222, 123]).all()

def test_convert_to_grayscale():
    arr = np.zeros((10, 10, 3), dtype=np.uint8)
    arr[..., 0] = 100  # B
    arr[..., 1] = 150  # G
    arr[..., 2] = 200  # R
    gray = convert_to_grayscale(arr)
    assert gray.shape == (10, 10)
    assert gray.dtype == np.uint8
    # OpenCV formula: 0.299*R + 0.587*G + 0.114*B
    expected = int(0.299*200 + 0.587*150 + 0.114*100)
    assert np.all(gray == expected)

def test_remove_noise_blurs():
    arr = np.random.randint(0, 256, (20, 20), dtype=np.uint8)
    blurred = remove_noise(arr)
    assert blurred.shape == arr.shape
    assert blurred.dtype == arr.dtype
    # Should be less variance after blur
    assert blurred.var() < arr.var() or np.isclose(blurred.var(), arr.var())

def test_threshold_image_binary():
    arr = np.full((10, 10), 128, dtype=np.uint8)
    arr[0:5] = 255
    arr[5:] = 0
    out = threshold_image(arr)
    assert out.shape == arr.shape
    assert set(np.unique(out)).issubset({0, 255})

def test_threshold_image_invert():
    arr = np.full((10, 10), 128, dtype=np.uint8)
    arr[0:5] = 255
    arr[5:] = 0
    out = threshold_image(arr, invert=True)
    assert out.shape == arr.shape
    assert set(np.unique(out)).issubset({0, 255})

def test_close_gaps_connects():
    arr = np.zeros((10, 10), dtype=np.uint8)
    arr[5, 2:8] = 255
    arr[4, 5] = 255  # small gap
    closed = close_gaps(arr)
    assert closed.shape == arr.shape
    assert closed[5, 5] == 255

def test_filter_primary_component_keeps_largest():
    arr = np.zeros((10, 10), dtype=np.uint8)
    arr[1:4, 1:4] = 255  # small blob
    arr[6:10, 6:10] = 255  # large blob (4x4)
    filtered = filter_primary_component(arr, min_area_ratio=0.01)
    # Only the large blob should remain
    assert filtered[7, 7] == 255
    assert filtered[2, 2] == 0

def test_filter_primary_component_tiny():
    arr = np.zeros((10, 10), dtype=np.uint8)
    arr[1, 1] = 255  # tiny
    filtered = filter_primary_component(arr, min_area_ratio=0.5)
    # Should return original if nothing big enough
    assert np.array_equal(filtered, arr)

def test_detect_edges():
    arr = np.zeros((10, 10), dtype=np.uint8)
    arr[2:8, 2:8] = 255
    edges = detect_edges(arr)
    assert edges.shape == arr.shape
    assert edges.dtype == np.uint8
    # Should have some edges
    assert np.any(edges > 0)

def test_save_intermediates_creates_files():
    with tempfile.TemporaryDirectory() as tmpdir:
        arr = np.zeros((10, 10), dtype=np.uint8)
        save_intermediates(tmpdir, arr, arr, arr, arr, arr, arr)
        for name in ["gray.png", "blur.png", "threshold.png", "closing.png", "processed.png", "edges.png"]:
            assert os.path.exists(os.path.join(tmpdir, name))

def test_preprocess_image_pipeline():
    with tempfile.TemporaryDirectory() as tmpdir:
        img_path = os.path.join(tmpdir, 'test.png')
        create_test_image(img_path, color=(0, 0, 0))
        result = preprocess_image(img_path, output_dir=tmpdir, save_steps=True)
        assert "processed" in result and "edges" in result
        assert isinstance(result["processed"], np.ndarray)
        assert isinstance(result["edges"], np.ndarray)
        # Intermediates saved
        for name in ["gray.png", "blur.png", "threshold.png", "closing.png", "processed.png", "edges.png"]:
            assert os.path.exists(os.path.join(tmpdir, name))
