#ifndef TTE_SYNTAX_H
#define TTE_SYNTAX_H

#include <stddef.h>

typedef enum { COLOR_TEXT, COLOR_KEYWORD, COLOR_TYPE, COLOR_NUMBER, COLOR_STRING, COLOR_COMMENT, COLOR_DIRECTIVE } SyntaxColor;
SyntaxColor syntax_color_at(const char *text, size_t length, size_t position, int holy_c);

#endif
