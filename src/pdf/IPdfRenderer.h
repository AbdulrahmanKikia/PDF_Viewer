// IPdfRenderer.h
// Abstract interface for PDF page rendering backends.
//
// Design goals:
//   - Backend-agnostic: UI code only talks to IPdfRenderer*.
//   - One instance per open document (not a singleton).
//   - Thread-safe to call from worker threads; each implementation documents
//     its threading model.
//
// Backend selection (compile-time flags set by CMake):
//   PDFVIEWER_USE_MUPDF  – MuPdfRenderer     (primary, fastest)
//   PDFVIEWER_USE_QTPDF  – QtPdfRenderer     (fallback, needs Qt6::Pdf)
//   Neither              – NullPdfRenderer   (placeholder, always compiles)
//
// Use PdfRendererFactory::create() to obtain the best available backend.

#ifndef IPDFRENDERER_H
#define IPDFRENDERER_H

#include <QImage>
#include <QSize>
#include <QSizeF>
#include <QString>
#include <QtGlobal>   // qFuzzyCompare, uint

// ---------------------------------------------------------------------------
// RenderQuality
// ---------------------------------------------------------------------------
enum class RenderQuality {
    Fast,      // Minimal anti-aliasing; best for quick scrolling
    Balanced,  // Default – good quality at reasonable cost
    Best       // Full AA + sub-pixel rendering; slowest
};

// ---------------------------------------------------------------------------
// RenderKey — unique, hashable cache key for one rendered page image.
//
// rotationDeg must be 0, 90, 180, or 270; other values are treated as 0.
// targetW/targetH of 0 means "no constraint" (zoom drives size entirely).
// ---------------------------------------------------------------------------
struct RenderKey {
    QString       filePath;
    int           pageIndex   = 0;
    float         zoom        = 1.0f;
    int           targetW     = 0;
    int           targetH     = 0;
    int           rotationDeg = 0;   // 0 | 90 | 180 | 270
    RenderQuality quality     = RenderQuality::Balanced;

    bool operator==(const RenderKey& o) const noexcept {
        return filePath     == o.filePath
            && pageIndex    == o.pageIndex
            && qFuzzyCompare(zoom, o.zoom)
            && targetW      == o.targetW
            && targetH      == o.targetH
            && rotationDeg  == o.rotationDeg
            && quality      == o.quality;
    }
    bool operator!=(const RenderKey& o) const noexcept { return !(*this == o); }
};

inline size_t qHash(const RenderKey& k, size_t seed = 0) noexcept
{
    size_t h = qHash(k.filePath,    seed);
    h ^= qHash(k.pageIndex,         seed) * 2654435761u;
    h ^= qHash(static_cast<int>(k.zoom * 1000.f), seed) * 40503u;
    h ^= qHash(k.targetW,           seed) * 2246822519u;
    h ^= qHash(k.targetH,           seed) * 3266489917u;
    h ^= qHash(k.rotationDeg,       seed) * 668265263u;
    h ^= qHash(static_cast<int>(k.quality), seed) * 374761393u;
    return h;
}

// ---------------------------------------------------------------------------
// IPdfRenderer
// ---------------------------------------------------------------------------
class IPdfRenderer
{
public:
    virtual ~IPdfRenderer() = default;

    // Open a PDF file. Returns true on success.
    // Safe to call from any thread.
    virtual bool load(const QString& filePath) = 0;

    // Release resources for the currently loaded document.
    virtual void close() = 0;

    // Number of pages in the loaded document (0 when nothing is loaded).
    virtual int pageCount() const = 0;

    // Page size in PDF points (1 pt = 1/72 inch).
    // Returns QSizeF() if pageIndex is out of range or no document is loaded.
    virtual QSizeF pageSizePoints(int pageIndex) const = 0;

    // Render page `pageIndex` and return a QImage.
    //
    // Parameters:
    //   pageIndex   – 0-based
    //   targetPx    – maximum pixel dimensions; 0 on either axis = unconstrained
    //   zoom        – scale factor applied on top of the 72-dpi point grid
    //   rotationDeg – clockwise rotation: 0, 90, 180, or 270
    //
    // The returned image is a deep-copied QImage (owns its buffer).
    // Returns a null QImage on failure; call lastError() for details.
    // Safe to call from any thread.
    virtual QImage renderPage(int           pageIndex,
                              QSize         targetPx,
                              float         zoom,
                              int           rotationDeg) = 0;

    // Human-readable description of the most recent error (empty if none).
    virtual QString lastError() const = 0;

    // Short name of this backend, e.g. "MuPDF", "Qt PDF", "Null".
    virtual QString backendName() const = 0;
};

#endif // IPDFRENDERER_H
