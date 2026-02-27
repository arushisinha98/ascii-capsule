"""
Text Concealer Module

Provides functionality to hide text messages within ASCII art images.
The concealed text is placed at positions that match the requested
difficulty level, making it easier or harder to spot.
"""

import random
from typing import List, Optional, Tuple


# Visual density of common ASCII art characters (0.0 = lightest, 1.0 = darkest)
_DENSITY_MAP = {
    ' ': 0.0, '.': 0.1, ',': 0.1, ':': 0.2, ';': 0.2,
    '-': 0.3, '~': 0.3, '=': 0.4, '!': 0.4, '+': 0.5,
    '*': 0.6, '#': 0.7, '%': 0.8, '$': 0.85, '@': 0.9,
}


def _char_density(char: str) -> float:
    """Return the visual density of a character (0.0 = lightest, 1.0 = darkest)."""
    if char in _DENSITY_MAP:
        return _DENSITY_MAP[char]
    if char.isalnum():
        return 0.5
    return 0.4


def _segment_density(line: str, col: int, length: int) -> float:
    """Return average visual density of a segment of a line."""
    segment = line[col:col + length]
    if not segment:
        return 0.5
    return sum(_char_density(c) for c in segment) / len(segment)


def _score_position(line: str, col: int, word_len: int, difficulty: str) -> float:
    """
    Score a column position for placing a word based on difficulty.

    Easy:   prefers lightest areas (text is most visible / easiest to find).
    Medium: prefers mid-density areas.
    Hard:   prefers darkest areas (text blends in / hardest to find).
    """
    density = _segment_density(line, col, word_len)

    if difficulty == "easy":
        return 1.0 - density
    elif difficulty == "hard":
        return density
    else:  # medium
        return 1.0 - abs(density - 0.5) * 2


def _find_best_column(
    line: str,
    word_len: int,
    difficulty: str,
    min_col: int,
    max_col: int,
) -> Tuple[int, float]:
    """
    Find the best column on a line to place a word.

    Among tied positions, one is chosen at random so placement
    is less predictable.

    Returns:
        Tuple of (best_column, best_score).
    """
    best_score = -float('inf')
    candidates: List[int] = []

    for col in range(min_col, max_col + 1):
        score = _score_position(line, col, word_len, difficulty)
        if score > best_score:
            best_score = score
            candidates = [col]
        elif score == best_score:
            candidates.append(col)

    best_col = random.choice(candidates) if candidates else min_col
    return best_col, best_score


def conceal(ascii_art: str, text: str, difficulty: str = "hard") -> str:
    """
    Conceal text within ASCII art.

    Words are split by whitespace and placed on successive lines of the
    ASCII art. Placement is guided by the difficulty level which controls
    whether text lands in light areas (easy to find) or dark areas
    (hard to find).

    Consecutive words are constrained so readers can follow the message
    naturally: for words t and the previous word t-1, the end column of t
    must be >= start column of t-1 minus 5, and the start column of t
    must be <= end column of t-1 plus 5.

    Args:
        ascii_art: ASCII art string with newline-separated rows.
        text: Message to conceal.  Whitespace splits text into words
              that are each placed on a separate line.
        difficulty: One of ``"easy"``, ``"medium"``, or ``"hard"``.
            - ``"easy"``:   text placed in lightest areas (most visible).
            - ``"medium"``: text placed in mid-density areas.
            - ``"hard"``:   text placed in darkest areas (least visible).

    Returns:
        Modified ASCII art with the text concealed inside it.

    Raises:
        ValueError: If difficulty is invalid, text has more words than
                    available lines, or no valid placement exists.
    """
    if difficulty not in ("easy", "medium", "hard"):
        raise ValueError("difficulty must be 'easy', 'medium', or 'hard'")

    words = text.split()
    if not words:
        return ascii_art

    lines = ascii_art.split('\n')
    num_lines = len(lines)
    num_words = len(words)

    if num_words > num_lines:
        raise ValueError(
            f"Text has {num_words} words but ASCII art only has {num_lines} lines"
        )

    # ------------------------------------------------------------------
    # Try every possible starting row and greedily assign each word to
    # the best column on its row.  Keep the start row with the highest
    # total score.
    # ------------------------------------------------------------------
    best_total_score = -float('inf')
    best_start_row: Optional[int] = None
    best_placements: List[Tuple[int, int]] = []

    for start_row in range(num_lines - num_words + 1):
        placements: List[Tuple[int, int]] = []
        total_score = 0.0
        valid = True

        for i, word in enumerate(words):
            row = start_row + i
            line = lines[row]
            word_len = len(word)

            if word_len > len(line):
                valid = False
                break

            # Column bounds (full line width)
            min_col = 0
            max_col = len(line) - word_len

            # Apply ±5 constraint relative to previous word
            if i > 0:
                prev_start, prev_end = placements[i - 1]
                # end_col(t) >= start_col(t-1) - 5
                #   => col + word_len - 1 >= prev_start - 5
                #   => col >= prev_start - word_len - 4
                min_col = max(min_col, prev_start - word_len - 4)
                # start_col(t) <= end_col(t-1) + 5
                max_col = min(max_col, prev_end + 5)

            if min_col > max_col:
                valid = False
                break

            col, score = _find_best_column(line, word_len, difficulty, min_col, max_col)
            placements.append((col, col + word_len - 1))
            total_score += score

        if valid and total_score > best_total_score:
            best_total_score = total_score
            best_start_row = start_row
            best_placements = list(placements)

    if best_start_row is None:
        raise ValueError("Could not find valid placement for text in ASCII art")

    # ------------------------------------------------------------------
    # Overwrite the chosen positions with the concealed words.
    # ------------------------------------------------------------------
    for i, word in enumerate(words):
        row = best_start_row + i
        start_col = best_placements[i][0]
        line = lines[row]
        lines[row] = line[:start_col] + word + line[start_col + len(word):]

    return '\n'.join(lines)