/**
 * test_content.c — Host-side tests for content and viewer modules
 *
 * Compiles on the development machine (macOS/Linux) without NDS deps.
 * Tests content manifest parsing, hash-based code checking, bonus codes,
 * viewer scrolling, and edge cases.
 *
 * Build:  cc -Wall -Wextra -std=c11 -I../source -o test_content test_content.c ../source/content.c ../source/viewer.c
 * Run:    ./test_content
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>

#include "content.h"
#include "viewer.h"

/* ------------------------------------------------------------------ */
/* Test helpers                                                        */
/* ------------------------------------------------------------------ */

static int tests_run    = 0;
static int tests_passed = 0;

#define TEST(name) \
    do { \
        tests_run++; \
        printf("  TEST: %-48s ", name); \
    } while (0)

#define PASS() \
    do { \
        tests_passed++; \
        printf("[PASS]\n"); \
    } while (0)

#define FAIL(msg) \
    do { \
        printf("[FAIL] %s\n", msg); \
    } while (0)

#define ASSERT_TRUE(cond, msg) \
    do { \
        if (!(cond)) { FAIL(msg); return; } \
    } while (0)

#define ASSERT_EQ_INT(a, b, msg) \
    do { \
        if ((a) != (b)) { \
            printf("[FAIL] %s (got %d, expected %d)\n", msg, (a), (b)); \
            return; \
        } \
    } while (0)

#define ASSERT_EQ_UINT(a, b, msg) \
    do { \
        if ((a) != (b)) { \
            printf("[FAIL] %s (got %u, expected %u)\n", msg, (unsigned)(a), (unsigned)(b)); \
            return; \
        } \
    } while (0)

#define ASSERT_EQ_STR(a, b, msg) \
    do { \
        if (strcmp((a), (b)) != 0) { \
            printf("[FAIL] %s (got \"%s\", expected \"%s\")\n", msg, (a), (b)); \
            return; \
        } \
    } while (0)

/* ------------------------------------------------------------------ */
/* Setup: create test data directory and files                         */
/* ------------------------------------------------------------------ */

static void create_dir(const char* path) {
    mkdir(path, 0755);
}

static void write_file(const char* path, const char* data) {
    FILE* f = fopen(path, "w");
    if (f) {
        fputs(data, f);
        fclose(f);
    }
}

/*
 * Pre-computed hashes (djb2-xor, uppercase):
 *   "PCKL 57"        → must compute at test time via content_code_hash
 *   "TNT 404"        → same
 *   "C0ZY"           → same
 *   "BONUS1"         → same
 *   "BONUS2"         → same
 */

static char manifest_buf[1024];

