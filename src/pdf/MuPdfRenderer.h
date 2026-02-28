// MuPdfRenderer.h
// Primary PDF rendering backend using the MuPDF C library.
// Compiled only when PDFVIEWER_USE_MUPDF is defined by CMake.
//
// MuPDF licensing: AGPL-3.0 for open-source use; commercial licence from Artifex.
// All MuPDF headers and libraries live under third_party/mupdf/ so the
// dependency can be swapped or removed without touching any UI code.
//
// Threading model:
//   fz_context is NOT thread-safe. This class holds one "master" context that
//   is only used to clone child contexts. Each call to renderPage() or
//   pageSizePoints() that runs on a worker thread gets its own cloned context,
//   so concurrent rendering from multiple threads is safe.

#ifndef MUPDFRENDERER_H
#define MUPDFRENDERER_H

#ifdef PDFVIEWER_USE_MUPDF

#include "IPdfRenderer.h"
#include <QMutex>

// Forward-declare MuPDF C types to avoid pulling fitz.h into every
// translation unit that includes this header.
struct fz_context;
struct fz_document;

class MuPdfRenderer final : public IPdfRenderer
{
public:
    MuPdfRenderer();
    ~MuPdfRenderer() override;

    // Not copyable or movable (owns raw MuPDF pointers).
    MuPdfRenderer(const MuPdfRenderer&)            = delete;
    MuPdfRenderer& operator=(const MuPdfRenderer&) = delete;

    bool    load(const QString& filePath)                                                  override;
    void    close()                                                                         override;
    int     pageCount()                                                              const  override;
    QSizeF  pageSizePoints(int pageIndex)                                            const  override;
    QImage  renderPage(int pageIndex, QSize targetPx, float zoom, int rotationDeg)         override;
    QString lastError()                                                              const  override;
    QString backendName()                                                    const override { return QStringLiteral("MuPDF"); }

private:
    // Master context – only used to clone into worker contexts; never used
    // directly for document operations.
    fz_context*  m_ctx       = nullptr;

    // Document and page count – guarded by m_docMutex.
    fz_document* m_doc       = nullptr;
    int          m_pageCount = 0;

    // Protects m_doc / m_pageCount during load/close/snapshot.
    mutable QMutex  m_docMutex;

    // Stores the most recent error; written under m_docMutex.
    mutable QString m_lastError;

    // Release m_doc (must be called with m_docMutex held or from destructor).
    void releaseDocument();
};

#endif // PDFVIEWER_USE_MUPDF
#endif // MUPDFRENDERER_H
