// PdfRenderService.h
// Asynchronous PDF rendering service (Milestone B).
//
// Usage pattern:
//   1. Construct with the best available IPdfRenderer from PdfRendererFactory.
//   2. Call open(filePath) — fast, blocking load.
//   3. Connect to pageRendered() and renderError() with Qt::QueuedConnection.
//   4. Call requestRender(key) from the UI thread whenever you need an image.
//      - Cache hit:  pageRendered() fires immediately on the caller's thread.
//      - Cache miss: a RenderTask is queued on QThreadPool::globalInstance();
//                    pageRendered() fires on the worker thread when done.
//   5. Call cancelPending() or close() to stop work in progress.
//
// Duplicate coalescing:
//   If a RenderKey is already queued (but not yet executing), a second
//   requestRender() for the same key is silently dropped.

#ifndef PDFRENDERSERVICE_H
#define PDFRENDERSERVICE_H

#include "IPdfRenderer.h"
#include "PageRenderCache.h"

#include <QObject>
#include <QImage>
#include <QMutex>
#include <QSet>

#include <memory>

class QThreadPool;

class PdfRenderService : public QObject
{
    Q_OBJECT

public:
    // Takes ownership of the renderer.
    explicit PdfRenderService(std::unique_ptr<IPdfRenderer> renderer,
                              QObject* parent = nullptr);
    ~PdfRenderService() override;

    // Open a PDF document. Blocks while MuPDF parses the header (usually <1 ms
    // for cached files). Returns false on failure; see lastError().
    bool open(const QString& filePath);

    // Close the document and evict its entries from the global cache.
    void close();

    bool    isOpen()                         const;
    int     pageCount()                      const;
    QSizeF  pageSizePoints(int pageIndex)    const;
    QString filePath()                       const { return m_filePath; }
    QString lastError()                      const;
    QString backendName()                    const;

    // Submit a render request.
    // - Cache hit  → emits pageRendered() synchronously on the caller's thread.
    // - Cache miss → schedules a background task; pageRendered() fires on the
    //                worker thread (connect with Qt::QueuedConnection for UI).
    void requestRender(const RenderKey& key);

    // Drop all pending (not-yet-started) render tasks.
    // In-flight tasks complete normally but their results are discarded.
    void cancelPending();

    // Shared cache for all documents in the process.
    static PageRenderCache& globalCache();

signals:
    void pageRendered(const RenderKey& key, const QImage& image);
    void renderError(const RenderKey& key,  const QString& message);

private:
    void submitToThreadPool(const RenderKey& key);

    // Called by RenderTask when a page finishes rendering.
    void onWorkerDone(const RenderKey& key, const QImage& image, const QString& error);

    std::unique_ptr<IPdfRenderer> m_renderer;
    QString     m_filePath;

    QMutex          m_inflightMutex;
    QSet<RenderKey> m_inFlight;   // Keys currently queued or executing

    QThreadPool* m_pool = nullptr;  // Borrowed; never deleted here.

    friend class RenderTask;
};

#endif // PDFRENDERSERVICE_H