static void setup_test_data(void) {
    create_dir("testdata");
    create_dir("testdata/art");
    create_dir("testdata/messages");

    /* Build manifest with real hashes computed at runtime */
    uint32_t h_pckl  = content_code_hash("PCKL 57");
    uint32_t h_tnt   = content_code_hash("TNT 404");
    uint32_t h_c0zy  = content_code_hash("C0ZY");
    uint32_t h_bon1  = content_code_hash("BONUS1");
    uint32_t h_bon2  = content_code_hash("BONUS2");

    snprintf(manifest_buf, sizeof(manifest_buf),
        "# Test manifest\n"
        "Pickle|%u|PCKL57.txt|PCKL57_msg.txt\n"
        "Tent|%u|TNT404.txt|TNT404_msg.txt\n"
        "Cozy|%u|C0ZY.txt|C0ZY_msg.txt\n"
        "\n"
        "# Bonus codes\n"
        "+%u|bonus1_msg.txt\n"
        "+%u|bonus2_msg.txt\n",
        h_pckl, h_tnt, h_c0zy, h_bon1, h_bon2);

    write_file("testdata/manifest.txt", manifest_buf);

    /* A small art file (lines must exceed 32 chars for scroll tests) */
    write_file("testdata/art/PCKL57.txt",
        "Line_1:_Hello_world!!_padded_out_to_exceed_32_characters_wide\n"
        "Line_2:_ABCDEFGHIJKLMNOPQRSTUVWXYZ_1234567890_extra_extras!!\n"
        "Line_3:_1234567890123456789012345678901234567890_long_line!!!\n"
        "Line 4: Short\n"
        "Line 5: End\n"
    );

    /* A save file */
    write_file("testdata/save.txt", "Pickle\n");

    /* Empty manifest for edge case test */
    write_file("testdata/empty_manifest.txt", "# Only comments\n\n");

    /* Malformed manifest */
    write_file("testdata/bad_manifest.txt",
        "OnlyOnePipe|123\n"
        "NoPipes\n"
        "||||\n"
        "\n"
    );

    /* Oversized manifest (more than MAX_ITEMS regular items) */
    {
        FILE* f = fopen("testdata/overflow_manifest.txt", "w");
        if (f) {
            for (int i = 0; i < MAX_ITEMS + 5; i++) {
                fprintf(f, "Item%d|%u|art%d.txt|msg%d.txt\n",
                        i, content_code_hash("X"), i, i);
            }
            /* Also add bonus items past MAX_BONUS */
            for (int i = 0; i < MAX_BONUS + 5; i++) {
                fprintf(f, "+%u|bonus%d.txt\n",
                        content_code_hash("Y"), i);
            }
            fclose(f);
        }
    }

    /* A file with very long lines for viewer */
    {
        FILE* f = fopen("testdata/art/wide.txt", "w");
        if (f) {
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 120; j++) fputc('A' + (j % 26), f);
                fputc('\n', f);
            }
            fclose(f);
        }
    }

    /* A file with many lines for viewer */
    {
        FILE* f = fopen("testdata/art/tall.txt", "w");
        if (f) {
            for (int i = 0; i < 100; i++) {
                fprintf(f, "Line %03d: content here\n", i);
            }
            fclose(f);
        }
    }

    /* An empty file */
    write_file("testdata/art/empty.txt", "");

    /* A single-character file */
    write_file("testdata/art/tiny.txt", "X\n");
}

static void cleanup_test_data(void) {
    remove("testdata/save.txt");
    remove("testdata/save_test.txt");
    remove("testdata/save_bonus.txt");
    remove("testdata/art/PCKL57.txt");
    remove("testdata/art/wide.txt");
    remove("testdata/art/tall.txt");
    remove("testdata/art/empty.txt");
    remove("testdata/art/tiny.txt");
    remove("testdata/messages");
    remove("testdata/art");
    remove("testdata/manifest.txt");
    remove("testdata/empty_manifest.txt");
    remove("testdata/bad_manifest.txt");
    remove("testdata/overflow_manifest.txt");
    remove("testdata");
}

/* ================================================================== */
/* Hash function tests                                                 */
/* ================================================================== */

static void test_hash_deterministic(void) {
    TEST("content_code_hash is deterministic");
    uint32_t h1 = content_code_hash("HELLO WORLD");
    uint32_t h2 = content_code_hash("HELLO WORLD");
    ASSERT_EQ_UINT(h1, h2, "same input → same hash");
    PASS();
}

static void test_hash_case_insensitive(void) {
    TEST("content_code_hash is case-insensitive");
    uint32_t h1 = content_code_hash("Hello World");
    uint32_t h2 = content_code_hash("HELLO WORLD");
    uint32_t h3 = content_code_hash("hello world");
    ASSERT_EQ_UINT(h1, h2, "mixed vs upper");
    ASSERT_EQ_UINT(h2, h3, "upper vs lower");
    PASS();
}

static void test_hash_different_inputs(void) {
    TEST("content_code_hash differs for different codes");
    uint32_t h1 = content_code_hash("PCKL 57");
    uint32_t h2 = content_code_hash("TNT 404");
    uint32_t h3 = content_code_hash("C0ZY");
    ASSERT_TRUE(h1 != h2, "PCKL 57 != TNT 404");
    ASSERT_TRUE(h2 != h3, "TNT 404 != C0ZY");
    ASSERT_TRUE(h1 != h3, "PCKL 57 != C0ZY");
    PASS();
}

static void test_hash_empty_string(void) {
    TEST("content_code_hash of empty string");
    uint32_t h = content_code_hash("");
    ASSERT_EQ_UINT(h, 5381, "empty string → seed value");
    PASS();
}

