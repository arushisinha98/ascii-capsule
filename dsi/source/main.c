/**
 * main.c — ASCII Capsule for Nintendo DSi
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
 *   MESSAGE      → Scrollable memory text (unlocked)
 *   FREE_ENTRY   → Code entry for bonus codes (no art)
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
#define SAVE_PATH        "fat:/ascii_capsule_save.txt"

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
    STATE_MESSAGE,
    STATE_FREE_ENTRY,
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
static CapsuleKeyboard   kbd;
static int        menu_cursor;
static int        selected_item;
static int        matched_bonus;     /* index of last matched bonus  */
static bool       free_entry_mode;   /* true when in free-entry flow */

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

    case STATE_MESSAGE:
        if (free_entry_mode && matched_bonus >= 0) {
            snprintf(path, sizeof(path), "nitro:/messages/%s",
                     content.bonus[matched_bonus].message_file);
        } else {
            snprintf(path, sizeof(path), "nitro:/messages/%s",
                     content.items[selected_item].message_file);
        }
        viewer_load(&viewer, path);
        break;

    case STATE_FREE_ENTRY:
        free_entry_mode = true;
        keyboard_init(&kbd);
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
        iprintf("\x1b[10;5H   ASCII  CAPSULE");
        iprintf("\x1b[12;3H=====================");
        iprintf("\x1b[16;8H\nLoading...");
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
        iprintf("\x1b[19;0HControls:");
        iprintf("\x1b[20;0H  D-Pad : Scroll");
        iprintf("\x1b[21;0H  L / R : Page up/down");
        iprintf("\x1b[22;0HPress START to continue");
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
    iprintf("\x1b[0;2H=== ASCII Capsule ===\n\n");
    iprintf("  Pick an item. Find the code.\n");
    iprintf("  Type it in. Unlock something.\n\n");

    /* Total menu entries = regular items + 1 (BONUS) */
    int total = content.count + 1;

    for (int i = 0; i < total && i < 14; i++) {
        if (i == menu_cursor)
            iprintf("  > ");
        else
            iprintf("    ");

        if (i < content.count) {
            iprintf("%-16s", content.items[i].name);
            if (content.items[i].unlocked)
                iprintf(" [*]");
        } else {
            /* BONUS slot */
            iprintf("%-16s", "BONUS");
            /* Show [*] if any bonus is unlocked */
            int any = 0;
            for (int b = 0; b < content.bonus_count; b++)
                if (content.bonus[b].unlocked) any++;
            if (any > 0)
                iprintf(" [%d]", any);
        }
        iprintf("\n");
    }

    /* Bottom screen: controls */
    consoleSelect(&bot_con);
    consoleClear();
    
    iprintf("\x1b[0;2HControls:");
    iprintf("\x1b[2;0H  A  : View / enter");
    iprintf("\x1b[3;0H  X  : Read message");
    iprintf("\x1b[4;0H       (if unlocked)");

    if (menu_cursor >= 0 && menu_cursor < content.count) {
        iprintf("\x1b[9;0H  Item: %s", content.items[menu_cursor].name);
        if (content.items[menu_cursor].unlocked)
            iprintf("\x1b[10;0H  Status: UNLOCKED");
        else
            iprintf("\x1b[10;0H  Status: LOCKED");
    } else if (menu_cursor == content.count) {
        iprintf("\x1b[9;0H  Item: BONUS");
        int unlocked_bonus = 0;
        for (int b = 0; b < content.bonus_count; b++)
            if (content.bonus[b].unlocked) unlocked_bonus++;
        iprintf("\x1b[10;0H  Found: %d / %d",
                unlocked_bonus, content.bonus_count);
    }

    /* Unlock count */
    int unlocked = 0;
    for (int i = 0; i < content.count; i++)
        if (content.items[i].unlocked) unlocked++;
    int bonus_unlocked = 0;
    for (int b = 0; b < content.bonus_count; b++)
        if (content.bonus[b].unlocked) bonus_unlocked++;
    iprintf("\x1b[16;0H  Items: %d / %d", unlocked, content.count);
    iprintf("\x1b[17;0H  Bonus: %d / %d",
            bonus_unlocked, content.bonus_count);
}

