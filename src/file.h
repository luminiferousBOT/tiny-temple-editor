#ifndef TTE_FILE_H
#define TTE_FILE_H

#include "editor.h"

int file_open(Editor *editor, const char *path);
int file_save(Editor *editor, const char *path);
int file_is_holyc(const char *path);

#endif
