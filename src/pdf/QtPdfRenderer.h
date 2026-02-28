// QtPdfRenderer.h
// PDF renderer backend using Qt6::Pdf (QPdfDocument).
// Compiled only when PDFVIEWER_USE_QTPDF is defined by CMake.

#ifndef QTPDFRENDERER_H
#define QTPDFRENDERER_H

#ifdef PDFVIEWER_USE_QTPDF

#include "IPdfRenderer.h"
#include <QPdfDocument>
#include <QMutex>

class QtPdfRenderer final : public IPdfRenderer
{
public:
    QtPdfRenderer();
    ~QtPdfRenderer() override;

    bool    load(const QString& filePath)                                             override;
    void    close()                                                                    override;
    int     pageCount()                                                         const override;
    QSizeF  pageSizePoints(int pageIndex)                                       const override;
    QImage  renderPage(int pageIndex, QSize targetPx, float zoom, int rotDeg)         override;
    QString lastError()                                                         const override;
    QString backendName()                                               const override { return QStringLiteral("Qt PDF"); }

private:
    QPdfDocument*  m_doc = nullptr;
    mutable QMutex m_mutex;
    mutable QString m_lastError;
};

#endif // PDFVIEWER_USE_QTPDF
#endif // QTPDFRENDERER_H
