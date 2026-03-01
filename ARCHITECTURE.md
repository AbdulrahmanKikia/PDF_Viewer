# PDFViewer — Architecture

This document describes the architecture of the **read-only** PDF viewer (SimplePdfWindow + PdfTabWidget + Qt PDF). It is the primary code path when building with Qt PDF enabled.

---

## 1. Entry point and UI selection

- **`main.cpp`** creates `QApplication`, then the main window. When `PDFVIEWER_USE_QTPDF` is defined and Qt PDF is available, the main window is **SimplePdfWindow**; otherwise another window (e.g. MainWindow) may be used.
- **`SimplePdfWindow`** is a `QMainWindow` that owns:
  - A **QTabWidget** for tabs (first tab is the permanent Home tab; others are PDF tabs).
  - Toolbar: Open, Prev/Next page, page spinbox, zoom controls, search line edit, find next/prev.
  - Status bar: page X of Y, zoom %, search match info.
  - Menu bar: File, Edit, View, Tools, Help.

---

## 2. Tabs and PDF widget

- **Home tab** (`HomePageWidget`): fixed first tab; shows recent files and allows opening files. Cannot be closed.
- **PDF tabs** (`PdfTabWidget`): one per open document. Each has:
  - **QPdfDocument** — loads and holds the PDF.
  - **QPdfView** — displays it (SinglePage mode, so one page at a time).
  - **QPdfPageNavigator** — current page and jump(page, location, zoom).
  - **QPdfSearchModel** / **QPdfBookmarkModel** — search and TOC.
  - **QTreeView** — bookmarks (TOC) in a sidebar; visibility follows “Navigation Pane” setting.
  - **QSplitter** — TOC | PDF view.

- **PdfTabWidget** does **not** include any annotation overlay, annotation store, or page-order logic; it is view-only.

---

## 3. Zoom and viewport-center anchoring

- **View mode:** `QPdfView::PageMode::SinglePage` so the visible content is one page scaled by zoom. This allows a simple scroll-based “zoom to center”:
  - Content size ∝ zoom ⇒ same logical point stays at viewport center if we set scroll correctly after zoom.
- **Zoom entry points:**
  - Toolbar / View menu: Zoom In, Zoom Out, Fit Width, Fit Page, Reset Zoom.
  - **Ctrl+wheel** on the view (handled in `SimplePdfWindow::eventFilter`).
- **Implementation (`zoomAnchoredToViewportCenter`):**
  1. Read viewport center in content coordinates: `centerContent = scroll + viewport.center`.
  2. Compute zoom ratio: `newZoom / oldZoom`.
  3. Set zoom: `setZoomMode(Custom)`, `setZoomFactor(newZoom)`.
  4. Connect once to **QPdfView::zoomFactorChanged**; in the slot, schedule a 0ms timer so layout and scroll bar maximum are updated, then set:
     - `scrollX = centerContentX * zoomRatio - viewport.width/2`
     - `scrollY = centerContentY * zoomRatio - viewport.height/2`
     - clamped to scroll bar ranges.

- **Fit Width / Fit Page / Reset** still use the older **scheduleZoomAnchorJump(tab, page)** so the current page remains in view when switching zoom mode.

---

## 4. Session and settings

- **SessionManager** (singleton): writes/reads a JSON file (e.g. under `AppConfigLocation`) with:
  - List of tabs: type (home/pdf), file path, current page, zoom level, active tab index.
  - No annotations or page order; viewer is read-only.
- **SettingsManager** (singleton): loads/saves user preferences (theme, default zoom, show thumbnails, reopen tabs on startup, tab open behavior).
- **ThemeManager**: applies dark/light/system theme (QSS).
- On startup: load settings → apply theme/zoom/thumbnails → create Home tab → optionally restore session.
- On close: save session (tab list, page, zoom) and window geometry.

---

## 5. Tab open behavior

- **openFileWithTabBehavior(filePath)** implements:
  - **new** — always open in a new tab.
  - **replace** — replace current tab if it’s a PDF tab; else new tab.
  - **ask** — dialog: “Open in new tab” vs “Replace current tab”.
- Used for: File → Open, recent files, drag-and-drop, and CLI open.

---

## 6. Search

- **QPdfSearchModel** is bound to the current tab’s document. The toolbar search line edit sets the search string; Find Next/Previous change the current result index and call **navigator()->jump(page, location)** to show the hit. Match count is shown in the status bar.

---

## 7. Print

- When **Qt PrintSupport** is available, File → Print uses **QPrinter** and **QPrintDialog**. Pages are rendered with **QPdfDocument::render()** and drawn with **QPainter** into the printer. No annotations or overlay; print is the raw PDF pages.

---

## 8. Files and config

- **RecentFilesManager**: in-memory recent files list; persisted (e.g. in settings) and used by the Home tab and File → Open Recent.
- **Session file**: path from **SessionManager::sessionFilePath()**; format is JSON (tabs array, activeTabIndex).
- **Window geometry**: stored in **QSettings** (e.g. “window/geometry”, “window/state”).

---

## 9. What is not in this viewer

- No annotation overlay, AnnotationStore, or annotation tools.
- No Save As or Export as images.
- No Signature menu or signing/verification.
- No page reordering (no PageOrder, delete/duplicate/move page).
- No multi-page scroll mode in the primary viewer (SinglePage only for correct zoom-to-center).

---

## 10. Build variants

- **PDFVIEWER_USE_QTPDF=ON** (default when Qt PDF is found): builds SimplePdfWindow, PdfTabWidget, and links Qt6::Pdf, Qt6::PdfWidgets.
- **PDFVIEWER_USE_MUPDF=ON**: adds MuPDF backend; see `third_party/mupdf` and CMake options.
- **PDFVIEWER_ENABLE_QTPDF=OFF**: disables Qt PDF UI; useful for a shell or other backends only.

The codebase also contains an alternate UI (MainWindow, TabManager, PdfViewerWidget, PageCanvasView, etc.) used in different build/configuration scenarios; the **documented, supported path** for the “viewer only” project is the Qt PDF + SimplePdfWindow + PdfTabWidget path described above.
