// ThumbnailSidebarWidget.h
// Shows page thumbnails in a QListWidget; clicking selects the page.
//
// Thumbnails are rendered asynchronously via PdfRenderService at low resolution
// (≤160 px wide).  A grey placeholder is shown until each thumbnail arrives.
// The sidebar does NOT own its own PdfRenderService; it borrows the one owned
// by PdfViewerWidget (same renderer, same document, separate render keys).

#ifndef THUMBNAILSIDEBARWIDGET_H
#define THUMBNAILSIDEBARWIDGET_H

#include <QWidget>
#include <QListWidget>
#include "../../pdf/IPdfRenderer.h"   // RenderKey

class PdfRenderService;

class ThumbnailSidebarWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ThumbnailSidebarWidget(QWidget* parent = nullptr);

    // Called by PdfViewerWidget once a document is open.
    // service  – borrowed; must outlive this widget.
    // filePath – used to build RenderKeys for thumbnails.
    // count    – total page count.
    void setup(PdfRenderService* service, const QString& filePath, int count);

    // Highlight the thumbnail for the given page (0-based).
    void setCurrentPage(int pageIndex);

    // Reset everything (e.g. when a new document is opened).
    void clear();

signals:
    // Emitted when the user clicks a thumbnail.
    void pageClicked(int pageIndex);

public slots:
    // Feed rendered thumbnail images back in.  Only those whose key.pageIndex
    // matches a pending thumbnail slot are consumed here.
    void onThumbnailRendered(const RenderKey& key, const QImage& image);

private slots:
    void onItemActivated(QListWidgetItem* item);

private:
    void requestNextBatch();

    QListWidget*      m_list        = nullptr;
    PdfRenderService* m_service     = nullptr;
    QString           m_filePath;
    int               m_pageCount   = 0;
    int               m_nextRequest = 0;   // Next thumbnail page index to request
    bool              m_setupDone   = false;

    static constexpr int kThumbWidth  = 120;
    static constexpr int kBatchSize   = 8;    // How many thumbnails to request at once
};

#endif // THUMBNAILSIDEBARWIDGET_H
