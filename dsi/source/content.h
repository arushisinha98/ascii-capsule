/**
 * content.h — Content loading and code checking
 *
 * Portable module (no NDS dependencies). Loads the content manifest
 * and provides code-checking logic. Testable on the host machine.
 */

#ifndef CONTENT_H
#define CONTENT_H

#include <stdbool.h>
#include <stdint.h>

#define MAX_ITEMS       16
#define MAX_BONUS       16
#define MAX_NAME_LEN    32
#define MAX_PATH_LEN    64

typedef struct {
    char     name[MAX_NAME_LEN];
    uint32_t code_hash;              /* djb2-xor hash of uppercase code */
    char     art_file[MAX_PATH_LEN];
    char     message_file[MAX_PATH_LEN];
    bool     unlocked;
} ContentItem;

typedef struct {
    uint32_t code_hash;
    char     message_file[MAX_PATH_LEN];
    bool     unlocked;
} BonusItem;

typedef struct {
    ContentItem items[MAX_ITEMS];
    int         count;
    BonusItem   bonus[MAX_BONUS];
    int         bonus_count;
} Content;

/**
 * Compute the djb2-xor hash of a string (uppercased).
 * This is the same hash used to store codes in the manifest.
 */
uint32_t content_code_hash(const char* input);

/**
 * Load the content manifest from base_path/manifest.txt
 *
 * Manifest format (pipe-delimited, one item per line):
 *   Display Name|HASH|art_filename.txt|message_filename.txt
 *
 * Bonus codes (lines starting with '+'):
 *   +HASH|message_filename.txt
 *
 * HASH is the decimal djb2-xor hash of the uppercase code.
 * Lines starting with '#' are comments. Empty lines are skipped.
 *
 * @param c         Content structure to populate
 * @param base_path Directory containing manifest.txt (e.g. "nitro:" or "testdata")
 * @return true if at least one item was loaded
 */
bool content_load(Content* c, const char* base_path);

/**
 * Check user input against an item's hashed code.
 *
 * @param item  Content item to check against
 * @param input User-entered code string
 * @return true if the hash of input matches the item's code_hash
 */
bool content_check_code(const ContentItem* item, const char* input);

/**
 * Check user input against all bonus codes.
 *
 * @param c     Content structure
 * @param input User-entered code string
 * @return index of matching bonus item, or -1 if no match
 */
int content_check_bonus_code(const Content* c, const char* input);

/**
 * Save unlock progress to a file.
 *
 * Writes the names of all unlocked items (and bonus indices), one per line.
 *
 * @param c    Content structure
 * @param path File path to save to (e.g. "fat:/ascii_capsule_save.txt")
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