static void test_hash_spaces_matter(void) {
    TEST("content_code_hash: spaces are significant");
    uint32_t h1 = content_code_hash("AB CD");
    uint32_t h2 = content_code_hash("ABCD");
    uint32_t h3 = content_code_hash("A BCD");
    ASSERT_TRUE(h1 != h2, "with space vs without");
    ASSERT_TRUE(h1 != h3, "space in different position");
    PASS();
}

static void test_hash_known_values(void) {
    TEST("content_code_hash matches known values");
    /* Verify against the values we computed with Python */
    ASSERT_EQ_UINT(content_code_hash("JO SOURDOUGH"),    4236073098u, "JO SOURDOUGH");
    ASSERT_EQ_UINT(content_code_hash("CHEF HTN"),        2829293631u, "CHEF HTN");
    ASSERT_EQ_UINT(content_code_hash("TENT 404"),         941826462u, "TENT 404");
    ASSERT_EQ_UINT(content_code_hash("2SHOO"),            114642636u, "2SHOO");
    PASS();
}

/* ================================================================== */
/* Content loading tests                                               */
/* ================================================================== */

static void test_content_load(void) {
    TEST("content_load parses manifest");
    Content c;
    bool ok = content_load(&c, "testdata");
    ASSERT_TRUE(ok, "content_load should succeed");
    ASSERT_EQ_INT(c.count, 3, "should load 3 regular items");
    ASSERT_EQ_STR(c.items[0].name, "Pickle", "first item name");
    ASSERT_EQ_UINT(c.items[0].code_hash, content_code_hash("PCKL 57"), "first item hash");
    ASSERT_EQ_STR(c.items[0].art_file, "PCKL57.txt", "first item art");
    ASSERT_EQ_STR(c.items[0].message_file, "PCKL57_msg.txt", "first item message");
    ASSERT_EQ_STR(c.items[1].name, "Tent", "second item name");
    ASSERT_EQ_STR(c.items[2].name, "Cozy", "third item name");
    ASSERT_TRUE(!c.items[0].unlocked, "items start locked");
    ASSERT_TRUE(!c.items[1].unlocked, "items start locked");
    PASS();
}

static void test_content_load_bonus(void) {
    TEST("content_load parses bonus items");
    Content c;
    content_load(&c, "testdata");
    ASSERT_EQ_INT(c.bonus_count, 2, "should load 2 bonus items");
    ASSERT_EQ_UINT(c.bonus[0].code_hash, content_code_hash("BONUS1"), "bonus 0 hash");
    ASSERT_EQ_STR(c.bonus[0].message_file, "bonus1_msg.txt", "bonus 0 file");
    ASSERT_EQ_STR(c.bonus[1].message_file, "bonus2_msg.txt", "bonus 1 file");
    ASSERT_TRUE(!c.bonus[0].unlocked, "bonus starts locked");
    PASS();
}

static void test_content_load_missing(void) {
    TEST("content_load fails on missing dir");
    Content c;
    bool ok = content_load(&c, "nonexistent");
    ASSERT_TRUE(!ok, "should fail for missing dir");
    PASS();
}

static void test_content_load_empty_manifest(void) {
    TEST("content_load fails on empty/comment-only manifest");
    /* Create a temp dir with the empty manifest */
    create_dir("testdata_empty");
    write_file("testdata_empty/manifest.txt", "# Only comments\n\n");
    Content c;
    bool ok = content_load(&c, "testdata_empty");
    ASSERT_TRUE(!ok, "should fail with no items");
    ASSERT_EQ_INT(c.count, 0, "count should be 0");
    remove("testdata_empty/manifest.txt");
    remove("testdata_empty");
    PASS();
}

static void test_content_load_malformed_lines(void) {
    TEST("content_load skips malformed lines");
    create_dir("testdata_bad");
    uint32_t h = content_code_hash("X");
    char buf[512];
    snprintf(buf, sizeof(buf),
        "OnlyOnePipe|123\n"
        "NoPipes\n"
        "||||\n"
        "Valid|%u|art.txt|msg.txt\n"
        "\n", h);
    write_file("testdata_bad/manifest.txt", buf);
    Content c;
    bool ok = content_load(&c, "testdata_bad");
    ASSERT_TRUE(ok, "should succeed with 1 valid item");
    ASSERT_EQ_INT(c.count, 1, "only valid lines parsed");
    ASSERT_EQ_STR(c.items[0].name, "Valid", "valid item name");
    remove("testdata_bad/manifest.txt");
    remove("testdata_bad");
    PASS();
}

