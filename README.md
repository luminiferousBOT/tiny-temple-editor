# Tiny Temple Editor

Tiny Temple Editor is a small Windows terminal text editor with a bright, high-contrast style inspired by the spirit of TempleOS.

It is made as a respectful tribute to Terry A. Davis and his singular, imaginative work on TempleOS. This is an original, independent project and is not affiliated with TempleOS.

Open a file, edit it, save it, search through it, and work with comfortable line numbers. Files ending in `.HC` receive simple HolyC syntax highlighting for keywords, types, strings, numbers, comments, and directives.

Build it with MinGW:

```powershell
mingw32-make
.\tte.exe .\samples\hello.HC
```

Useful keys: `Ctrl+S` to save, `Ctrl+F` to search, and `Ctrl+Q` to quit.
