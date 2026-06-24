#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "editor.h"
#include "file.h"
#include "search.h"
#include "syntax.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

enum { KEY_ESC = 27, CTRL_Q = 17, CTRL_S = 19, CTRL_F = 6 };

static HANDLE output_handle;
static HANDLE input_handle;
static WORD original_attributes;
static DWORD original_input_mode;

static void render(Editor *editor);

static int digits(size_t value) {
    int result = 1;
    while (value >= 10) { value /= 10; result++; }
    return result;
}

static WORD color_for(SyntaxColor color) {
    switch (color) {
        case COLOR_KEYWORD: return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
        case COLOR_TYPE: return FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        case COLOR_NUMBER: return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        case COLOR_STRING: return FOREGROUND_GREEN | FOREGROUND_INTENSITY;
        case COLOR_COMMENT: return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        case COLOR_DIRECTIVE: return FOREGROUND_RED | FOREGROUND_INTENSITY;
        default: return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
    }
}

static void write_at(SHORT x, SHORT y, const char *text, WORD color) {
    DWORD written;
    COORD position = { x, y };
    SetConsoleCursorPosition(output_handle, position);
    SetConsoleTextAttribute(output_handle, color | BACKGROUND_BLUE);
    WriteConsoleA(output_handle, text, (DWORD)strlen(text), &written, NULL);
}

static void clear_line(SHORT y, SHORT width) {
    DWORD written;
    COORD position = { 0, y };
    FillConsoleOutputCharacterA(output_handle, ' ', width, position, &written);
    FillConsoleOutputAttribute(output_handle, BACKGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
                               width, position, &written);
}

static void prompt(Editor *editor, const char *label, char *result, size_t result_size) {
    size_t length = 0;
    result[0] = 0;
    for (;;) {
        editor_set_message(editor, "%s%s", label, result);
        render(editor);
        INPUT_RECORD record;
        DWORD read;
        ReadConsoleInputA(input_handle, &record, 1, &read);
        if (record.EventType != KEY_EVENT || !record.Event.KeyEvent.bKeyDown) continue;
        KEY_EVENT_RECORD key = record.Event.KeyEvent;
        if (key.wVirtualKeyCode == VK_ESCAPE) { result[0] = 0; return; }
        if (key.wVirtualKeyCode == VK_RETURN) return;
        if (key.wVirtualKeyCode == VK_BACK && length) result[--length] = 0;
        else if (key.uChar.AsciiChar >= 32 && key.uChar.AsciiChar < 127 && length + 1 < result_size) {
            result[length++] = key.uChar.AsciiChar;
            result[length] = 0;
        }
    }
}

