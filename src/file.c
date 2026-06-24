#include "file.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void editor_replace_document(Editor *editor, Line *lines, size_t count);

static char *duplicate(const char *text, size_t length) {
    char *copy = malloc(length + 1);
    if (!copy) return NULL;
    memcpy(copy, text, length);
    copy[length] = 0;
    return copy;
}

int file_is_holyc(const char *path) {
    const char *extension = strrchr(path, '.');
    return extension && (tolower((unsigned char)extension[1]) == 'h') &&
           (tolower((unsigned char)extension[2]) == 'c') && extension[3] == 0;
}

int file_open(Editor *editor, const char *path) {
    FILE *file = fopen(path, "rb");
    if (!file) return 0;
    Line *lines = NULL;
    size_t count = 0, capacity = 0;
    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), file)) {
        size_t length = strlen(buffer);
        while (length && (buffer[length - 1] == '\n' || buffer[length - 1] == '\r')) length--;
        if (count == capacity) {
            capacity = capacity ? capacity * 2 : 32;
            Line *grown = realloc(lines, capacity * sizeof(*lines));
            if (!grown) { fclose(file); return 0; }
            lines = grown;
        }
        lines[count].text = duplicate(buffer, length);
        lines[count].length = length;
        if (!lines[count].text) { fclose(file); return 0; }
        count++;
    }
    fclose(file);
    editor_replace_document(editor, lines, count);
    strncpy(editor->filename, path, sizeof(editor->filename) - 1);
    editor->filename[sizeof(editor->filename) - 1] = 0;
    editor->holy_c = file_is_holyc(path);
    editor->dirty = 0;
    editor_set_message(editor, "Opened %s", path);
    return 1;
}

int file_save(Editor *editor, const char *path) {
    FILE *file = fopen(path, "wb");
    if (!file) return 0;
    for (size_t i = 0; i < editor->line_count; i++) {
        if (fwrite(editor->lines[i].text, 1, editor->lines[i].length, file) != editor->lines[i].length ||
            (i + 1 < editor->line_count && fputc('\n', file) == EOF)) { fclose(file); return 0; }
    }
    if (fclose(file) != 0) return 0;
    strncpy(editor->filename, path, sizeof(editor->filename) - 1);
    editor->filename[sizeof(editor->filename) - 1] = 0;
    editor->holy_c = file_is_holyc(path);
    editor->dirty = 0;
    editor_set_message(editor, "Saved %s", path);
    return 1;
}
