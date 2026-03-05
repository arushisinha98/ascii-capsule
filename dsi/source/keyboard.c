/**
 * keyboard.c — On-screen keyboard for code entry
 *
 * Renders an alphanumeric keyboard on the NDS bottom screen console.
 * Supports D-pad navigation + A to select, and touchscreen taps.
 */

#include "keyboard.h"
#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Keyboard layout                                                     */
/* ------------------------------------------------------------------ */

/* Character rows on the keyboard.  Each character is displayed as    */
/* " X " (3 console columns wide).  Row 4 is the special-key row.    */

static const char* kb_chars[] = {
    "1234567890",   /* row 0 */
    "QWERTYUIOP",   /* row 1 */
    "ASDFGHJKL",    /* row 2 */
    "ZXCVBNM",      /* row 3 */
};
#define KB_CHAR_ROWS  4
#define KB_TOTAL_ROWS 5   /* 4 char rows + 1 special row */

/* Console Y position where each keyboard row starts */
#define KB_ROW_Y_START 6
#define KB_ROW_SPACING 2

/* Special key indices (row 4) */
#define SPECIAL_SPC 0
#define SPECIAL_DEL 1
#define SPECIAL_OK  2
#define SPECIAL_COUNT 3

/* ------------------------------------------------------------------ */
/* Helpers                                                             */
/* ------------------------------------------------------------------ */

static int row_len(int row) {
    if (row < KB_CHAR_ROWS)
        return (int)strlen(kb_chars[row]);
    return SPECIAL_COUNT;
}

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */

void keyboard_init(CapsuleKeyboard* k) {
    memset(k->input, 0, sizeof(k->input));
    k->len     = 0;
    k->sel_row = 1;   /* start on QWERTY row */
    k->sel_col = 0;
}

static void kb_type_char(CapsuleKeyboard* k, char ch) {
    if (k->len < KB_MAX_INPUT) {
        k->input[k->len++] = ch;
        k->input[k->len]   = '\0';
    }
}

static void kb_delete_char(CapsuleKeyboard* k) {
    if (k->len > 0) {
        k->input[--k->len] = '\0';
    }
}

int keyboard_update(CapsuleKeyboard* k, u16 kdown, touchPosition* touch) {

    /* ----- D-pad navigation ----- */
    if (kdown & KEY_UP) {
        k->sel_row--;
        if (k->sel_row < 0) k->sel_row = KB_TOTAL_ROWS - 1;
    }
    if (kdown & KEY_DOWN) {
        k->sel_row++;
        if (k->sel_row >= KB_TOTAL_ROWS) k->sel_row = 0;
    }

    /* Clamp column to current row length */
    int rlen = row_len(k->sel_row);
    if (k->sel_col >= rlen) k->sel_col = rlen - 1;
    if (k->sel_col < 0)    k->sel_col = 0;

    if (kdown & KEY_LEFT) {
        k->sel_col--;
        if (k->sel_col < 0) k->sel_col = rlen - 1;
    }
    if (kdown & KEY_RIGHT) {
        k->sel_col++;
        if (k->sel_col >= rlen) k->sel_col = 0;
    }

    /* ----- A button: select key ----- */
    if (kdown & KEY_A) {
        if (k->sel_row < KB_CHAR_ROWS) {
            kb_type_char(k, kb_chars[k->sel_row][k->sel_col]);
        } else {
            switch (k->sel_col) {
                case SPECIAL_SPC: kb_type_char(k, ' ');  break;
                case SPECIAL_DEL: kb_delete_char(k);     break;
                case SPECIAL_OK:  return 1;  /* confirmed */
            }
        }
    }

    /* ----- START also confirms ----- */
    if (kdown & KEY_START) return 1;

    /* ----- B cancels ----- */
    if (kdown & KEY_B) return -1;

    /* ----- Touch input ----- */
    if (kdown & KEY_TOUCH && touch) {
        /* Map touch pixel coordinates to console cell */
        int tcol = touch->px / 8;   /* 8 pixels per console char */
        int trow = touch->py / 8;

        /* Determine which keyboard row was tapped */
        for (int r = 0; r < KB_TOTAL_ROWS; r++) {
            int con_y = KB_ROW_Y_START + r * KB_ROW_SPACING;
            /* Each row occupies 2 console rows of touch area */
            if (trow >= con_y && trow < con_y + 2) {
                if (r < KB_CHAR_ROWS) {
                    /* Character key: each key is 3 console columns " X " */
                    int key_col = (tcol - 1) / 3;
                    int max_col = (int)strlen(kb_chars[r]);
                    if (key_col >= 0 && key_col < max_col) {
                        kb_type_char(k, kb_chars[r][key_col]);
                        k->sel_row = r;
                        k->sel_col = key_col;
                    }
                } else {
                    /* Special key row */
                    if (tcol >= 1 && tcol < 7) {
                        kb_type_char(k, ' ');          /* SPC */
                    } else if (tcol >= 9 && tcol < 15) {
                        kb_delete_char(k);             /* DEL */
                    } else if (tcol >= 17 && tcol < 23) {
                        return 1;                      /* OK  */
                    }
                }
                break;
            }
        }
    }

    return 0;
}

void keyboard_draw(const CapsuleKeyboard* k, PrintConsole* con) {
    consoleSelect(con);
    consoleClear();

    /* Header */
    iprintf("\x1b[1;1HEnter the hidden code:");

    /* Input field */
    iprintf("\x1b[3;1H> %-*s", KB_MAX_INPUT, k->input);
    /* Cursor indicator */
    if (k->len < KB_MAX_INPUT) {
        iprintf("\x1b[3;%dH_", 3 + k->len);
    }

    /* Character rows */
    for (int r = 0; r < KB_CHAR_ROWS; r++) {
        int con_y = KB_ROW_Y_START + r * KB_ROW_SPACING;
        int len = (int)strlen(kb_chars[r]);
        iprintf("\x1b[%d;1H", con_y);

        for (int c = 0; c < len; c++) {
            if (r == k->sel_row && c == k->sel_col) {
                iprintf("[%c]", kb_chars[r][c]);
            } else {
                iprintf(" %c ", kb_chars[r][c]);
            }
        }
    }

    /* Special key row */
    int sp_y = KB_ROW_Y_START + KB_CHAR_ROWS * KB_ROW_SPACING;
    bool on_sp = (k->sel_row == KB_CHAR_ROWS);

    iprintf("\x1b[%d;1H", sp_y);
    if (on_sp && k->sel_col == SPECIAL_SPC) iprintf("[SPC]"); else iprintf(" SPC ");
    iprintf("  ");
    if (on_sp && k->sel_col == SPECIAL_DEL) iprintf("[DEL]"); else iprintf(" DEL ");
    iprintf("  ");
    if (on_sp && k->sel_col == SPECIAL_OK)  iprintf("[OK!]"); else iprintf(" OK! ");
}

const char* keyboard_get_input(const CapsuleKeyboard* k) {
    return k->input;
}
