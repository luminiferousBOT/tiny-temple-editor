CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2
SOURCES = src/main.c src/editor.c src/file.c src/search.c src/syntax.c
TARGET = tte.exe

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $@ $(SOURCES)

clean:
	-del /Q $(TARGET) 2>NUL

.PHONY: all clean
