/**
 * test_content.c — Host-side tests for content and viewer modules
 *
 * Compiles on the development machine (macOS/Linux) without NDS deps.
 * Tests content manifest parsing, code checking, and viewer scrolling.
 *
 * Build:  cc -Wall -Wextra -I../source -o test_content test_content.c ../source/content.c ../source/viewer.c
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
        printf("  TEST: %-40s ", name); \
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

static void write_file(const char* path, const char* content) {
    FILE* f = fopen(path, "w");
    if (f) {
        fputs(content, f);
        fclose(f);
    }
}

static void setup_test_data(void) {
    create_dir("testdata");
    create_dir("testdata/art");
    create_dir("testdata/letters");

    /* manifest.txt */
    write_file("testdata/manifest.txt",
        "# Test manifest\n"
        "Pickle|PCKL 57|PCKL57.txt|PCKL57.txt\n"
        "Tent|TNT 404|TNT404.txt|TNT404.txt\n"
        "Cozy|C0ZY|C0ZY.txt|C0ZY.txt\n"
    );

    /* A small art file (lines must exceed 32 chars for scroll tests) */
    write_file("testdata/art/PCKL57.txt",
        "Line_1:_Hello_world!!_padded_out_to_exceed_32_characters_wide\n"
        "Line_2:_ABCDEFGHIJKLMNOPQRSTUVWXYZ_1234567890_extra_extras!!\n"
        "Line_3:_1234567890123456789012345678901234567890_long_line!!!\n"
        "Line 4: Short\n"
        "Line 5: End\n"
    );

    /* A save file */
    write_file("testdata/save.txt",
        "Pickle\n"
    );
}

static void cleanup_test_data(void) {
    remove("testdata/save.txt");
    remove("testdata/art/PCKL57.txt");
    remove("testdata/letters");
    remove("testdata/art");
    remove("testdata/manifest.txt");
    remove("testdata");
}

/* ------------------------------------------------------------------ */
/* Content tests                                                       */
/* ------------------------------------------------------------------ */

static void test_content_load(void) {
    TEST("content_load parses manifest");
    Content c;
    bool ok = content_load(&c, "testdata");
    ASSERT_TRUE(ok, "content_load should succeed");
    ASSERT_EQ_INT(c.count, 3, "should load 3 items");
    ASSERT_EQ_STR(c.items[0].name, "Pickle", "first item name");
    ASSERT_EQ_STR(c.items[0].code, "PCKL 57", "first item code");
    ASSERT_EQ_STR(c.items[0].art_file, "PCKL57.txt", "first item art");
    ASSERT_EQ_STR(c.items[0].letter_file, "PCKL57.txt", "first item letter");
    ASSERT_EQ_STR(c.items[1].name, "Tent", "second item name");
    ASSERT_EQ_STR(c.items[2].name, "Cozy", "third item name");
    ASSERT_TRUE(!c.items[0].unlocked, "items start locked");
    PASS();
}

static void test_content_load_missing(void) {
    TEST("content_load fails on missing dir");
    Content c;
    bool ok = content_load(&c, "nonexistent");
    ASSERT_TRUE(!ok, "should fail for missing dir");
    PASS();
}

static void test_content_check_code_exact(void) {
    TEST("content_check_code exact match");
    ContentItem item;
    strncpy(item.code, "PCKL 57", MAX_CODE_LEN);
    ASSERT_TRUE(content_check_code(&item, "PCKL 57"), "exact match");
    PASS();
}

static void test_content_check_code_case_insensitive(void) {
    TEST("content_check_code case insensitive");
    ContentItem item;
    strncpy(item.code, "PCKL 57", MAX_CODE_LEN);
    ASSERT_TRUE(content_check_code(&item, "pckl 57"), "lower case");
    ASSERT_TRUE(content_check_code(&item, "Pckl 57"), "mixed case");
    PASS();
}

static void test_content_check_code_wrong(void) {
    TEST("content_check_code rejects wrong code");
    ContentItem item;
    strncpy(item.code, "PCKL 57", MAX_CODE_LEN);
    ASSERT_TRUE(!content_check_code(&item, "WRONG"), "wrong code");
    ASSERT_TRUE(!content_check_code(&item, "PCKL57"), "missing space");
    ASSERT_TRUE(!content_check_code(&item, ""), "empty input");
    PASS();
}

static void test_content_save_load_progress(void) {
    TEST("content save/load progress");
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

/* ------------------------------------------------------------------ */
/* Viewer tests                                                        */
/* ------------------------------------------------------------------ */

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

static void test_viewer_get_row(void) {
    TEST("viewer_get_row returns padded row");
    Viewer v;
    viewer_init(&v);
    viewer_load(&v, "testdata/art/PCKL57.txt");

    char buf[VIEWER_COLS + 1];
    viewer_get_row(&v, 0, buf, sizeof(buf));
    ASSERT_EQ_INT((int)strlen(buf), VIEWER_COLS, "row is VIEWER_COLS wide");

    /* Check that short line is padded */
    viewer_get_row(&v, 3, buf, sizeof(buf));  /* "Short" */
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
    /* "Line_1:_Hello_world!!_padded..." scrolled right 5 → "1:_He..." */
    ASSERT_TRUE(buf[0] == '1', "first visible char after scroll");

    PASS();
}

/* ------------------------------------------------------------------ */
/* Main                                                                */
/* ------------------------------------------------------------------ */

int main(void) {
    printf("\n=== ASCII Memories — Host-Side Tests ===\n\n");

    setup_test_data();

    /* Content tests */
    printf("Content module:\n");
    test_content_load();
    test_content_load_missing();
    test_content_check_code_exact();
    test_content_check_code_case_insensitive();
    test_content_check_code_wrong();
    test_content_save_load_progress();

    /* Viewer tests */
    printf("\nViewer module:\n");
    test_viewer_load();
    test_viewer_load_missing();
    test_viewer_scroll_clamp();
    test_viewer_get_row();
    test_viewer_get_row_scrolled();

    cleanup_test_data();

    printf("\n=== Results: %d/%d tests passed ===\n\n",
           tests_passed, tests_run);

    return (tests_passed == tests_run) ? 0 : 1;
}
