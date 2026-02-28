// NullPdfRenderer.h
// Always-compilable stub renderer used when no real backend is available.
// Renders a grey placeholder image explaining the situation.

#ifndef NULLPDFRENDERER_H
#define NULLPDFRENDERER_H

#include "IPdfRenderer.h"
#include <QPainter>

class NullPdfRenderer final : public IPdfRenderer
{
public:
    bool load(const QString& filePath) override
    {
        m_filePath = filePath;
        m_loaded   = !filePath.isEmpty();
        return m_loaded;
    }

    void close() override { m_loaded = false; m_filePath.clear(); }

    int pageCount() const override { return m_loaded ? 1 : 0; }

    QSizeF pageSizePoints(int /*pageIndex*/) const override
    {
        return m_loaded ? QSizeF(595, 842) : QSizeF(); // A4 in points
    }

    QImage renderPage(int pageIndex, QSize targetPx, float /*zoom*/, int /*rotationDeg*/) override
    {
        if (!m_loaded || pageIndex < 0)
            return {};

        QSize sz = (targetPx.width() > 0 && targetPx.height() > 0)
                       ? targetPx
                       : QSize(595, 842);

        QImage img(sz, QImage::Format_ARGB32_Premultiplied);
        img.fill(Qt::white);

        QPainter p(&img);
        p.fillRect(img.rect(), QColor(0xE8, 0xE8, 0xE8));
        p.setPen(Qt::darkGray);
        p.drawRect(img.rect().adjusted(0, 0, -1, -1));

        QFont f = p.font();
        f.setPixelSize(qMax(12, sz.height() / 20));
        p.setFont(f);
        p.drawText(img.rect(), Qt::AlignCenter | Qt::TextWordWrap,
                   QStringLiteral("No PDF backend compiled.\n\n"
                                  "Enable PDFVIEWER_USE_MUPDF or install Qt6::Pdf.\n\n"
                                  "File: ") + m_filePath);
        p.end();
        return img;
    }

    QString lastError() const override
    {
        return m_loaded ? QString()
                        : QStringLiteral("No PDF backend available.");
    }

    QString backendName() const override { return QStringLiteral("Null"); }

private:
    bool    m_loaded   = false;
    QString m_filePath;
};

#endif // NULLPDFRENDERER_H
