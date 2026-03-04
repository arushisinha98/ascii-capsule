/**
 * content.c — Content loading and code checking
 *
 * Portable module: only uses standard C library functions.
 * Can be compiled and tested on any platform.
 *
 * Codes are stored as djb2-xor hashes in the manifest, so the actual
 * code text never appears in the ROM binary.
 */

#include "content.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

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

static void safe_copy(char* dst, const char* src, int maxlen) {
    strncpy(dst, src, maxlen - 1);
    dst[maxlen - 1] = '\0';
}

/* ------------------------------------------------------------------ */
/* Hash function (public — also used by tests / tools)                 */
/* ------------------------------------------------------------------ */

uint32_t content_code_hash(const char* input) {
    uint32_t h = 5381;
    for (int i = 0; input[i]; i++) {
        int ch = toupper((unsigned char)input[i]);
        h = ((h << 5) + h) ^ (uint32_t)ch;
    }
    return h;
}

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */

bool content_load(Content* c, const char* base_path) {
    char path[256];
    snprintf(path, sizeof(path), "%s/manifest.txt", base_path);

    FILE* f = fopen(path, "r");
    if (!f) return false;

    c->count       = 0;
    c->bonus_count = 0;
    char line[256];

    while (fgets(line, sizeof(line), f)) {
        /* Skip comments and blank lines */
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
            continue;

        trim_trailing(line);
        if (strlen(line) == 0)
            continue;

        /* ---- Bonus code line: +HASH|message_file ---- */
        if (line[0] == '+' && c->bonus_count < MAX_BONUS) {
            char* tok_hash = strtok(line + 1, "|");
            char* tok_msg  = strtok(NULL, "|");

            if (!tok_hash || !tok_msg)
                continue;

            BonusItem* b = &c->bonus[c->bonus_count];
            b->code_hash = (uint32_t)strtoul(tok_hash, NULL, 10);
            safe_copy(b->message_file, tok_msg, MAX_PATH_LEN);
            b->unlocked  = false;

            c->bonus_count++;
            continue;
        }

        /* ---- Regular item line: Name|HASH|ArtFile|MessageFile ---- */
        if (c->count >= MAX_ITEMS)
            continue;

        char* tok_name = strtok(line, "|");
        char* tok_hash = strtok(NULL, "|");
        char* tok_art  = strtok(NULL, "|");
        char* tok_msg  = strtok(NULL, "|");

        if (!tok_name || !tok_hash || !tok_art || !tok_msg)
            continue;

        ContentItem* item = &c->items[c->count];
        safe_copy(item->name,         tok_name, MAX_NAME_LEN);
        item->code_hash = (uint32_t)strtoul(tok_hash, NULL, 10);
        safe_copy(item->art_file,     tok_art,  MAX_PATH_LEN);
        safe_copy(item->message_file, tok_msg,  MAX_PATH_LEN);
        item->unlocked = false;

        c->count++;
    }

    fclose(f);
    return c->count > 0;
}

bool content_check_code(const ContentItem* item, const char* input) {
    return content_code_hash(input) == item->code_hash;
}

int content_check_bonus_code(const Content* c, const char* input) {
    uint32_t h = content_code_hash(input);
    for (int i = 0; i < c->bonus_count; i++) {
        if (c->bonus[i].code_hash == h)
            return i;
    }
    return -1;
}

void content_save_progress(const Content* c, const char* path) {
    FILE* f = fopen(path, "w");
    if (!f) return;

    for (int i = 0; i < c->count; i++) {
        if (c->items[i].unlocked) {
            fprintf(f, "%s\n", c->items[i].name);
        }
    }

    /* Save bonus unlocks as "+INDEX" lines */
    for (int i = 0; i < c->bonus_count; i++) {
        if (c->bonus[i].unlocked) {
            fprintf(f, "+%d\n", i);
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

        /* Bonus unlock: "+INDEX" */
        if (line[0] == '+') {
            int idx = atoi(line + 1);
            if (idx >= 0 && idx < c->bonus_count) {
                c->bonus[idx].unlocked = true;
            }
            continue;
        }

        for (int i = 0; i < c->count; i++) {
            if (strcmp(c->items[i].name, line) == 0) {
                c->items[i].unlocked = true;
                break;
            }
        }
    }
    fclose(f);
}
