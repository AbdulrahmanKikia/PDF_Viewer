# PDFViewer

A **read-only** PDF viewer built with **C++17** and **Qt 6**. Uses Qt’s PDF module (QtPdf) for rendering. No PDF editing, annotations, or signatures — viewing only.

**Platforms:** Windows, macOS, Linux.

---

## What this project is

- **View** PDFs: open, scroll, zoom, search, print.
- **Multi-tab** interface with session restore and recent files.
- **Settings**: theme (dark/light/system), default zoom, thumbnail sidebar, reopen tabs on startup, tab open behavior (new / replace / ask).
- **Zoom** keeps the **center of the viewport** fixed (zoom in/out from the middle of the screen).

## What this project is not

- **No PDF editing** — no annotations, highlights, comments, drawing, or eraser.
- **No Save As** or **Export as images** — files are opened read-only.
- **No signature** UI — no signing, verifying, or certificate management.
- **No page reordering** — no delete/duplicate/move pages.

---

## Features

| Feature | Description |
|--------|-------------|
| **Tabs** | Multiple PDFs open at once; each tab has its own page, zoom, and search state. |
| **Home tab** | Permanent first tab with recent files and file browser. |
| **Rendering** | QtPdf (Chromium-based engine). |
| **View mode** | Single-page: one page at a time; zoom is anchored to viewport center. |
| **Zoom** | Toolbar and View menu: Zoom In, Zoom Out, Fit Width, Fit Page, Reset. **Ctrl+scroll** zooms with the center of the screen fixed. |
| **Navigation** | Prev/Next, page spinbox, PgUp/PgDn, Home/End. Table-of-contents sidebar for bookmarks. |
| **Search** | Text search with Next/Previous and match count in the status bar. |
| **Print** | File → Print (when Qt PrintSupport is available). |
| **Session** | Saves open tabs, current page, and zoom; restores on next launch (if “Reopen tabs on startup” is enabled). |
| **Settings** | Preferences dialog: theme, default zoom, show thumbnails, reopen tabs, tab open behavior. |
| **Recent files** | File → Open Recent. |
| **Drag-and-drop** | Drop PDFs to open (respects tab open behavior). |
| **CLI** | `PDFViewer.exe path/to/file.pdf` opens the file on startup. |

---

## Keyboard shortcuts

| Shortcut | Action |
|----------|--------|
| Ctrl+O | Open file |
| Ctrl+T | New tab |
| Ctrl+W | Close tab |
| Ctrl+Tab | Next tab |
| Ctrl+Shift+Tab | Previous tab |
| Ctrl+1..8 | Jump to tab 1–8 |
| Ctrl+9 | Last tab |
| Ctrl+F | Focus search |
| Ctrl++ / Ctrl+- | Zoom in / out |
| Ctrl+0 | Reset zoom (100%) |
| PgUp / PgDn | Previous / next page |
| Home / End | First / last page |
| F11 | Full screen |
| Ctrl+R | Reload document |
| Ctrl+, | Preferences |
| Ctrl+Q | Quit |
| Escape | Clear search / exit full screen |

---

## Requirements

| Component | Version |
|-----------|---------|
| Qt | 6.5+ (tested with 6.10.2) |
| Qt modules | Core, Gui, Widgets, Concurrent, **Pdf**, **PdfWidgets**; optional: Svg, PrintSupport |
| CMake | 3.20+ |
| Compiler | MSVC 2022/2019 (Windows), Clang (macOS), GCC/Clang (Linux) |

### Installing Qt and Qt PDF

- **Qt Installer:** Install Qt 6.x for your platform; in Additional Libraries, select **Qt PDF**.
- **Windows:** Run `check_qt6_pdf.bat` to verify Qt PDF is found.
- **Linux (apt):** `sudo apt install qt6-base-dev qt6-pdf-dev libqt6pdfwidgets6`
- **Linux (dnf):** `sudo dnf install qt6-qtbase-devel qt6-qtpdf-devel`

---

## Building

### Windows (quick)

```bat
rebuild.bat
```