static void render(Editor *editor) {
    CONSOLE_SCREEN_BUFFER_INFO info;
    GetConsoleScreenBufferInfo(output_handle, &info);
    SHORT width = info.srWindow.Right - info.srWindow.Left + 1;
    SHORT height = info.srWindow.Bottom - info.srWindow.Top + 1;
    int text_rows = height - 2;
    int gutter = digits(editor->line_count) + 3;
    if (editor->cursor_y < editor->scroll_y) editor->scroll_y = editor->cursor_y;
    if (editor->cursor_y >= editor->scroll_y + (size_t)text_rows) editor->scroll_y = editor->cursor_y - text_rows + 1;
    if (editor->cursor_x < editor->scroll_x) editor->scroll_x = editor->cursor_x;
    if (editor->cursor_x >= editor->scroll_x + (size_t)(width - gutter)) editor->scroll_x = editor->cursor_x - (width - gutter) + 1;

    for (int screen_y = 0; screen_y < text_rows; screen_y++) {
        clear_line((SHORT)screen_y, width);
        size_t row = editor->scroll_y + (size_t)screen_y;
        if (row >= editor->line_count) continue;
        char number[32];
        snprintf(number, sizeof(number), "%*lu | ", gutter - 3, (unsigned long)(row + 1));
        write_at(0, (SHORT)screen_y, number, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
        Line *line = &editor->lines[row];
        size_t start = editor->scroll_x;
        size_t end = line->length;
        if (end > start + (size_t)(width - gutter)) end = start + (size_t)(width - gutter);
        for (size_t x = start; x < end; x++) {
            char one[2] = { line->text[x] == '\t' ? ' ' : line->text[x], 0 };
            write_at((SHORT)(gutter + x - start), (SHORT)screen_y, one,
                     color_for(syntax_color_at(line->text, line->length, x, editor->holy_c)));
        }
    }
    clear_line((SHORT)(height - 2), width);
    char status[512];
    snprintf(status, sizeof(status), " %s %s | %s | Ln %lu, Col %lu ",
             editor->filename[0] ? editor->filename : "[No Name]", editor->dirty ? "[+]" : "",
             editor->holy_c ? "HolyC" : "Text", (unsigned long)(editor->cursor_y + 1),
             (unsigned long)(editor->cursor_x + 1));
    write_at(0, (SHORT)(height - 2), status, BACKGROUND_RED | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    clear_line((SHORT)(height - 1), width);
    write_at(0, (SHORT)(height - 1), editor->message, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    COORD cursor = { (SHORT)(gutter + editor->cursor_x - editor->scroll_x), (SHORT)(editor->cursor_y - editor->scroll_y) };
    SetConsoleCursorPosition(output_handle, cursor);
    SetConsoleTextAttribute(output_handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY | BACKGROUND_BLUE);
}

static int handle_key(Editor *editor, KEY_EVENT_RECORD key, int page_rows) {
    char character = key.uChar.AsciiChar;
    int control_down = (key.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) != 0;
    if ((control_down && key.wVirtualKeyCode == 'Q') || character == CTRL_Q) {
        if (editor->dirty && !editor->quit_warning) {
            editor->quit_warning = 1;
            editor_set_message(editor, "Unsaved changes. Press Ctrl+Q again to quit.");
            return 1;
        }
        return 0;
    }
    editor->quit_warning = 0;
    if ((control_down && key.wVirtualKeyCode == 'S') || character == CTRL_S) {
        char name[TTE_FILENAME_MAX];
        if (!editor->filename[0]) { prompt(editor, "Save as: ", name, sizeof(name)); if (!name[0]) return 1; }
        else strcpy(name, editor->filename);
        if (!file_save(editor, name)) editor_set_message(editor, "Could not save %s", name);
    } else if ((control_down && key.wVirtualKeyCode == 'F') || character == CTRL_F) {
        char query[128];
        prompt(editor, "Search: ", query, sizeof(query));
        if (query[0]) editor_set_message(editor, search_find(editor, query) ? "Found: %s" : "Not found: %s", query);
    } else if (key.wVirtualKeyCode == VK_LEFT) editor_move_cursor(editor, -1, 0);
    else if (key.wVirtualKeyCode == VK_RIGHT) editor_move_cursor(editor, 1, 0);
    else if (key.wVirtualKeyCode == VK_UP) editor_move_cursor(editor, 0, -1);
    else if (key.wVirtualKeyCode == VK_DOWN) editor_move_cursor(editor, 0, 1);
    else if (key.wVirtualKeyCode == VK_HOME) editor_move_home(editor);
    else if (key.wVirtualKeyCode == VK_END) editor_move_end(editor);
    else if (key.wVirtualKeyCode == VK_PRIOR) editor_page_move(editor, -1, page_rows);
    else if (key.wVirtualKeyCode == VK_NEXT) editor_page_move(editor, 1, page_rows);
    else if (key.wVirtualKeyCode == VK_BACK) editor_backspace(editor);
    else if (key.wVirtualKeyCode == VK_DELETE) editor_delete(editor);
    else if (key.wVirtualKeyCode == VK_RETURN) editor_insert_newline(editor);
    else if (key.wVirtualKeyCode == VK_TAB) editor_insert_char(editor, '\t');
    else if (character >= 32 && character < 127) editor_insert_char(editor, character);
    return 1;
}

int main(int argc, char **argv) {
    Editor editor;
    editor_init(&editor);
    output_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    input_handle = GetStdHandle(STD_INPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO initial_info;
    GetConsoleScreenBufferInfo(output_handle, &initial_info);
    original_attributes = initial_info.wAttributes;
    GetConsoleMode(input_handle, &original_input_mode);
    SetConsoleMode(input_handle, ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT);
    SetConsoleTextAttribute(output_handle, BACKGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    if (argc > 1 && !file_open(&editor, argv[1])) editor_set_message(&editor, "Could not open %s", argv[1]);
    int running = 1;
    while (running) {
        CONSOLE_SCREEN_BUFFER_INFO info;
        GetConsoleScreenBufferInfo(output_handle, &info);
        render(&editor);
        INPUT_RECORD record;
        DWORD read;
        ReadConsoleInputA(input_handle, &record, 1, &read);
        if (record.EventType == KEY_EVENT && record.Event.KeyEvent.bKeyDown)
            running = handle_key(&editor, record.Event.KeyEvent, info.srWindow.Bottom - info.srWindow.Top - 2);
    }
    SetConsoleMode(input_handle, original_input_mode);
    SetConsoleTextAttribute(output_handle, original_attributes);
    editor_free(&editor);
    return 0;
}
