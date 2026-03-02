/**
 * main.c — ASCII Memories for Nintendo DSi
 *
 * Entry point, state machine, and NDS-specific rendering.
 *
 * States:
 *   SPLASH       → Welcome art for 3 seconds
 *   WELCOME      → Scrollable welcome letter
 *   MENU         → Item list (D-pad navigate, A select)
 *   CHALLENGE    → Scrollable ASCII art with hidden code
 *   CODE_ENTRY   → On-screen keyboard for code input
 *   WRONG_CODE   → Error screen, retry or go back
 *   LETTER       → Scrollable memory text (unlocked)
 *   INSTRUCTIONS → How to extend the app
 */

#include <nds.h>
#include <fat.h>
#include <filesystem.h>
#include <stdio.h>
#include <string.h>

#include "content.h"
#include "viewer.h"
#include "keyboard.h"

/* ================================================================== */
/* Constants                                                           */
/* ================================================================== */

#define SPLASH_FRAMES    180      /* 3 seconds at 60 fps             */
#define SCROLL_DELAY     15       /* key-repeat delay (frames)       */
#define SCROLL_REPEAT    3        /* key-repeat interval (frames)    */
#define SAVE_PATH        "fat:/ascii_memories_save.txt"

/* ================================================================== */
/* Application states                                                  */
/* ================================================================== */

typedef enum {
    STATE_SPLASH,
    STATE_WELCOME,
    STATE_MENU,
    STATE_CHALLENGE,
    STATE_CODE_ENTRY,
    STATE_WRONG_CODE,
    STATE_LETTER,
    STATE_INSTRUCTIONS
} AppState;

/* ================================================================== */
/* Globals                                                             */
/* ================================================================== */

static PrintConsole top_con;
static PrintConsole bot_con;

static AppState   state;
static int        frame_count;
static Content    content;
static Viewer     viewer;
static Keyboard   kbd;
static int        menu_cursor;
static int        selected_item;

/* Tracks whether screens need redraw */
static bool       top_dirty;
static bool       bot_dirty;

/* Previous scroll position for change detection */
static int        prev_sx, prev_sy;

/* ================================================================== */
/* Drawing helpers                                                     */
/* ================================================================== */

/**
 * Draw the viewer content on the given console.
 * Renders 32x24 visible characters from the viewer buffer.
 */
static void draw_viewer(Viewer* v, PrintConsole* con) {
    consoleSelect(con);
    char buf[VIEWER_COLS + 1];

    for (int row = 0; row < VIEWER_ROWS; row++) {
        viewer_get_row(v, row, buf, sizeof(buf));
        iprintf("\x1b[%d;0H%s", row, buf);
    }
}

/**
 * Handle viewer scroll input using keysDownRepeat for smooth scrolling.
 */
static void update_viewer_scroll(Viewer* v, u16 kdown, u16 krepeat) {
    /* Page scroll on single press */
    if (kdown & KEY_L)  viewer_scroll(v, 0, -(VIEWER_ROWS - 2));
    if (kdown & KEY_R)  viewer_scroll(v, 0,  (VIEWER_ROWS - 2));

    /* D-pad scroll with auto-repeat */
    if (krepeat & KEY_UP)    viewer_scroll(v, 0, -1);
    if (krepeat & KEY_DOWN)  viewer_scroll(v, 0,  1);
    if (krepeat & KEY_LEFT)  viewer_scroll(v, -1, 0);
    if (krepeat & KEY_RIGHT) viewer_scroll(v,  1, 0);
}

/**
 * Check if viewer scroll position has changed; if so, redraw.
 */
static void redraw_viewer_if_changed(Viewer* v, PrintConsole* con) {
    if (v->scroll_x != prev_sx || v->scroll_y != prev_sy || top_dirty) {
        draw_viewer(v, con);
        prev_sx = v->scroll_x;
        prev_sy = v->scroll_y;
        top_dirty = false;
    }
}

/* ================================================================== */
/* State transitions                                                   */
/* ================================================================== */

