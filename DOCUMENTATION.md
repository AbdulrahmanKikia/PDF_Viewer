# PDFViewer — Documentation Index

**Project:** Read-only PDF viewer (C++17, Qt 6).  
**Status:** Done. No editing, annotations, signatures, or export.

---

## Main docs

| Document | Purpose |
|----------|---------|
| **[README.md](README.md)** | Overview, features, what’s in/out of scope, build/run, shortcuts, project structure. |
| **[ARCHITECTURE.md](ARCHITECTURE.md)** | Technical design: entry point, tabs, zoom-to-center, session, settings, and what is not included. |
| **[KNOWN_ISSUES.md](KNOWN_ISSUES.md)** | Known issues, zoom notes, why annotations were removed, build/environment notes. |

---

## Quick reference

- **Build (Windows):** `rebuild.bat` → `build-msvc\Release\PDFViewer.exe`
- **Run with file:** `PDFViewer.exe path\to\file.pdf`
- **Zoom:** Toolbar / View menu / Ctrl+wheel; center of the screen stays fixed when zooming.
- **View:** Single page per tab; use Prev/Next or TOC to change page.

---

## Version

Project version is set in **CMakeLists.txt** (e.g. `project(PDFViewer VERSION 1.2.0 ...)`).