static void test_content_load_overflow(void) {
    TEST("content_load respects MAX_ITEMS/MAX_BONUS");
    /* Build an overflow manifest directly */
    create_dir("testdata_overflow");
    {
        FILE* f = fopen("testdata_overflow/manifest.txt", "w");
        uint32_t h = content_code_hash("X");
        for (int i = 0; i < MAX_ITEMS + 5; i++)
            fprintf(f, "Item%d|%u|art%d.txt|msg%d.txt\n", i, h, i, i);
        uint32_t hb = content_code_hash("Y");
        for (int i = 0; i < MAX_BONUS + 5; i++)
            fprintf(f, "+%u|bonus%d.txt\n", hb, i);
        fclose(f);
    }
    Content c;
    content_load(&c, "testdata_overflow");
    ASSERT_EQ_INT(c.count, MAX_ITEMS, "regular capped at MAX_ITEMS");
    ASSERT_EQ_INT(c.bonus_count, MAX_BONUS, "bonus capped at MAX_BONUS");
    remove("testdata_overflow/manifest.txt");
    remove("testdata_overflow");
    PASS();
}

/* ================================================================== */
/* Code checking tests                                                 */
/* ================================================================== */

static void test_content_check_code_exact(void) {
    TEST("content_check_code exact match");
    ContentItem item;
    item.code_hash = content_code_hash("PCKL 57");
    ASSERT_TRUE(content_check_code(&item, "PCKL 57"), "exact match");
    PASS();
}

static void test_content_check_code_case_insensitive(void) {
    TEST("content_check_code case insensitive");
    ContentItem item;
    item.code_hash = content_code_hash("PCKL 57");
    ASSERT_TRUE(content_check_code(&item, "pckl 57"), "lower case");
    ASSERT_TRUE(content_check_code(&item, "Pckl 57"), "mixed case");
    PASS();
}

static void test_content_check_code_wrong(void) {
    TEST("content_check_code rejects wrong code");
    ContentItem item;
    item.code_hash = content_code_hash("PCKL 57");
    ASSERT_TRUE(!content_check_code(&item, "WRONG"), "wrong code");
    ASSERT_TRUE(!content_check_code(&item, "PCKL57"), "missing space");
    ASSERT_TRUE(!content_check_code(&item, ""), "empty input");
    ASSERT_TRUE(!content_check_code(&item, "PCKL 5"), "truncated");
    ASSERT_TRUE(!content_check_code(&item, "PCKL 57 "), "trailing space");
    PASS();
}

static void test_content_check_bonus_code(void) {
    TEST("content_check_bonus_code finds match");
    Content c;
    content_load(&c, "testdata");
    int idx = content_check_bonus_code(&c, "BONUS1");
    ASSERT_EQ_INT(idx, 0, "should match bonus 0");
    idx = content_check_bonus_code(&c, "bonus2");
    ASSERT_EQ_INT(idx, 1, "should match bonus 1 (case-insensitive)");
    PASS();
}

static void test_content_check_bonus_code_miss(void) {
    TEST("content_check_bonus_code returns -1 on miss");
    Content c;
    content_load(&c, "testdata");
    int idx = content_check_bonus_code(&c, "NOPE");
    ASSERT_EQ_INT(idx, -1, "no match for NOPE");
    idx = content_check_bonus_code(&c, "");
    ASSERT_EQ_INT(idx, -1, "no match for empty");
    PASS();
}

/* ================================================================== */
/* Save/load progress tests                                            */
/* ================================================================== */