static void enter_state(AppState new_state) {
    state = new_state;

    /* Clear both screens */
    consoleSelect(&top_con); consoleClear();
    consoleSelect(&bot_con); consoleClear();
    top_dirty = true;
    bot_dirty = true;
    prev_sx = -1;
    prev_sy = -1;

    char path[128];

    switch (new_state) {
    case STATE_SPLASH:
        viewer_load(&viewer, "nitro:/welcome_art.txt");
        frame_count = 0;
        break;

    case STATE_WELCOME:
        viewer_load(&viewer, "nitro:/welcome_letter.txt");
        break;

    case STATE_MENU:
        break;

    case STATE_CHALLENGE:
        snprintf(path, sizeof(path), "nitro:/art/%s",
                 content.items[selected_item].art_file);
        viewer_load(&viewer, path);
        break;

    case STATE_CODE_ENTRY:
        keyboard_init(&kbd);
        break;

    case STATE_WRONG_CODE:
        break;

    case STATE_LETTER:
        snprintf(path, sizeof(path), "nitro:/letters/%s",
                 content.items[selected_item].letter_file);
        viewer_load(&viewer, path);
        break;

    case STATE_INSTRUCTIONS:
        viewer_load(&viewer, "nitro:/instructions.txt");
        break;
    }
}

/* ================================================================== */
/* State: SPLASH                                                       */
/* ================================================================== */

static void tick_splash(u16 kdown) {
    if (top_dirty) {
        draw_viewer(&viewer, &top_con);
        top_dirty = false;
    }

    if (bot_dirty) {
        consoleSelect(&bot_con);
        iprintf("\x1b[8;3H======================");
        iprintf("\x1b[10;5HASCII  MEMORIES");
        iprintf("\x1b[12;3H======================");
        iprintf("\x1b[16;8HLoading...");
        iprintf("\x1b[20;4HPress A to skip");
        bot_dirty = false;
    }

    frame_count++;
    if (frame_count > SPLASH_FRAMES || (kdown & KEY_A)) {
        enter_state(STATE_WELCOME);
    }
}

/* ================================================================== */
/* State: WELCOME                                                      */
/* ================================================================== */

static void tick_welcome(u16 kdown, u16 krepeat) {
    update_viewer_scroll(&viewer, kdown, krepeat);
    redraw_viewer_if_changed(&viewer, &top_con);

    if (bot_dirty) {
        consoleSelect(&bot_con);
        iprintf("\x1b[0;3H=== Welcome! ===");
        iprintf("\x1b[2;0HRead the letter above.");
        iprintf("\x1b[4;0HControls:");
        iprintf("\x1b[5;0H  D-Pad : Scroll");
        iprintf("\x1b[6;0H  L / R : Page up/down");
        iprintf("\x1b[22;0HPRESS START to continue");
        bot_dirty = false;
    }

    if (kdown & KEY_START) {
        enter_state(STATE_MENU);
    }
}

/* ================================================================== */
/* State: MENU                                                         */
/* ================================================================== */

static void draw_menu(void) {
    /* Top screen: item list */
    consoleSelect(&top_con);
    consoleClear();
    iprintf("\x1b[0;2H=== ASCII Memories ===\n\n");
    iprintf("  Select an item to view\n");
    iprintf("  its ASCII art and find\n");
    iprintf("  the hidden code!\n\n");

    for (int i = 0; i < content.count && i < 14; i++) {
        if (i == menu_cursor)
            iprintf("  > ");
        else
            iprintf("    ");

        iprintf("%-16s", content.items[i].name);
        if (content.items[i].unlocked)
            iprintf(" [*]");
        iprintf("\n");
    }

    /* Bottom screen: controls */
    consoleSelect(&bot_con);
    consoleClear();
    iprintf("\x1b[0;2HControls:");
    iprintf("\x1b[2;0H  Up/Down : Navigate");
    iprintf("\x1b[3;0H  A       : View art");
    iprintf("\x1b[4;0H  X       : Read memory");
    iprintf("\x1b[5;0H            (if unlocked)");
    iprintf("\x1b[6;0H  SELECT  : Instructions");

    if (menu_cursor >= 0 && menu_cursor < content.count) {
        iprintf("\x1b[9;0H  Item: %s", content.items[menu_cursor].name);
        if (content.items[menu_cursor].unlocked)
            iprintf("\x1b[10;0H  Status: UNLOCKED");
        else
            iprintf("\x1b[10;0H  Status: LOCKED");
    }

    /* Unlock count */
    int unlocked = 0;
    for (int i = 0; i < content.count; i++)
        if (content.items[i].unlocked) unlocked++;
    iprintf("\x1b[13;0H  Progress: %d / %d", unlocked, content.count);
}

