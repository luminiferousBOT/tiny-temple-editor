#include "editor.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *copy_text(const char *text, size_t length) {
    char *copy = malloc(length + 1);
    if (copy == NULL) return NULL;
    if (length) memcpy(copy, text, length);
    copy[length] = '\0';
    return copy;
}

static void ensure_lines(Editor *editor, size_t needed) {
    if (needed <= editor->line_capacity) return;
    size_t capacity = editor->line_capacity ? editor->line_capacity * 2 : 16;
    while (capacity < needed) capacity *= 2;
    Line *lines = realloc(editor->lines, capacity * sizeof(*lines));
    if (lines == NULL) return;
    editor->lines = lines;
    editor->line_capacity = capacity;
}

static void insert_line(Editor *editor, size_t at, const char *text, size_t length) {
    ensure_lines(editor, editor->line_count + 1);
    if (editor->line_count + 1 > editor->line_capacity) return;
    memmove(&editor->lines[at + 1], &editor->lines[at],
            (editor->line_count - at) * sizeof(Line));
    editor->lines[at].text = copy_text(text, length);
    editor->lines[at].length = length;
    editor->line_count++;
}

static void delete_line(Editor *editor, size_t at) {
    free(editor->lines[at].text);
    memmove(&editor->lines[at], &editor->lines[at + 1],
            (editor->line_count - at - 1) * sizeof(Line));
    editor->line_count--;
}

void editor_init(Editor *editor) {
    memset(editor, 0, sizeof(*editor));
    insert_line(editor, 0, "", 0);
    editor_set_message(editor, "Ctrl+S Save | Ctrl+F Search | Ctrl+Q Quit");
}

void editor_free(Editor *editor) {
    for (size_t i = 0; i < editor->line_count; i++) free(editor->lines[i].text);
    free(editor->lines);
    memset(editor, 0, sizeof(*editor));
}

void editor_set_message(Editor *editor, const char *format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(editor->message, sizeof(editor->message), format, args);
    va_end(args);
}

void editor_insert_char(Editor *editor, char character) {
    Line *line = &editor->lines[editor->cursor_y];
    char *text = realloc(line->text, line->length + 2);
    if (text == NULL) return;
    line->text = text;
    memmove(line->text + editor->cursor_x + 1, line->text + editor->cursor_x,
            line->length - editor->cursor_x + 1);
    line->text[editor->cursor_x++] = character;
    line->length++;
    editor->dirty = 1;
}

void editor_insert_newline(Editor *editor) {
    Line *line = &editor->lines[editor->cursor_y];
    size_t right_length = line->length - editor->cursor_x;
    char *right = copy_text(line->text + editor->cursor_x, right_length);
    if (right == NULL) return;
    line->text[editor->cursor_x] = '\0';
    line->length = editor->cursor_x;
    insert_line(editor, editor->cursor_y + 1, right, right_length);
    free(right);
    editor->cursor_y++;
    editor->cursor_x = 0;
    editor->dirty = 1;
}

void editor_backspace(Editor *editor) {
    if (editor->cursor_x > 0) {
        Line *line = &editor->lines[editor->cursor_y];
        memmove(line->text + editor->cursor_x - 1, line->text + editor->cursor_x,
                line->length - editor->cursor_x + 1);
        editor->cursor_x--;
        line->length--;
        editor->dirty = 1;
    } else if (editor->cursor_y > 0) {
        size_t previous = editor->cursor_y - 1;
        Line *above = &editor->lines[previous];
        Line *line = &editor->lines[editor->cursor_y];
        char *text = realloc(above->text, above->length + line->length + 1);
        if (text == NULL) return;
        editor->cursor_x = above->length;
        above->text = text;
        memcpy(above->text + above->length, line->text, line->length + 1);
        above->length += line->length;
        delete_line(editor, editor->cursor_y--);
        editor->dirty = 1;
    }
}

void editor_delete(Editor *editor) {
    Line *line = &editor->lines[editor->cursor_y];
    if (editor->cursor_x < line->length) {
        memmove(line->text + editor->cursor_x, line->text + editor->cursor_x + 1,
                line->length - editor->cursor_x);
        line->length--;
        editor->dirty = 1;
    } else if (editor->cursor_y + 1 < editor->line_count) {
        editor_move_cursor(editor, 0, 1);
        editor_backspace(editor);
    }
}

void editor_move_cursor(Editor *editor, int dx, int dy) {
    long y = (long)editor->cursor_y + dy;
    if (y < 0) y = 0;
    if ((size_t)y >= editor->line_count) y = (long)editor->line_count - 1;
    editor->cursor_y = (size_t)y;
    long x = (long)editor->cursor_x + dx;
    if (x < 0) x = 0;
    if ((size_t)x > editor->lines[editor->cursor_y].length)
        x = (long)editor->lines[editor->cursor_y].length;
    editor->cursor_x = (size_t)x;
}

void editor_move_home(Editor *editor) { editor->cursor_x = 0; }
void editor_move_end(Editor *editor) { editor->cursor_x = editor->lines[editor->cursor_y].length; }
void editor_page_move(Editor *editor, int direction, int rows) { editor_move_cursor(editor, 0, direction * rows); }

/* Used by file.c to replace the complete document. */
void editor_replace_document(Editor *editor, Line *lines, size_t count) {
    for (size_t i = 0; i < editor->line_count; i++) free(editor->lines[i].text);
    free(editor->lines);
    editor->lines = lines;
    editor->line_count = count ? count : 1;
    editor->line_capacity = count ? count : 1;
    if (!count) editor->lines = calloc(1, sizeof(Line));
    editor->cursor_x = editor->cursor_y = editor->scroll_x = editor->scroll_y = 0;
}
