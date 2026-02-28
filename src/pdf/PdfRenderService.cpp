// PdfRenderService.cpp
#include "PdfRenderService.h"
#include "PdfRendererFactory.h"

#include <QRunnable>
#include <QThreadPool>
#include <QMutexLocker>

// ---------------------------------------------------------------------------
// Process-wide shared cache (200 MB default; see PageRenderCache)
// ---------------------------------------------------------------------------
static PageRenderCache g_cache(200LL * 1024 * 1024);

PageRenderCache& PdfRenderService::globalCache()
{
    return g_cache;
}

// ---------------------------------------------------------------------------
// RenderTask  – QRunnable submitted to QThreadPool
// ---------------------------------------------------------------------------
class RenderTask : public QRunnable
{
public:
    RenderTask(PdfRenderService* svc,
               IPdfRenderer*     renderer,
               const RenderKey&  key)
        : m_svc(svc), m_renderer(renderer), m_key(key)
    {
        setAutoDelete(true);
    }

    void run() override
    {
        QImage  img = m_renderer->renderPage(
            m_key.pageIndex,
            QSize(m_key.targetW, m_key.targetH),
            m_key.zoom,
            m_key.rotationDeg);

        QString err;
        if (img.isNull())
            err = m_renderer->lastError();

        m_svc->onWorkerDone(m_key, img, err);
    }

private:
    PdfRenderService* m_svc;      // Not owned
    IPdfRenderer*     m_renderer; // Not owned; owned by m_svc
    RenderKey         m_key;
};

// ---------------------------------------------------------------------------
// PdfRenderService
// ---------------------------------------------------------------------------
PdfRenderService::PdfRenderService(std::unique_ptr<IPdfRenderer> renderer,
                                   QObject* parent)
    : QObject(parent)
    , m_renderer(std::move(renderer))
    , m_pool(QThreadPool::globalInstance())
{}

PdfRenderService::~PdfRenderService()
{
    close();
}

bool PdfRenderService::open(const QString& filePath)
{
    m_filePath = filePath;
    if (!m_renderer->load(filePath)) {
        m_filePath.clear();
        return false;
    }
    return true;
}

void PdfRenderService::close()
{
    cancelPending();
    if (!m_filePath.isEmpty()) {
        g_cache.evictFile(m_filePath);
        m_renderer->close();
        m_filePath.clear();
    }
}

bool PdfRenderService::isOpen() const
{
    return m_renderer && m_renderer->pageCount() > 0;
}

int PdfRenderService::pageCount() const
{
    return m_renderer ? m_renderer->pageCount() : 0;
}

QSizeF PdfRenderService::pageSizePoints(int pageIndex) const
{
    return m_renderer ? m_renderer->pageSizePoints(pageIndex) : QSizeF();
}

QString PdfRenderService::lastError() const
{
    return m_renderer ? m_renderer->lastError()
                      : QStringLiteral("No renderer available.");
}

QString PdfRenderService::backendName() const
{
    return m_renderer ? m_renderer->backendName() : QStringLiteral("None");
}

void PdfRenderService::requestRender(const RenderKey& key)
{
    // 1. Cache hit → emit immediately on caller's thread.
    QImage cached = g_cache.get(key);
    if (!cached.isNull()) {
        emit pageRendered(key, cached);
        return;
    }

    // 2. Already in-flight → do not queue a duplicate.
    {
        QMutexLocker lk(&m_inflightMutex);
        if (m_inFlight.contains(key))
            return;
        m_inFlight.insert(key);
    }

    // 3. Submit to thread pool.
    submitToThreadPool(key);
}

void PdfRenderService::cancelPending()
{
    QMutexLocker lk(&m_inflightMutex);
    // Clearing the set causes onWorkerDone() to silently discard late results.
    m_inFlight.clear();
}

void PdfRenderService::submitToThreadPool(const RenderKey& key)
{
    auto* task = new RenderTask(this, m_renderer.get(), key);
    m_pool->start(task);
}

void PdfRenderService::onWorkerDone(const RenderKey& key,
                                    const QImage&    image,
                                    const QString&   error)
{
    {
        QMutexLocker lk(&m_inflightMutex);
        if (!m_inFlight.remove(key))
            return; // This request was cancelled; discard result.
    }

    if (!image.isNull()) {
        g_cache.put(key, image);
        emit pageRendered(key, image);
    } else {
        emit renderError(key, error);
    }
}