static void tick_menu(u16 kdown) {
    bool need_draw = bot_dirty;

    if (kdown & KEY_UP) {
        menu_cursor--;
        if (menu_cursor < 0) menu_cursor = content.count - 1;
        need_draw = true;
    }
    if (kdown & KEY_DOWN) {
        menu_cursor++;
        if (menu_cursor >= content.count) menu_cursor = 0;
        need_draw = true;
    }

    if (need_draw) {
        draw_menu();
        bot_dirty = false;
    }

    if (kdown & KEY_A) {
        selected_item = menu_cursor;
        enter_state(STATE_CHALLENGE);
    }

    if ((kdown & KEY_X) && content.items[menu_cursor].unlocked) {
        selected_item = menu_cursor;
        enter_state(STATE_LETTER);
    }

    if (kdown & KEY_SELECT) {
        enter_state(STATE_INSTRUCTIONS);
    }
}

/* ================================================================== */
/* State: CHALLENGE                                                    */
/* ================================================================== */

static void tick_challenge(u16 kdown, u16 krepeat) {
    update_viewer_scroll(&viewer, kdown, krepeat);
    redraw_viewer_if_changed(&viewer, &top_con);

    if (bot_dirty) {
        consoleSelect(&bot_con);
        iprintf("\x1b[0;1H- Find the Hidden Code -");
        iprintf("\x1b[2;0HScroll the ASCII art on");
        iprintf("\x1b[3;0Hthe top screen to find");
        iprintf("\x1b[4;0Htext hidden in the art.");
        iprintf("\x1b[6;0HControls:");
        iprintf("\x1b[7;0H  D-Pad : Scroll");
        iprintf("\x1b[8;0H  L / R : Page up/down");
        iprintf("\x1b[11;0HItem: %s",
                content.items[selected_item].name);
        iprintf("\x1b[21;0H  A : Enter code");
        iprintf("\x1b[22;0H  B : Back to menu");
        bot_dirty = false;
    }

    if (kdown & KEY_A) {
        enter_state(STATE_CODE_ENTRY);
    }
    if (kdown & KEY_B) {
        enter_state(STATE_MENU);
    }
}

/* ================================================================== */
/* State: CODE_ENTRY                                                   */
/* ================================================================== */

static void tick_code_entry(u16 kdown) {
    /* Keep showing art on top for reference */
    if (top_dirty) {
        draw_viewer(&viewer, &top_con);
        top_dirty = false;
    }

    touchPosition touch;
    touchRead(&touch);

    int result = keyboard_update(&kbd, kdown, &touch);
    keyboard_draw(&kbd, &bot_con);

    if (result == 1) {
        /* User pressed OK */
        if (content_check_code(&content.items[selected_item],
                               keyboard_get_input(&kbd))) {
            /* Correct! */
            content.items[selected_item].unlocked = true;
            content_save_progress(&content, SAVE_PATH);
            enter_state(STATE_LETTER);
        } else {
            enter_state(STATE_WRONG_CODE);
        }
    } else if (result == -1) {
        /* User pressed cancel */
        enter_state(STATE_CHALLENGE);
    }
}

/* ================================================================== */
/* State: WRONG_CODE                                                   */
/* ================================================================== */

static void tick_wrong_code(u16 kdown) {
    if (top_dirty) {
        draw_viewer(&viewer, &top_con);
        top_dirty = false;
    }

    if (bot_dirty) {
        consoleSelect(&bot_con);
        iprintf("\x1b[6;4H=== Wrong Code! ===");
        iprintf("\x1b[9;2HYou entered:");
        iprintf("\x1b[10;4H\"%s\"", keyboard_get_input(&kbd));
        iprintf("\x1b[12;2HThat's not it.");
        iprintf("\x1b[13;2HLook more carefully!");
        iprintf("\x1b[17;2H  A : Try again");
        iprintf("\x1b[18;2H  B : Back to art");
        bot_dirty = false;
    }

    if (kdown & KEY_A) {
        enter_state(STATE_CODE_ENTRY);
    }
    if (kdown & KEY_B) {
        enter_state(STATE_CHALLENGE);
    }
}

/* ================================================================== */
/* State: LETTER                                                       */
/* ================================================================== */