static void tick_menu(u16 kdown) {
    bool need_draw = bot_dirty;
    int total = content.count + 1;  /* +1 for BONUS */

    if (kdown & KEY_UP) {
        menu_cursor--;
        if (menu_cursor < 0) menu_cursor = total - 1;
        need_draw = true;
    }
    if (kdown & KEY_DOWN) {
        menu_cursor++;
        if (menu_cursor >= total) menu_cursor = 0;
        need_draw = true;
    }

    if (need_draw) {
        draw_menu();
        bot_dirty = false;
    }

    if (kdown & KEY_A) {
        if (menu_cursor < content.count) {
            /* Regular item → view ASCII art */
            selected_item = menu_cursor;
            free_entry_mode = false;
            enter_state(STATE_CHALLENGE);
        } else {
            /* BONUS → go directly to code entry */
            enter_state(STATE_FREE_ENTRY);
        }
    }

    if (kdown & KEY_X) {
        if (menu_cursor < content.count &&
            content.items[menu_cursor].unlocked) {
            selected_item = menu_cursor;
            free_entry_mode = false;
            enter_state(STATE_MESSAGE);
        }
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
        iprintf("\x1b[0;1H\n\nItem: %s",
                content.items[selected_item].name);
        iprintf("\x1b[18;0HControls:");
        iprintf("\x1b[19;0H  D-Pad : Scroll");
        iprintf("\x1b[20;0H  L / R : Page up / down");
        iprintf("\x1b[21;0H      A : Enter code");
        iprintf("\x1b[22;0H      B : Back to menu");
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
    /* Keep showing art on top for reference (regular mode only) */
    if (top_dirty && !free_entry_mode) {
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
            enter_state(STATE_MESSAGE);
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
    if (top_dirty && !free_entry_mode) {
        draw_viewer(&viewer, &top_con);
        top_dirty = false;
    }

    if (bot_dirty) {
        consoleSelect(&bot_con);
        iprintf("\x1b[6;4HNo");
        iprintf("\x1b[15;2H  A : Try again");
        if (free_entry_mode)
            iprintf("\x1b[18;2H  B : Back to menu");
        else
            iprintf("\x1b[18;2H  B : Back");
        bot_dirty = false;
    }

    if (kdown & KEY_A) {
        if (free_entry_mode)
            enter_state(STATE_FREE_ENTRY);
        else
            enter_state(STATE_CODE_ENTRY);
    }
    if (kdown & KEY_B) {
        if (free_entry_mode)
            enter_state(STATE_MENU);
        else
            enter_state(STATE_CHALLENGE);
    }
}

/* ================================================================== */
/* State: MESSAGE                                                      */
/* ================================================================== */

static void tick_message(u16 kdown, u16 krepeat) {
    update_viewer_scroll(&viewer, kdown, krepeat);
    redraw_viewer_if_changed(&viewer, &top_con);

    if (bot_dirty) {
        consoleSelect(&bot_con);
        iprintf("\x1b[0;2H\n** Message Unlocked! **");
        if (free_entry_mode) {
            iprintf("\x1b[2;0H\nBonus code accepted!");
        } else {
            iprintf("\x1b[2;0H\nItem: %s",
                    content.items[selected_item].name);
        }
        iprintf("\x1b[19;0HControls:");
        iprintf("\x1b[20;0H  D-Pad : Scroll");
        iprintf("\x1b[21;0H  L / R : Page up / down");
        iprintf("\x1b[22;0H      B : Back to menu");
        bot_dirty = false;
    }

    if (kdown & KEY_B) {
        free_entry_mode = false;
        enter_state(STATE_MENU);
    }
}

/* ================================================================== */
/* State: BONUS                                                 */
/* ================================================================== */

static void tick_free_entry(u16 kdown) {
    if (top_dirty) {
        consoleSelect(&top_con);
        consoleClear();
        iprintf("\x1b[2;3H=== BONUS ===");
        iprintf("\x1b[5;0H  Enter a bonus code.");
        iprintf("\x1b[6;0H  There are no hints.");

        int unlocked_bonus = 0;
        for (int b = 0; b < content.bonus_count; b++)
            if (content.bonus[b].unlocked) unlocked_bonus++;
        iprintf("\x1b[13;0H  Found: %d / %d",
                unlocked_bonus, content.bonus_count);

        top_dirty = false;
    }

    touchPosition touch;
    touchRead(&touch);

    int result = keyboard_update(&kbd, kdown, &touch);
    keyboard_draw(&kbd, &bot_con);

    if (result == 1) {
        /* User pressed OK — check against all bonus codes */
        matched_bonus = content_check_bonus_code(&content,
                                                  keyboard_get_input(&kbd));
        if (matched_bonus >= 0) {
            content.bonus[matched_bonus].unlocked = true;
            content_save_progress(&content, SAVE_PATH);
            enter_state(STATE_MESSAGE);
        } else {
            enter_state(STATE_WRONG_CODE);
        }
    } else if (result == -1) {
        /* User pressed cancel */
        free_entry_mode = false;
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
        iprintf("\x1b[19;0HControls:");
        iprintf("\x1b[20;0H  D-Pad : Scroll");
        iprintf("\x1b[21;0H  L / R : Page up / down");
        iprintf("\x1b[22;0H      B : Back to menu");
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
    menu_cursor     = 0;
    selected_item   = 0;
    matched_bonus   = -1;
    free_entry_mode = false;
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
            case STATE_MESSAGE:      tick_message(kdown, krepeat);    break;
            case STATE_FREE_ENTRY:   tick_free_entry(kdown);          break;
            case STATE_INSTRUCTIONS: tick_instructions(kdown, krepeat); break;
        }
    }

    return 0;
}
