/**
 * keyboard.h — On-screen keyboard for code entry
 *
 * NDS-specific module. Provides a D-pad navigable + touch-tappable
 * alphanumeric keyboard displayed on the bottom screen console.
 */

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <nds.h>
#include <stdbool.h>

#define KB_MAX_INPUT 20

typedef struct {
    char input[KB_MAX_INPUT + 1];   /* current text buffer    */
    int  len;                       /* current input length   */
    int  sel_row;                   /* selected key row       */
    int  sel_col;                   /* selected key column    */
} Keyboard;

/**
 * Initialize/reset the keyboard state.
 */
void keyboard_init(Keyboard* k);

/**
 * Process one frame of keyboard input.
 *
 * @param k     Keyboard state
 * @param kdown Keys pressed this frame (from keysDown())
 * @param touch Current touch position
 * @return  1 = user confirmed (pressed OK / START)
 *         -1 = user cancelled (pressed B)
 *          0 = still editing
 */
int keyboard_update(Keyboard* k, u16 kdown, touchPosition* touch);

/**
 * Draw the keyboard UI on the given console.
 * Overwrites the full bottom screen.
 */
void keyboard_draw(const Keyboard* k, PrintConsole* con);

/**
 * Get the current input string.
 */
const char* keyboard_get_input(const Keyboard* k);

#endif /* KEYBOARD_H */
