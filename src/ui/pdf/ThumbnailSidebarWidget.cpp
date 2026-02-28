// ThumbnailSidebarWidget.cpp
#include "ThumbnailSidebarWidget.h"
#include "../../pdf/PdfRenderService.h"

#include <QVBoxLayout>
#include <QListWidgetItem>
#include <QPixmap>
#include <QImage>
#include <QLabel>
#include <QScrollBar>
#include <QPainter>

static constexpr int kThumbHeight = 160;   // max height
static constexpr int kItemSpacing = 6;

ThumbnailSidebarWidget::ThumbnailSidebarWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_list = new QListWidget(this);
    m_list->setViewMode(QListView::IconMode);
    m_list->setIconSize(QSize(ThumbnailSidebarWidget::kThumbWidth, kThumbHeight));
    m_list->setGridSize(QSize(ThumbnailSidebarWidget::kThumbWidth + kItemSpacing,
                               kThumbHeight + 24));   // 24px for page-number label
    m_list->setResizeMode(QListView::Adjust);
    m_list->setMovement(QListView::Static);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    m_list->setFlow(QListView::TopToBottom);
    m_list->setWrapping(false);
    m_list->setSpacing(kItemSpacing / 2);

    // Stretch items to fill width when displayed in a narrow sidebar.
    m_list->setUniformItemSizes(true);

    // Style: dark sidebar.
    m_list->setStyleSheet(
        QStringLiteral("QListWidget {"
                       "  background: #1e1e1e;"
                       "  border: none;"
                       "}"
                       "QListWidget::item {"
                       "  color: #cccccc;"
                       "  padding-bottom: 2px;"
                       "}"
                       "QListWidget::item:selected {"
                       "  background: #0078d4;"
                       "  color: white;"
                       "}"));

    layout->addWidget(m_list);

    connect(m_list, &QListWidget::itemClicked,
            this,   &ThumbnailSidebarWidget::onItemActivated);
}

void ThumbnailSidebarWidget::setup(PdfRenderService* service,
                                   const QString& filePath,
                                   int count)
{
    m_list->clear();
    m_service      = service;
    m_filePath     = filePath;
    m_pageCount    = count;
    m_nextRequest  = 0;
    m_setupDone    = false;

    if (count <= 0 || !service) {
        return;
    }

    // Create placeholder items for all pages immediately.
    QImage placeholder(kThumbWidth, kThumbHeight, QImage::Format_ARGB32);
    placeholder.fill(QColor(0x3c, 0x3c, 0x3c));
    const QIcon placeholderIcon(QPixmap::fromImage(placeholder));

    for (int i = 0; i < count; ++i) {
        auto* item = new QListWidgetItem(placeholderIcon,
                                         QString::number(i + 1));
        item->setData(Qt::UserRole, i);   // Store 0-based page index.
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignBottom);
        item->setSizeHint(QSize(kThumbWidth + kItemSpacing,
                                 kThumbHeight + 24));
        m_list->addItem(item);
    }

    m_setupDone = true;
    requestNextBatch();
}

void ThumbnailSidebarWidget::setCurrentPage(int pageIndex)
{
    if (pageIndex < 0 || pageIndex >= m_list->count()) {
        return;
    }
    // Block signals so the click handler does not re-navigate.
    const bool blocked = m_list->blockSignals(true);
    m_list->setCurrentRow(pageIndex);
    m_list->blockSignals(blocked);
}

void ThumbnailSidebarWidget::clear()
{
    m_list->clear();
    m_service     = nullptr;
    m_filePath.clear();
    m_pageCount   = 0;
    m_nextRequest = 0;
    m_setupDone   = false;
}

void ThumbnailSidebarWidget::onThumbnailRendered(const RenderKey& key,
                                                  const QImage& image)
{
    if (key.filePath != m_filePath) {
        return;   // Belongs to a different document.
    }
    const int pageIdx = key.pageIndex;
    if (pageIdx < 0 || pageIdx >= m_list->count() || image.isNull()) {
        return;
    }

    QListWidgetItem* item = m_list->item(pageIdx);
    if (!item) {
        return;
    }

    // Scale image to fit thumbnail slot while keeping aspect ratio.
    const QImage scaled = image.scaled(kThumbWidth, kThumbHeight,
                                        Qt::KeepAspectRatio,
                                        Qt::SmoothTransformation);
    item->setIcon(QIcon(QPixmap::fromImage(scaled)));

    // Request the next batch as thumbnails come in.
    if (m_nextRequest < m_pageCount) {
        requestNextBatch();
    }
}

void ThumbnailSidebarWidget::onItemActivated(QListWidgetItem* item)
{
    if (!item) {
        return;
    }
    const int pageIdx = item->data(Qt::UserRole).toInt();
    emit pageClicked(pageIdx);
}

void ThumbnailSidebarWidget::requestNextBatch()
{
    if (!m_service || !m_setupDone) {
        return;
    }

    int sent = 0;
    while (m_nextRequest < m_pageCount && sent < kBatchSize) {
        RenderKey key;
        key.filePath    = m_filePath;
        key.pageIndex   = m_nextRequest;
        key.zoom        = 1.0f;
        key.targetW     = kThumbWidth;
        key.targetH     = kThumbHeight;
        key.rotationDeg = 0;
        key.quality     = RenderQuality::Fast;

        m_service->requestRender(key);
        ++m_nextRequest;
        ++sent;
    }
}
