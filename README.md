# PDFViewer

A lightweight, native PDF viewer built with **C++17** and **Qt 6**. Uses Qt's built-in PDF module for rendering -- no external dependencies required beyond Qt itself.

## Features

- **Fast PDF rendering** via QtPdf (Chromium-based engine shipped with Qt)
- **Smooth scrolling** in continuous multi-page mode
- **Ctrl+scroll zoom** (trackpad / mouse wheel), plus toolbar buttons for Zoom In, Zoom Out, Fit Width, Fit Page, and Reset
- **Table of Contents** sidebar with single-click navigation
- **Text search** with Next / Previous result highlighting
- **Page navigation** via Prev/Next buttons, page number spinbox, or scrolling
- **Live page counter** and zoom percentage in the status bar, always in sync
- **Dark theme** applied by default

## Screenshots

<!-- Add screenshots here -->

## Requirements

| Component | Version |
|-----------|---------|
| Qt | 6.5+ (tested with 6.10.2) |
| Qt modules | Core, Gui, Widgets, Concurrent, Pdf, PdfWidgets |
| CMake | 3.20+ |
| Compiler | MSVC 2022 / 2019 (Windows) or GCC / Clang with C++17 support |

### Installing Qt PDF

Qt PDF is an optional module. Install it via **Qt Maintenance Tool**:

1. Open Qt Maintenance Tool
2. Select **Add or remove components**
3. Under your Qt version (e.g. Qt 6.10.2), expand **Additional Libraries**
4. Check **Qt PDF** and complete the installation

Run `check_qt6_pdf.bat` (Windows) to verify the module is detected.

## Building

### Windows (one-click)

```
rebuild.bat
```

This automatically sets up the MSVC environment, configures CMake, and builds a Release binary. The executable is placed at:

```
build-msvc\Release\PDFViewer.exe
```

### Manual build

```bash
# Configure
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 \
  -DCMAKE_PREFIX_PATH="C:/Qt/6.10.2/msvc2022_64"

# Build
cmake --build build --config Release --parallel
```

### Linux / macOS

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x.x/gcc_64
cmake --build build --config Release --parallel
```

### CMake options

| Option | Default | Description |
|--------|---------|-------------|
| `PDFVIEWER_ENABLE_QTPDF` | `ON` | Use QtPdf for rendering and the SimplePdfWindow UI |
| `PDFVIEWER_USE_MUPDF` | `OFF` | Use MuPDF as the primary rendering backend |
| `PDFVIEWER_ENABLE_DEBUGLOG` | `OFF` | Write debug logs to `AppDataLocation/debug.log` |

## Project structure

```
src/
  main.cpp                  Entry point
  application.cpp           QApplication subclass
  config/
    theme.cpp               Dark / light theme management
    debuglog.cpp             Optional NDJSON debug logging
    sessionmanager.cpp       Session persistence
    recentfilesmanager.cpp   Recent files tracking
    settingsmanager.cpp      User preferences
  ui/
    simplepdfwindow.cpp      QtPdf-based viewer (current default)
    mainwindow.cpp           Full tabbed UI (legacy, used without QtPdf)
  pdf/
    IPdfRenderer.h           Renderer interface
    QtPdfRenderer.cpp        QtPdf backend
    PdfRendererFactory.cpp   Backend selection
resources/
  resources.qrc             Icons and assets
```

## License

<!-- Add your license here (e.g. MIT, GPL, etc.) -->

## Acknowledgements

- [Qt](https://www.qt.io/) -- application framework and PDF rendering engine
- [Qt PDF module](https://doc.qt.io/qt-6/qtpdf-index.html) -- Chromium-based PDF backend
