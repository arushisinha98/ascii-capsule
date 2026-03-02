/**
 * viewer.h — Scrollable text viewer
 *
 * Portable module (no NDS dependencies). Loads text files into a line
 * buffer and provides scrolling + row extraction for rendering.
 */

#ifndef VIEWER_H
#define VIEWER_H

#include <stdbool.h>

#define VIEWER_MAX_LINES    512
#define VIEWER_MAX_LINE_LEN 128
#define VIEWER_COLS         32
#define VIEWER_ROWS         24

typedef struct {
    char lines[VIEWER_MAX_LINES][VIEWER_MAX_LINE_LEN];
    int  num_lines;
    int  max_width;     /* longest line length */
    int  scroll_x;
    int  scroll_y;
} Viewer;

/**
 * Initialize/reset the viewer to empty state.
 */
void viewer_init(Viewer* v);

/**
 * Load a text file into the viewer buffer.
 * Resets scroll position to (0, 0).
 *
 * @param v        Viewer to populate
 * @param filepath Full path to the text file (e.g. "nitro:/art/PCKL57.txt")
 * @return true on success
 */
bool viewer_load(Viewer* v, const char* filepath);

/**
 * Scroll the viewer by (dx, dy). Values are clamped to valid range.
 */
void viewer_scroll(Viewer* v, int dx, int dy);

/**
 * Get the visible text for a given screen row.
 * Fills buf with exactly VIEWER_COLS characters (padded with spaces)
 * plus a null terminator.
 *
 * @param v          Viewer
 * @param screen_row Row on screen (0 to VIEWER_ROWS-1)
 * @param buf        Output buffer (must be at least VIEWER_COLS+1 bytes)
 * @param bufsize    Size of buf
 */
void viewer_get_row(const Viewer* v, int screen_row, char* buf, int bufsize);

/**
 * @return Maximum horizontal scroll position.
 */
int viewer_max_scroll_x(const Viewer* v);

/**
 * @return Maximum vertical scroll position.
 */
int viewer_max_scroll_y(const Viewer* v);

#endif /* VIEWER_H */