static void test_content_save_load_progress(void) {
    TEST("content save/load progress (regular items)");
    Content c;
    content_load(&c, "testdata");

    /* Unlock one item */
    c.items[0].unlocked = true;
    content_save_progress(&c, "testdata/save_test.txt");

    /* Reset and reload */
    c.items[0].unlocked = false;
    content_load_progress(&c, "testdata/save_test.txt");
    ASSERT_TRUE(c.items[0].unlocked, "Pickle should be unlocked");
    ASSERT_TRUE(!c.items[1].unlocked, "Tent should still be locked");

    remove("testdata/save_test.txt");
    PASS();
}

static void test_content_save_load_bonus_progress(void) {
    TEST("content save/load progress (bonus items)");
    Content c;
    content_load(&c, "testdata");

    /* Unlock bonus item 1 */
    c.bonus[1].unlocked = true;
    content_save_progress(&c, "testdata/save_bonus.txt");

    /* Reset and reload */
    c.bonus[1].unlocked = false;
    content_load_progress(&c, "testdata/save_bonus.txt");
    ASSERT_TRUE(!c.bonus[0].unlocked, "bonus 0 still locked");
    ASSERT_TRUE(c.bonus[1].unlocked, "bonus 1 should be unlocked");

    remove("testdata/save_bonus.txt");
    PASS();
}

static void test_content_load_progress_missing_file(void) {
    TEST("content_load_progress handles missing file");
    Content c;
    content_load(&c, "testdata");
    /* Should not crash or change state */
    content_load_progress(&c, "testdata/no_such_file.txt");
    ASSERT_TRUE(!c.items[0].unlocked, "items unchanged");
    PASS();
}

static void test_content_save_progress_all_unlocked(void) {
    TEST("content save/load with all items unlocked");
    Content c;
    content_load(&c, "testdata");
    for (int i = 0; i < c.count; i++) c.items[i].unlocked = true;
    for (int i = 0; i < c.bonus_count; i++) c.bonus[i].unlocked = true;

    content_save_progress(&c, "testdata/save_test.txt");

    Content c2;
    content_load(&c2, "testdata");
    content_load_progress(&c2, "testdata/save_test.txt");
    for (int i = 0; i < c2.count; i++)
        ASSERT_TRUE(c2.items[i].unlocked, "all regular unlocked");
    for (int i = 0; i < c2.bonus_count; i++)
        ASSERT_TRUE(c2.bonus[i].unlocked, "all bonus unlocked");

    remove("testdata/save_test.txt");
    PASS();
}

/* ================================================================== */
/* Viewer tests                                                        */
/* ================================================================== */

static void test_viewer_load(void) {
    TEST("viewer_load reads file");
    Viewer v;
    viewer_init(&v);
    bool ok = viewer_load(&v, "testdata/art/PCKL57.txt");
    ASSERT_TRUE(ok, "should load successfully");
    ASSERT_EQ_INT(v.num_lines, 5, "should have 5 lines");
    ASSERT_TRUE(v.max_width > 0, "max_width should be positive");
    PASS();
}

static void test_viewer_load_missing(void) {
    TEST("viewer_load fails on missing file");
    Viewer v;
    viewer_init(&v);
    bool ok = viewer_load(&v, "nonexistent.txt");
    ASSERT_TRUE(!ok, "should fail for missing file");
    ASSERT_EQ_INT(v.num_lines, 0, "no lines loaded");
    PASS();
}

static void test_viewer_load_empty(void) {
    TEST("viewer_load handles empty file");
    Viewer v;
    viewer_init(&v);
    bool ok = viewer_load(&v, "testdata/art/empty.txt");
    ASSERT_TRUE(ok, "open succeeds");
    ASSERT_EQ_INT(v.num_lines, 0, "no lines in empty file");
    ASSERT_EQ_INT(v.max_width, 0, "max_width is 0");
    ASSERT_EQ_INT(viewer_max_scroll_x(&v), 0, "no horizontal scroll");
    ASSERT_EQ_INT(viewer_max_scroll_y(&v), 0, "no vertical scroll");
    PASS();
}

static void test_viewer_load_tiny(void) {
    TEST("viewer_load handles single-char file");
    Viewer v;
    viewer_init(&v);
    viewer_load(&v, "testdata/art/tiny.txt");
    ASSERT_EQ_INT(v.num_lines, 1, "1 line");
    ASSERT_EQ_INT(v.max_width, 1, "width 1");
    PASS();
}

