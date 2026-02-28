# PDFViewer

A lightweight, cross-platform PDF viewer built with **C++17** and **Qt 6**. Uses Qt's built-in PDF module for rendering — no external dependencies required beyond Qt itself.

Works on **Windows**, **macOS**, and **Linux**.

## Features

- **Multi-tab interface** — open many PDFs at once, each in an independent tab with its own page, zoom, and search state
- **Fast PDF rendering** via QtPdf (Chromium-based engine shipped with Qt)
- **Smooth scrolling** in continuous multi-page mode
- **Ctrl+scroll / trackpad zoom**, plus toolbar buttons for Zoom In, Zoom Out, Fit Width, Fit Page, and Reset
- **Table of Contents** sidebar with single-click navigation
- **Text search** with Next / Previous result highlighting and match count in the status bar
- **Page navigation** via Prev/Next buttons, page number spinbox, or keyboard (PgUp/PgDn, Home/End)
- **Full keyboard shortcut set** — see [Keyboard Shortcuts](#keyboard-shortcuts) below
- **Drag-and-drop** to open PDF files (each dropped file opens in a new tab)
- **Recent files** menu (File → Open Recent)
- **Session restore** — reopens all tabs from the previous session on startup
- **Window geometry persistence** — remembers size and position between sessions
- **CLI open** — pass a PDF path as the first argument to open it directly
- **Live page counter** and zoom percentage in the status bar
- **Dark theme** applied by default; switch to Light via the View menu

## Screenshots

<!-- Add screenshots here -->

## Keyboard Shortcuts

| Shortcut | Action |
|---|---|
| Ctrl+O | Open file |
| Ctrl+T | New tab |
| Ctrl+W | Close tab |
| Ctrl+Tab | Next tab |
| Ctrl+Shift+Tab | Previous tab |
| Ctrl+1..8 | Jump to tab N |
| Ctrl+9 | Jump to last tab |
| Ctrl+F | Focus search |
| Ctrl++ / Ctrl+- | Zoom in / out |
| Ctrl+0 | Reset zoom (100%) |
| PgUp / PgDn | Previous / next page |
| Home / End | First / last page |
| F11 | Toggle full screen |
| Ctrl+R | Reload document |
| Ctrl+Q | Exit |
| Escape | Clear search / exit full screen |

## Requirements

| Component | Version |
|-----------|---------|
| Qt | 6.5+ (tested with 6.10.2) |
| Qt modules | Core, Gui, Widgets, Concurrent, Pdf, PdfWidgets |
| CMake | 3.20+ |
| Compiler | MSVC 2022/2019 (Windows), Clang (macOS), or GCC/Clang (Linux) |

### Installing Qt and Qt PDF

**Option A — Qt Installer (all platforms)**

1. Download the [Qt Online Installer](https://www.qt.io/download-qt-installer)
2. Install Qt 6.5+ for your platform (e.g. `macOS`, `MSVC 2022 64-bit`, or `Desktop gcc 64-bit`)
3. Under **Additional Libraries**, check **Qt PDF**

On Windows, run `check_qt6_pdf.bat` to verify the module is detected.

**Option B — Homebrew (macOS only)**

```bash
brew install qt
```

> Note: The Homebrew Qt formula may not include the PDF module. If CMake reports
> `Qt6::Pdf not found`, use Option A instead.

**Option C — System package manager (Linux)**

```bash
# Ubuntu / Debian
sudo apt install qt6-base-dev qt6-pdf-dev libqt6pdfwidgets6

# Fedora
sudo dnf install qt6-qtbase-devel qt6-qtpdf-devel
```

## Building

### Windows (one-click)

```
rebuild.bat
```

This automatically sets up the MSVC environment, configures CMake, and builds a Release binary. The executable is placed at:

```
build-msvc\Release\PDFViewer.exe
```

### macOS

```bash
# Configure (adjust the path to match your Qt installation)
cmake -S . -B build -DCMAKE_PREFIX_PATH=~/Qt/6.10.2/macos

# Build
cmake --build build --config Release --parallel

# Run
./build/PDFViewer.app/Contents/MacOS/PDFViewer
```

If you installed Qt via Homebrew:

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"
cmake --build build --config Release --parallel
```

### Linux

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x.x/gcc_64
cmake --build build --config Release --parallel
./build/PDFViewer
```

### Windows (manual)

```bash
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 ^
  -DCMAKE_PREFIX_PATH="C:/Qt/6.10.2/msvc2022_64"
cmake --build build --config Release --parallel
```

### CMake options

| Option | Default | Description |
|--------|---------|-------------|
| `PDFVIEWER_USE_QTPDF` | `ON` | Use QtPdf for rendering and the SimplePdfWindow UI |
| `PDFVIEWER_USE_MUPDF` | `OFF` | Use MuPDF as the primary rendering backend |
| `PDFVIEWER_ENABLE_DEBUGLOG` | `OFF` | Write debug logs to `AppDataLocation/debug.log` |

### Opening a file from the command line

```bash
PDFViewer.exe path/to/document.pdf
```

## Project structure

```
src/
  main.cpp                  Entry point
  application.cpp           QApplication subclass
  config/
    theme.cpp               Dark / light theme management
    debuglog.cpp            Optional NDJSON debug logging
    sessionmanager.cpp      Session persistence (save/restore all tabs on exit/startup)
    recentfilesmanager.cpp  Recent files tracking
    settingsmanager.cpp     User preferences
  ui/
    simplepdfwindow.cpp     Main window with tab bar and all UI logic
    pdftabwidget.cpp        Self-contained per-tab PDF viewer widget
resources/
  resources.qrc             Icons and assets
```

## License

<!-- Add your license here (e.g. MIT, GPL, etc.) -->

## Acknowledgements

- [Qt](https://www.qt.io/) — application framework and PDF rendering engine
- [Qt PDF module](https://doc.qt.io/qt-6/qtpdf-index.html) — Chromium-based PDF backend
