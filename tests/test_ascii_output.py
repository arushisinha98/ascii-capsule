import os
import tempfile
import shutil
import pytest
from ascii_output import ASCIIOutputHandler

ASCII_ART = """
  __  
 /  \ 
| () |
 \__/ 
"""

def test_save_as_text_creates_file_and_content():
    with tempfile.TemporaryDirectory() as tmpdir:
        filepath = os.path.join(tmpdir, "art.txt")
        ASCIIOutputHandler.save_as_text(ASCII_ART, filepath)
        assert os.path.exists(filepath)
        with open(filepath, 'r', encoding='utf-8') as f:
            content = f.read()
        assert content == ASCII_ART

def test_save_as_text_creates_directory():
    with tempfile.TemporaryDirectory() as tmpdir:
        subdir = os.path.join(tmpdir, "subdir")
        filepath = os.path.join(subdir, "art.txt")
        ASCIIOutputHandler.save_as_text(ASCII_ART, filepath)
        assert os.path.exists(filepath)
        with open(filepath, 'r', encoding='utf-8') as f:
            content = f.read()
        assert content == ASCII_ART

def test_save_as_text_ioerror(monkeypatch):
    def raise_ioerror(*args, **kwargs):
        raise IOError("fail")
    monkeypatch.setattr("builtins.open", raise_ioerror)
    with pytest.raises(IOError, match="Error saving text file: fail"):
        ASCIIOutputHandler.save_as_text(ASCII_ART, "/tmp/fake.txt")

def test_save_as_html_creates_file_and_content():
    with tempfile.TemporaryDirectory() as tmpdir:
        filepath = os.path.join(tmpdir, "art.html")
        ASCIIOutputHandler.save_as_html(ASCII_ART, filepath, title="Test Art")
        assert os.path.exists(filepath)
        with open(filepath, 'r', encoding='utf-8') as f:
            html = f.read()
        assert "<html" in html and "ASCII Art" not in html
        assert "Test Art" in html
        assert "<pre>" in html
        assert "__" in html  # ASCII art present

def test_save_as_html_creates_directory():
    with tempfile.TemporaryDirectory() as tmpdir:
        subdir = os.path.join(tmpdir, "subdir")
        filepath = os.path.join(subdir, "art.html")
        ASCIIOutputHandler.save_as_html(ASCII_ART, filepath)
        assert os.path.exists(filepath)
        with open(filepath, 'r', encoding='utf-8') as f:
            html = f.read()
        assert "<pre>" in html
        assert "__" in html

def test_save_as_html_ioerror(monkeypatch):
    def raise_ioerror(*args, **kwargs):
        raise IOError("fail")
    monkeypatch.setattr("builtins.open", raise_ioerror)
    with pytest.raises(IOError, match="Error saving HTML file: fail"):
        ASCIIOutputHandler.save_as_html(ASCII_ART, "/tmp/fake.html")

def test_get_output_path_with_output_dir():
    path = ASCIIOutputHandler.get_output_path("/foo/bar/image.png", "/tmp/out", "txt")
    assert path == os.path.join("/tmp/out", "image.txt")

def test_get_output_path_without_output_dir():
    path = ASCIIOutputHandler.get_output_path("/foo/bar/image.png", None, "html")
    assert path == os.path.join("/foo/bar", "image.html")

def test_get_output_path_with_relative_input():
    path = ASCIIOutputHandler.get_output_path("image.png", "output", "txt")
    assert path == os.path.join("output", "image.txt")

def test_get_output_path_with_no_extension():
    path = ASCIIOutputHandler.get_output_path("/foo/bar/image", None, "txt")
    assert path == os.path.join("/foo/bar", "image.txt")
