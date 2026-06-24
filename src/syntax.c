#include "syntax.h"

#include <ctype.h>
#include <string.h>

static int word_at(const char *text, size_t length, size_t position, const char *word) {
    size_t word_length = strlen(word);
    return position + word_length <= length && !strncmp(text + position, word, word_length) &&
           (position == 0 || !isalnum((unsigned char)text[position - 1])) &&
           (position + word_length == length || !isalnum((unsigned char)text[position + word_length]));
}

SyntaxColor syntax_color_at(const char *text, size_t length, size_t position, int holy_c) {
    if (!holy_c) return COLOR_TEXT;
    if (position == 0 && text[position] == '#') return COLOR_DIRECTIVE;
    for (size_t i = 0; i < position; i++) {
        if (text[i] == '/' && i + 1 < length && text[i + 1] == '/') return COLOR_COMMENT;
        if (text[i] == '"' && (i == 0 || text[i - 1] != '\\')) return COLOR_STRING;
        if (text[i] == '\'' && (i == 0 || text[i - 1] != '\\')) return COLOR_STRING;
    }
    if (text[position] == '"' || text[position] == '\'') return COLOR_STRING;
    if (isdigit((unsigned char)text[position])) return COLOR_NUMBER;
    const char *types[] = {"U0", "U8", "U16", "U32", "U64", "I8", "I16", "I32", "I64", "Bool", NULL};
    const char *keywords[] = {"if", "else", "for", "while", "do", "switch", "case", "break", "continue", "return", "class", "union", "public", NULL};
    for (int i = 0; types[i]; i++) if (word_at(text, length, position, types[i])) return COLOR_TYPE;
    for (int i = 0; keywords[i]; i++) if (word_at(text, length, position, keywords[i])) return COLOR_KEYWORD;
    return COLOR_TEXT;
}
