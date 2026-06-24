#include "search.h"

#include <string.h>

int search_find(Editor *editor, const char *query) {
    if (!query[0]) return 0;
    for (size_t offset = 0; offset < editor->line_count; offset++) {
        size_t row = (editor->cursor_y + offset) % editor->line_count;
        char *match = strstr(editor->lines[row].text, query);
        if (match) {
            editor->cursor_y = row;
            editor->cursor_x = (size_t)(match - editor->lines[row].text);
            return 1;
        }
    }
    return 0;
}