static void test_viewer_scroll_clamp(void) {
    TEST("viewer_scroll clamps to bounds");
    Viewer v;
    viewer_init(&v);
    viewer_load(&v, "testdata/art/PCKL57.txt");

    /* Scroll past top-left */
    viewer_scroll(&v, -100, -100);
    ASSERT_EQ_INT(v.scroll_x, 0, "scroll_x clamped to 0");
    ASSERT_EQ_INT(v.scroll_y, 0, "scroll_y clamped to 0");

    /* Scroll past bottom-right */
    viewer_scroll(&v, 9999, 9999);
    int mx = viewer_max_scroll_x(&v);
    int my = viewer_max_scroll_y(&v);
    ASSERT_EQ_INT(v.scroll_x, mx, "scroll_x clamped to max");
    ASSERT_EQ_INT(v.scroll_y, my, "scroll_y clamped to max");

    PASS();
}

static void test_viewer_scroll_none_needed(void) {
    TEST("viewer_scroll short content (no scroll needed)");
    Viewer v;
    viewer_init(&v);
    viewer_load(&v, "testdata/art/tiny.txt");

    viewer_scroll(&v, 10, 10);
    ASSERT_EQ_INT(v.scroll_x, 0, "can't scroll tiny file horizontally");
    ASSERT_EQ_INT(v.scroll_y, 0, "can't scroll tiny file vertically");
    PASS();
}

static void test_viewer_scroll_wide(void) {
    TEST("viewer_scroll with wide content");
    Viewer v;
    viewer_init(&v);
    viewer_load(&v, "testdata/art/wide.txt");

    int mx = viewer_max_scroll_x(&v);
    ASSERT_TRUE(mx > 0, "wide file needs horizontal scroll");
    ASSERT_EQ_INT(mx, 120 - VIEWER_COLS, "max_scroll_x = width - cols");

    viewer_scroll(&v, mx, 0);
    ASSERT_EQ_INT(v.scroll_x, mx, "can scroll to max_x");

    viewer_scroll(&v, 1, 0);
    ASSERT_EQ_INT(v.scroll_x, mx, "clamped at max_x");
    PASS();
}

static void test_viewer_scroll_tall(void) {
    TEST("viewer_scroll with tall content");
    Viewer v;
    viewer_init(&v);
    viewer_load(&v, "testdata/art/tall.txt");

    int my = viewer_max_scroll_y(&v);
    ASSERT_TRUE(my > 0, "tall file needs vertical scroll");
    ASSERT_EQ_INT(my, 100 - VIEWER_ROWS, "max_scroll_y = lines - rows");

    viewer_scroll(&v, 0, my);
    ASSERT_EQ_INT(v.scroll_y, my, "can scroll to max_y");
    PASS();
}

static void test_viewer_get_row(void) {
    TEST("viewer_get_row returns padded row");
    Viewer v;
    viewer_init(&v);
    viewer_load(&v, "testdata/art/PCKL57.txt");

    char buf[VIEWER_COLS + 1];
    viewer_get_row(&v, 0, buf, sizeof(buf));
    ASSERT_EQ_INT((int)strlen(buf), VIEWER_COLS, "row is VIEWER_COLS wide");

    /* Check that short line is padded */
    viewer_get_row(&v, 3, buf, sizeof(buf));  /* "Line 4: Short" */
    ASSERT_EQ_INT((int)strlen(buf), VIEWER_COLS, "short line padded");
    ASSERT_TRUE(buf[0] == 'L', "starts with line content");

    /* Row beyond content should be all spaces */
    viewer_get_row(&v, 20, buf, sizeof(buf));
    for (int i = 0; i < VIEWER_COLS; i++) {
        ASSERT_TRUE(buf[i] == ' ', "blank row is spaces");
    }

    PASS();
}

