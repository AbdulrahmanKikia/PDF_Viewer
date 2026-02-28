## PDFViewer Security Notes

### Supported / Tested Dependencies

- **Qt**: Project is currently configured for **Qt 6.10.2** (see `CMakeLists.txt`).
- **PDF backends**:
  - **MuPDF** (primary backend, optional): statically linked when `PDFVIEWER_USE_MUPDF=ON`.
  - **QtPdf** (fallback backend, optional): used when `Qt6::Pdf` is available and `PDFVIEWER_ENABLE_QTPDF=ON`.
  - **Null backend**: always available; shows a placeholder page when no real backend is compiled in.

Keep Qt and MuPDF patched to the latest compatible versions and monitor CVEs for both libraries, since they parse untrusted PDF content.

### Build-Time Hardening Switches

The following CMake options control which PDF backends are compiled in:

- `PDFVIEWER_USE_MUPDF` (default: `OFF`)
  - `ON`: enable MuPDF primary backend.
  - `OFF`: never link MuPDF; the application will use QtPdf (if enabled) or the Null backend only.

- `PDFVIEWER_ENABLE_QTPDF` (default: `ON`)
  - `ON`: allow using `QtPdfRenderer` when `Qt6::Pdf` is found.
  - `OFF`: never compile or link `QtPdfRenderer`, even if `Qt6::Pdf` is installed.

Recommended hardening profiles:

- **UI-only / safest shell (no PDF parsing)**  
  `-DPDFVIEWER_USE_MUPDF=OFF -DPDFVIEWER_ENABLE_QTPDF=OFF`

- **MuPDF only**  
  `-DPDFVIEWER_USE_MUPDF=ON  -DPDFVIEWER_ENABLE_QTPDF=OFF`

- **QtPdf only**  
  `-DPDFVIEWER_USE_MUPDF=OFF -DPDFVIEWER_ENABLE_QTPDF=ON`

### Future: Out-of-Process Rendering

MuPDF and QtPdf are currently used in-process. For stronger isolation in high-risk environments:

- Introduce a small helper executable (e.g. `PDFViewerRenderWorker`) that:
  - Accepts a PDF path and render parameters over a local IPC channel.
  - Returns rendered page bitmaps or error codes.
- Run the helper inside an OS sandbox or restricted account.
- Keep the main GUI process free of direct PDF parsing.

This architecture change is not yet implemented but the interfaces (`IPdfRenderer`, `PdfRenderService`) are designed so they can be backed by an out-of-process implementation later.

