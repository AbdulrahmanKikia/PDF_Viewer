// QtPdfRenderer.cpp
#include "QtPdfRenderer.h"

#ifdef PDFVIEWER_USE_QTPDF

#include <QPdfDocument>
#include <QPdfDocumentRenderOptions>
#include <QMutexLocker>
#include <QTransform>

QtPdfRenderer::QtPdfRenderer()
    : m_doc(new QPdfDocument(nullptr))
{}

QtPdfRenderer::~QtPdfRenderer()
{
    delete m_doc;
}

bool QtPdfRenderer::load(const QString& filePath)
{
    QMutexLocker lk(&m_mutex);
    m_lastError.clear();
    const QPdfDocument::Error err = m_doc->load(filePath);
    if (err != QPdfDocument::Error::None) {
        m_lastError = QStringLiteral("QPdfDocument::load failed (%1): %2")
                          .arg(QString::number(static_cast<int>(err)))
                          .arg(filePath);
        return false;
    }
    return true;
}

void QtPdfRenderer::close()
{
    QMutexLocker lk(&m_mutex);
    m_doc->close();
    m_lastError.clear();
}

int QtPdfRenderer::pageCount() const
{
    QMutexLocker lk(&m_mutex);
    return m_doc->pageCount();
}

QSizeF QtPdfRenderer::pageSizePoints(int pageIndex) const
{
    QMutexLocker lk(&m_mutex);
    if (pageIndex < 0 || pageIndex >= m_doc->pageCount()) return {};
    return m_doc->pagePointSize(pageIndex);
}

QImage QtPdfRenderer::renderPage(int pageIndex, QSize targetPx, float zoom, int rotDeg)
{
    QMutexLocker lk(&m_mutex);
    m_lastError.clear();

    if (pageIndex < 0 || pageIndex >= m_doc->pageCount()) {
        m_lastError = QStringLiteral("Page index out of range: %1").arg(pageIndex);
        return {};
    }

    QPdfDocumentRenderOptions opts;
    opts.setRotation(static_cast<QPdfDocumentRenderOptions::Rotation>(
        ((rotDeg / 90) % 4 + 4) % 4));   // map to 0-3

    QSizeF ptSz = m_doc->pagePointSize(pageIndex);
    QSizeF pixSz = ptSz * zoom;

    if (targetPx.width() > 0 && targetPx.height() > 0 && !pixSz.isEmpty()) {
        double scale = qMin(double(targetPx.width())  / pixSz.width(),
                            double(targetPx.height()) / pixSz.height());
        if (scale < 1.0) pixSz *= scale;
    }

    QSize renderSz = pixSz.toSize().expandedTo(QSize(1, 1));
    QImage img = m_doc->render(pageIndex, renderSz, opts);
    if (img.isNull())
        m_lastError = QStringLiteral("QPdfDocument::render returned null for page %1").arg(pageIndex);
    return img;
}

QString QtPdfRenderer::lastError() const
{
    QMutexLocker lk(&m_mutex);
    return m_lastError;
}

#endif // PDFVIEWER_USE_QTPDF