static void tick_letter(u16 kdown, u16 krepeat) {
    update_viewer_scroll(&viewer, kdown, krepeat);
    redraw_viewer_if_changed(&viewer, &top_con);

    if (bot_dirty) {
        consoleSelect(&bot_con);
        iprintf("\x1b[0;2H** Memory Unlocked! **");
        iprintf("\x1b[2;0HItem: %s",
                content.items[selected_item].name);
        iprintf("\x1b[4;0HRead the memory above.");
        iprintf("\x1b[6;0HControls:");
        iprintf("\x1b[7;0H  D-Pad : Scroll");
        iprintf("\x1b[8;0H  L / R : Page up/down");
        iprintf("\x1b[22;0H  B : Back to menu");
        bot_dirty = false;
    }

    if (kdown & KEY_B) {
        enter_state(STATE_MENU);
    }
}

/* ================================================================== */
/* State: INSTRUCTIONS                                                 */
/* ================================================================== */

static void tick_instructions(u16 kdown, u16 krepeat) {
    update_viewer_scroll(&viewer, kdown, krepeat);
    redraw_viewer_if_changed(&viewer, &top_con);

    if (bot_dirty) {
        consoleSelect(&bot_con);
        iprintf("\x1b[0;2H--- How to Extend ---");
        iprintf("\x1b[2;0HRead the instructions");
        iprintf("\x1b[3;0Habove to add your own");
        iprintf("\x1b[4;0HASCII art and memories!");
        iprintf("\x1b[6;0HControls:");
        iprintf("\x1b[7;0H  D-Pad : Scroll");
        iprintf("\x1b[8;0H  L / R : Page up/down");
        iprintf("\x1b[22;0H  B : Back to menu");
        bot_dirty = false;
    }

    if (kdown & KEY_B) {
        enter_state(STATE_MENU);
    }
}

/* ================================================================== */
/* Main                                                                */
/* ================================================================== */

int main(void) {

    /* --- Video setup --- */
    videoSetMode(MODE_0_2D);
    videoSetModeSub(MODE_0_2D);

    vramSetBankA(VRAM_A_MAIN_BG);
    vramSetBankC(VRAM_C_SUB_BG);

    /* Top screen = main engine, bottom = sub engine */
    consoleInit(&top_con, 3, BgType_Text4bpp, BgSize_T_256x256,
                31, 0, true, true);
    consoleInit(&bot_con, 3, BgType_Text4bpp, BgSize_T_256x256,
                31, 0, false, true);

    /* --- Key repeat for smooth scrolling --- */
    keysSetRepeat(SCROLL_DELAY, SCROLL_REPEAT);

    /* --- Filesystem init --- */
    bool fat_ok = fatInitDefault();      /* SD card (for save files) */

    if (!nitroFSInit(NULL)) {
        consoleSelect(&bot_con);
        iprintf("NitroFS init failed!\n\n");
        iprintf("Make sure the .nds was\n");
        iprintf("built with NitroFS\n");
        iprintf("support (nitrofiles/).\n");
        while (1) swiWaitForVBlank();
    }

    /* --- Load content manifest --- */
    if (!content_load(&content, "nitro:")) {
        consoleSelect(&bot_con);
        iprintf("Failed to load\n");
        iprintf("manifest.txt!\n\n");
        iprintf("Check nitrofiles/\n");
        while (1) swiWaitForVBlank();
    }

    /* --- Load saved progress (if FAT available) --- */
    if (fat_ok) {
        content_load_progress(&content, SAVE_PATH);
    }

    /* --- Initial state --- */
    viewer_init(&viewer);
    menu_cursor   = 0;
    selected_item = 0;
    enter_state(STATE_SPLASH);

    /* ============================================================== */
    /* Main loop                                                       */
    /* ============================================================== */
    while (1) {
        swiWaitForVBlank();
        scanKeys();

        u16 kdown   = keysDown();
        u16 krepeat = keysDownRepeat();

        switch (state) {
            case STATE_SPLASH:       tick_splash(kdown);              break;
            case STATE_WELCOME:      tick_welcome(kdown, krepeat);    break;
            case STATE_MENU:         tick_menu(kdown);                break;
            case STATE_CHALLENGE:    tick_challenge(kdown, krepeat);  break;
            case STATE_CODE_ENTRY:   tick_code_entry(kdown);          break;
            case STATE_WRONG_CODE:   tick_wrong_code(kdown);          break;
            case STATE_LETTER:       tick_letter(kdown, krepeat);     break;
            case STATE_INSTRUCTIONS: tick_instructions(kdown, krepeat); break;
        }
    }

    return 0;
}