static void test_viewer_get_row_scrolled(void) {
    TEST("viewer_get_row respects scroll offset");
    Viewer v;
    viewer_init(&v);
    viewer_load(&v, "testdata/art/PCKL57.txt");

    /* Scroll right by 5 */
    viewer_scroll(&v, 5, 0);

    char buf[VIEWER_COLS + 1];
    viewer_get_row(&v, 0, buf, sizeof(buf));
    /* "Line_1:_Hello..." scrolled right 5 → "1:_He..." */
    ASSERT_TRUE(buf[0] == '1', "first visible char after scroll");

    PASS();
}

static void test_viewer_get_row_empty_file(void) {
    TEST("viewer_get_row on empty file returns spaces");
    Viewer v;
    viewer_init(&v);
    viewer_load(&v, "testdata/art/empty.txt");

    char buf[VIEWER_COLS + 1];
    viewer_get_row(&v, 0, buf, sizeof(buf));
    ASSERT_EQ_INT((int)strlen(buf), VIEWER_COLS, "correct width");
    for (int i = 0; i < VIEWER_COLS; i++)
        ASSERT_TRUE(buf[i] == ' ', "all spaces");
    PASS();
}

static void test_viewer_init_zeroes(void) {
    TEST("viewer_init zeroes all fields");
    Viewer v;
    v.num_lines = 99;
    v.max_width = 99;
    v.scroll_x  = 99;
    v.scroll_y  = 99;
    viewer_init(&v);
    ASSERT_EQ_INT(v.num_lines, 0, "num_lines zeroed");
    ASSERT_EQ_INT(v.max_width, 0, "max_width zeroed");
    ASSERT_EQ_INT(v.scroll_x, 0, "scroll_x zeroed");
    ASSERT_EQ_INT(v.scroll_y, 0, "scroll_y zeroed");
    PASS();
}

static void test_viewer_reload_resets_scroll(void) {
    TEST("viewer_load resets scroll on reload");
    Viewer v;
    viewer_init(&v);
    viewer_load(&v, "testdata/art/PCKL57.txt");
    viewer_scroll(&v, 5, 3);
    ASSERT_TRUE(v.scroll_x > 0 || v.scroll_y > 0, "scrolled");

    /* Reload should reset */
    viewer_load(&v, "testdata/art/tiny.txt");
    ASSERT_EQ_INT(v.scroll_x, 0, "scroll_x reset");
    ASSERT_EQ_INT(v.scroll_y, 0, "scroll_y reset");
    PASS();
}

/* ================================================================== */
/* Main                                                                */
/* ================================================================== */

int main(void) {
    printf("\n=== ASCII Capsule — Host-Side Tests ===\n\n");

    setup_test_data();

    /* Hash function tests */
    printf("Hash function:\n");
    test_hash_deterministic();
    test_hash_case_insensitive();
    test_hash_different_inputs();
    test_hash_empty_string();
    test_hash_spaces_matter();
    test_hash_known_values();

    /* Content loading tests */
    printf("\nContent loading:\n");
    test_content_load();
    test_content_load_bonus();
    test_content_load_missing();
    test_content_load_empty_manifest();
    test_content_load_malformed_lines();
    test_content_load_overflow();

    /* Code checking tests */
    printf("\nCode checking:\n");
    test_content_check_code_exact();
    test_content_check_code_case_insensitive();
    test_content_check_code_wrong();
    test_content_check_bonus_code();
    test_content_check_bonus_code_miss();

    /* Save/load progress tests */
    printf("\nSave/load progress:\n");
    test_content_save_load_progress();
    test_content_save_load_bonus_progress();
    test_content_load_progress_missing_file();
    test_content_save_progress_all_unlocked();

    /* Viewer tests */
    printf("\nViewer module:\n");
    test_viewer_load();
    test_viewer_load_missing();
    test_viewer_load_empty();
    test_viewer_load_tiny();
    test_viewer_scroll_clamp();
    test_viewer_scroll_none_needed();
    test_viewer_scroll_wide();
    test_viewer_scroll_tall();
    test_viewer_get_row();
    test_viewer_get_row_scrolled();
    test_viewer_get_row_empty_file();
    test_viewer_init_zeroes();
    test_viewer_reload_resets_scroll();

    cleanup_test_data();

    printf("\n=== Results: %d/%d tests passed ===\n\n",
           tests_passed, tests_run);

    return (tests_passed == tests_run) ? 0 : 1;
}
