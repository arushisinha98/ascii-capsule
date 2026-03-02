/**
 * content.c — Content loading and code checking
 *
 * Portable module: only uses standard C library functions (stdio, string, ctype).
 * Can be compiled and tested on any platform.
 */

#include "content.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

/* ------------------------------------------------------------------ */
/* Helpers                                                             */
/* ------------------------------------------------------------------ */

static void trim_trailing(char* s) {
    int len = (int)strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r' ||
                       s[len - 1] == ' '  || s[len - 1] == '\t')) {
        s[--len] = '\0';
    }
}

static int ci_strcmp(const char* a, const char* b) {
    while (*a && *b) {
        int ca = toupper((unsigned char)*a);
        int cb = toupper((unsigned char)*b);
        if (ca != cb) return ca - cb;
        a++;
        b++;
    }
    return toupper((unsigned char)*a) - toupper((unsigned char)*b);
}

static void safe_copy(char* dst, const char* src, int maxlen) {
    strncpy(dst, src, maxlen - 1);
    dst[maxlen - 1] = '\0';
}

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */

bool content_load(Content* c, const char* base_path) {
    char path[256];
    snprintf(path, sizeof(path), "%s/manifest.txt", base_path);

    FILE* f = fopen(path, "r");
    if (!f) return false;

    c->count = 0;
    char line[256];

    while (fgets(line, sizeof(line), f) && c->count < MAX_ITEMS) {
        /* Skip comments and blank lines */
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
            continue;

        trim_trailing(line);
        if (strlen(line) == 0)
            continue;

        /* Parse: Name|Code|ArtFile|LetterFile */
        char* tok_name   = strtok(line, "|");
        char* tok_code   = strtok(NULL, "|");
        char* tok_art    = strtok(NULL, "|");
        char* tok_letter = strtok(NULL, "|");

        if (!tok_name || !tok_code || !tok_art || !tok_letter)
            continue;

        ContentItem* item = &c->items[c->count];
        safe_copy(item->name,        tok_name,   MAX_NAME_LEN);
        safe_copy(item->code,        tok_code,   MAX_CODE_LEN);
        safe_copy(item->art_file,    tok_art,    MAX_PATH_LEN);
        safe_copy(item->letter_file, tok_letter, MAX_PATH_LEN);
        item->unlocked = false;

        c->count++;
    }

    fclose(f);
    return c->count > 0;
}

bool content_check_code(const ContentItem* item, const char* input) {
    return ci_strcmp(item->code, input) == 0;
}

void content_save_progress(const Content* c, const char* path) {
    FILE* f = fopen(path, "w");
    if (!f) return;

    for (int i = 0; i < c->count; i++) {
        if (c->items[i].unlocked) {
            fprintf(f, "%s\n", c->items[i].name);
        }
    }
    fclose(f);
}

void content_load_progress(Content* c, const char* path) {
    FILE* f = fopen(path, "r");
    if (!f) return;

    char line[MAX_NAME_LEN + 4];
    while (fgets(line, sizeof(line), f)) {
        trim_trailing(line);
        if (strlen(line) == 0) continue;

        for (int i = 0; i < c->count; i++) {
            if (strcmp(c->items[i].name, line) == 0) {
                c->items[i].unlocked = true;
                break;
            }
        }
    }
    fclose(f);
}
