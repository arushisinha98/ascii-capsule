/**
 * content.h — Content loading and code checking
 *
 * Portable module (no NDS dependencies). Loads the content manifest
 * and provides code-checking logic. Testable on the host machine.
 */

#ifndef CONTENT_H
#define CONTENT_H

#include <stdbool.h>

#define MAX_ITEMS       16
#define MAX_NAME_LEN    32
#define MAX_CODE_LEN    20
#define MAX_PATH_LEN    64

typedef struct {
    char name[MAX_NAME_LEN];
    char code[MAX_CODE_LEN];
    char art_file[MAX_PATH_LEN];
    char letter_file[MAX_PATH_LEN];
    bool unlocked;
} ContentItem;

typedef struct {
    ContentItem items[MAX_ITEMS];
    int count;
} Content;

/**
 * Load the content manifest from base_path/manifest.txt
 *
 * Manifest format (pipe-delimited, one item per line):
 *   Display Name|CODE|art_filename.txt|letter_filename.txt
 *
 * Lines starting with '#' are comments. Empty lines are skipped.
 *
 * @param c         Content structure to populate
 * @param base_path Directory containing manifest.txt (e.g. "nitro:" or "testdata")
 * @return true if at least one item was loaded
 */
bool content_load(Content* c, const char* base_path);

/**
 * Case-insensitive code comparison.
 *
 * @param item  Content item to check against
 * @param input User-entered code string
 * @return true if the input matches the item's code (case-insensitive)
 */
bool content_check_code(const ContentItem* item, const char* input);

/**
 * Save unlock progress to a file.
 *
 * Writes the names of all unlocked items, one per line.
 *
 * @param c    Content structure
 * @param path File path to save to (e.g. "fat:/ascii_memories_save.txt")
 */
void content_save_progress(const Content* c, const char* path);

/**
 * Load unlock progress from a file.
 *
 * Marks items as unlocked if their name appears in the save file.
 *
 * @param c    Content structure
 * @param path File path to load from
 */
void content_load_progress(Content* c, const char* path);

#endif /* CONTENT_H */
