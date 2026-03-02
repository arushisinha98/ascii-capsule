/**
 * viewer.c — Scrollable text viewer
 *
 * Portable: only uses standard C library (stdio, string).
 */

#include "viewer.h"
#include <stdio.h>
#include <string.h>

void viewer_init(Viewer* v) {
    v->num_lines  = 0;
    v->max_width  = 0;
    v->scroll_x   = 0;
    v->scroll_y   = 0;
}

bool viewer_load(Viewer* v, const char* filepath) {
    viewer_init(v);

    FILE* f = fopen(filepath, "r");
    if (!f) return false;

    while (v->num_lines < VIEWER_MAX_LINES) {
        if (!fgets(v->lines[v->num_lines], VIEWER_MAX_LINE_LEN, f))
            break;

        /* Strip trailing newline / carriage return */
        int len = (int)strlen(v->lines[v->num_lines]);
        while (len > 0 && (v->lines[v->num_lines][len - 1] == '\n' ||
                           v->lines[v->num_lines][len - 1] == '\r')) {
            v->lines[v->num_lines][--len] = '\0';
        }

        if (len > v->max_width)
            v->max_width = len;

        v->num_lines++;
    }

    fclose(f);
    return true;
}

int viewer_max_scroll_x(const Viewer* v) {
    int mx = v->max_width - VIEWER_COLS;
    return mx > 0 ? mx : 0;
}

int viewer_max_scroll_y(const Viewer* v) {
    int my = v->num_lines - VIEWER_ROWS;
    return my > 0 ? my : 0;
}

void viewer_scroll(Viewer* v, int dx, int dy) {
    v->scroll_x += dx;
    v->scroll_y += dy;

    /* Clamp */
    int mx = viewer_max_scroll_x(v);
    int my = viewer_max_scroll_y(v);

    if (v->scroll_x < 0)  v->scroll_x = 0;
    if (v->scroll_x > mx) v->scroll_x = mx;
    if (v->scroll_y < 0)  v->scroll_y = 0;
    if (v->scroll_y > my) v->scroll_y = my;
}

void viewer_get_row(const Viewer* v, int screen_row, char* buf, int bufsize) {
    int cols = VIEWER_COLS;
    if (cols >= bufsize) cols = bufsize - 1;

    /* Fill with spaces */
    memset(buf, ' ', cols);
    buf[cols] = '\0';

    int line_idx = v->scroll_y + screen_row;
    if (line_idx < 0 || line_idx >= v->num_lines)
        return;

    const char* line = v->lines[line_idx];
    int line_len = (int)strlen(line);

    for (int col = 0; col < cols; col++) {
        int char_idx = v->scroll_x + col;
        if (char_idx >= 0 && char_idx < line_len) {
            buf[col] = line[char_idx];
        }
    }
}