Output: `build-msvc\Release\PDFViewer.exe`

### Windows (manual)

```bat
cmake -S . -B build-msvc -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:/Qt/6.10.2/msvc2022_64"
cmake --build build-msvc --config Release
```

### macOS

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=~/Qt/6.10.2/macos
cmake --build build --config Release
./build/PDFViewer.app/Contents/MacOS/PDFViewer
```

### Linux

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/gcc_64
cmake --build build --config Release
./build/PDFViewer
```

### CMake options

| Option | Default | Description |
|--------|---------|-------------|
| `PDFVIEWER_ENABLE_QTPDF` | ON | Build QtPdf viewer (SimplePdfWindow). |
| `PDFVIEWER_USE_MUPDF` | OFF | Use MuPDF backend (see `third_party/mupdf`). |
| `PDFVIEWER_ENABLE_DEBUGLOG` | OFF | Write debug logs under AppDataLocation. |

---

## Running

- **GUI:** Start the app; use File → Open or the Home tab to open PDFs.
- **CLI:** `PDFViewer path/to/document.pdf` opens that file in a new tab (or per tab-open setting).

---

## Project structure (Qt PDF path)

When built with Qt PDF (`PDFVIEWER_USE_QTPDF=ON`), the main UI is the “simple” viewer:

```
src/
  main.cpp                 Entry point; launches SimplePdfWindow or MainWindow
  application.cpp/h        QApplication setup
  config/
    theme.cpp/h            Dark / light / system theme
    sessionmanager.cpp/h   Save/restore tabs (paths, page, zoom) to JSON
    recentfilesmanager.*   Recent files list
    settingsmanager.*     App settings (theme, zoom, thumbnails, tab behavior)
    debuglog.*             Optional file logging
  ui/
    simplepdfwindow.cpp/h  Main window: menu bar, toolbar, tabs, zoom, search, print
    pdftabwidget.cpp/h     Per-tab widget: QPdfView, TOC, one PDF per tab
    homepagewidget.*      Home tab (recent files, open UI)
    settingsdialog.*      Preferences dialog
  resources/               Icons, styles (dark/light QSS)
```

**Other UI** (e.g. `mainwindow`, `tabmanager`, `PdfViewerWidget`, `PageCanvasView`) belongs to an alternate, more complex UI that can be built when not using the simple viewer; the **primary supported flow** is `main.cpp` → `SimplePdfWindow` → `PdfTabWidget` with QtPdf.

---

## Architecture notes

- **Single-page view:** The viewer uses `QPdfView::PageMode::SinglePage` so that zoom uses a single scaled page and scroll math keeps the **viewport center** fixed when zooming (toolbar, menu, Ctrl+wheel).
- **Zoom-to-center:** Before zoom, the center of the viewport in content coordinates is stored; after `setZoomFactor`, a one-shot connected to `zoomFactorChanged` applies scroll so that the same logical point stays at the center.
- **Session:** On close, open tab paths, current page, and zoom are written to a JSON file under the app config directory; on startup, if “Reopen tabs on startup” is enabled, those tabs are reopened and restored.
- **Settings:** Stored via `SettingsManager`; applied at startup and when the user clicks OK in the Preferences dialog.

---

## Known issues and notes

See **[KNOWN_ISSUES.md](KNOWN_ISSUES.md)** for details. Summary:

| Area | Note |
|------|------|
| **Zoom** | Zoom-to-center requires SinglePage mode. In MultiPage (stacked pages with gaps) the scroll math does not hold, so we use one page at a time. Zoom-to-center may still need tuning in edge cases. |
| **Annotations** | Removed. An overlay broke text selection and caused drifting/superposition; the viewer is read-only. |
| **Save/Export, Signatures** | Removed by design. |

---

## License

See repository or add your chosen license (e.g. MIT, GPL).

---

## Acknowledgements

- [Qt](https://www.qt.io/) — application framework
- [Qt PDF](https://doc.qt.io/qt-6/qtpdf-index.html) — PDF rendering (Chromium-based)
