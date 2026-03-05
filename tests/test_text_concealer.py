import pytest
from text_concealer import conceal

ASCII_ART = (
    "      .-.-.     "
    "\n   (     )    "
    "\n  (       )   "
    "\n   `-._.-'    "
)

def test_conceal_easy_visible():
    # Easy: text should be placed in lightest areas
    result = conceal(ASCII_ART, "HI THERE", difficulty="easy")
    assert "HI" in result
    assert "THERE" in result
    # Should not raise

def test_conceal_medium_mid_density():
    # Medium: text should be placed in mid-density areas
    result = conceal(ASCII_ART, "MID TEST", difficulty="medium")
    assert "MID" in result
    assert "TEST" in result

def test_conceal_hard_darkest():
    # Hard: text should be placed in darkest areas
    result = conceal(ASCII_ART, "DARK MODE", difficulty="hard")
    assert "DARK" in result
    assert "MODE" in result

def test_conceal_too_many_words_raises():
    art = "line1\nline2"
    with pytest.raises(ValueError) as e:
        conceal(art, "one two three", difficulty="easy")
    assert "only has" in str(e.value)

def test_conceal_invalid_difficulty_raises():
    with pytest.raises(ValueError):
        conceal(ASCII_ART, "oops", difficulty="impossible")

def test_conceal_empty_text_returns_art():
    art = "foo\nbar"
    result = conceal(art, "", difficulty="easy")
    assert result == art

def test_conceal_word_too_long_for_line():
    art = "short\nline"
    with pytest.raises(ValueError):
        conceal(art, "toolongword", difficulty="easy")

def test_conceal_no_valid_placement():
    # Create a situation where the ±5 constraint makes placement impossible
    art = "abcde\nabcde"
    # Both words are 5 chars, but constraint will fail
    with pytest.raises(ValueError):
        conceal(art, "first second", difficulty="easy")

def test_conceal_preserves_other_lines():
    art = "12345\n67890\nabcde"
    result = conceal(art, "hi there", difficulty="easy")
    # Only two lines should be changed
    lines = result.split("\n")
    assert lines[2] == "abcde"

def test_conceal_randomness_still_valid():
    # Run multiple times to check for randomness but always valid output
    art = ".....\n.....\n....."
    for _ in range(10):
        result = conceal(art, "a b c", difficulty="medium")
        assert result.count("a") == 1
        assert result.count("b") == 1
        assert result.count("c") == 1
