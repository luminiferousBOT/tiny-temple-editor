#ifndef TTE_EDITOR_H
#define TTE_EDITOR_H

#include <stddef.h>

#define TTE_FILENAME_MAX 260
#define TTE_MESSAGE_MAX 256

typedef struct {
    char *text;
    size_t length;
} Line;

typedef struct {
    Line *lines;
    size_t line_count;
    size_t line_capacity;
    size_t cursor_x;
    size_t cursor_y;
    size_t scroll_x;
    size_t scroll_y;
    char filename[TTE_FILENAME_MAX];
    char message[TTE_MESSAGE_MAX];
    int dirty;
    int quit_warning;
    int holy_c;
} Editor;

void editor_init(Editor *editor);
void editor_free(Editor *editor);
void editor_set_message(Editor *editor, const char *format, ...);
void editor_insert_char(Editor *editor, char character);
void editor_insert_newline(Editor *editor);
void editor_backspace(Editor *editor);
void editor_delete(Editor *editor);
void editor_move_cursor(Editor *editor, int dx, int dy);
void editor_move_home(Editor *editor);
void editor_move_end(Editor *editor);
void editor_page_move(Editor *editor, int direction, int rows);

#endif
