// PageCanvasView.cpp
#include "PageCanvasView.h"

#include <QTimer>
#include <QResizeEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QScrollBar>
#include <QGraphicsRectItem>
#include <QPainter>
#include <QtMath>
#include <QDebug>

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
PageCanvasView::PageCanvasView(QWidget* parent)
    : QGraphicsView(parent)
{
    m_scene = new QGraphicsScene(this);
    setScene(m_scene);

    // Show a "Loading…" text until setupPages() is called.
    auto* loadingText = m_scene->addText(QStringLiteral("Loading…"));
    loadingText->setDefaultTextColor(QColor(0x88, 0x88, 0x88));
    QFont f = loadingText->font();
    f.setPointSize(18);
    loadingText->setFont(f);
    loadingText->setPos(0, 0);
    m_scene->setSceneRect(0, 0, 400, 100);

    setDragMode(QGraphicsView::ScrollHandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);

    setRenderHint(QPainter::SmoothPixmapTransform, true);
    setRenderHint(QPainter::Antialiasing, false);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setOptimizationFlags(QGraphicsView::DontSavePainterState);
    setBackgroundBrush(QColor(0x2b, 0x2b, 0x2b));

    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    m_debounce = new QTimer(this);
    m_debounce->setSingleShot(true);
    m_debounce->setInterval(kDebounceMs);
    connect(m_debounce, &QTimer::timeout, this, &PageCanvasView::onDebounceTimeout);
}

// ---------------------------------------------------------------------------
// Document setup
// ---------------------------------------------------------------------------
void PageCanvasView::setupPages(const QVector<QSizeF>& pageSizes)
{
    clearPages();
    ++m_epochToken;

    m_pageItems.resize(pageSizes.size());
    for (int i = 0; i < pageSizes.size(); ++i) {
        PageItem& pi = m_pageItems[i];
        pi.ptSize   = pageSizes[i];
        pi.rendered = false;
        pi.renderVer = 0;
        pi.item = m_scene->addPixmap(QPixmap());
        pi.item->setShapeMode(QGraphicsPixmapItem::BoundingRectShape);
        pi.item->setZValue(0.0);
    }

    rebuildScene();
    scheduleDebounce();
}

void PageCanvasView::clearPages()
{
    m_debounce->stop();
    m_scene->clear();
    m_pageItems.clear();
    m_currentPage = 0;
    ++m_epochToken;
}

// ---------------------------------------------------------------------------
// Feed rendered image
// ---------------------------------------------------------------------------
void PageCanvasView::showRenderedPage(int pageIndex, int renderVer, const QImage& image)
{
    if (pageIndex < 0 || pageIndex >= m_pageItems.size()) return;
    PageItem& pi = m_pageItems[pageIndex];

    // Stale check: drop if a newer request has been issued for this page.
    if (renderVer != pi.renderVer) return;
    if (image.isNull()) return;

    const QPixmap pm = QPixmap::fromImage(image, Qt::NoFormatConversion);
    pi.item->setPixmap(pm);
    pi.rendered = true;

    // In SinglePage mode the scene rect is the single item; in Continuous it
    // was already laid out — no relayout needed.
}

void PageCanvasView::showPagePlaceholder(int pageIndex)
{
    if (pageIndex < 0 || pageIndex >= m_pageItems.size()) return;
    PageItem& pi = m_pageItems[pageIndex];

    // Use stored scene rect size for placeholder.
    const int w = qMax(1, static_cast<int>(pi.sceneRect.width()));
    const int h = qMax(1, static_cast<int>(pi.sceneRect.height()));

    QImage ph(qMin(w, 64), qMin(h, 64), QImage::Format_ARGB32);
    ph.fill(QColor(0x3c, 0x3c, 0x3c));
    pi.item->setPixmap(QPixmap::fromImage(ph));
    pi.rendered = false;
}

// ---------------------------------------------------------------------------
// View mode
// ---------------------------------------------------------------------------
void PageCanvasView::setViewMode(PdfViewMode mode)
{
    if (m_viewMode == mode) return;
    m_viewMode = mode;
    ++m_epochToken;
    rebuildScene();
    scheduleDebounce();
}

// ---------------------------------------------------------------------------
// Page navigation
// ---------------------------------------------------------------------------
void PageCanvasView::setCurrentPage(int pageIndex)
{
    if (m_pageItems.isEmpty()) return;
    pageIndex = qBound(0, pageIndex, m_pageItems.size() - 1);

    if (m_viewMode == PdfViewMode::SinglePage) {
        m_currentPage = pageIndex;
        rebuildScene();
        scheduleDebounce();
        emit currentPageChanged(m_currentPage);
    } else {
        // Continuous: scroll to that page's top.
        if (pageIndex < 0 || pageIndex >= m_pageItems.size()) return;
        const QRectF& pr = m_pageItems[pageIndex].sceneRect;
        // Center the page horizontally, scroll to its top.
        centerOn(pr.center().x(), pr.top() + 1);
        m_currentPage = pageIndex;
        emit currentPageChanged(m_currentPage);
    }
}

// ---------------------------------------------------------------------------
// Zoom
// ---------------------------------------------------------------------------
void PageCanvasView::setZoomMode(ZoomMode mode)
{
    if (m_zoomMode == mode) return;
    m_zoomMode = mode;
    applyZoom();
    scheduleDebounce();
}

void PageCanvasView::setZoomPercent(double pct)
{
    pct = qBound(kZoomMin, pct, kZoomMax);
    if (qFuzzyCompare(pct, m_zoomPct) && m_zoomMode == ZoomMode::Custom) return;
    m_zoomMode = ZoomMode::Custom;
    m_zoomPct  = pct;
    applyZoom();
    scheduleDebounce();
}

// ---------------------------------------------------------------------------
// Scroll state
// ---------------------------------------------------------------------------
QPointF PageCanvasView::scrollPos() const
{
    return QPointF(horizontalScrollBar()->value(),
                   verticalScrollBar()->value());
}

void PageCanvasView::setScrollPos(const QPointF& pos)
{
    horizontalScrollBar()->setValue(static_cast<int>(pos.x()));
    verticalScrollBar()->setValue(static_cast<int>(pos.y()));
}

// ---------------------------------------------------------------------------
// Events
// ---------------------------------------------------------------------------
void PageCanvasView::resizeEvent(QResizeEvent* event)
{
    QGraphicsView::resizeEvent(event);
    if (m_zoomMode != ZoomMode::Custom) {
        applyZoom();
    }
    scheduleDebounce();
}

void PageCanvasView::wheelEvent(QWheelEvent* event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        const double delta = event->angleDelta().y() / 120.0;
        double newPct = m_zoomPct + delta * kZoomStep;
        newPct = qBound(kZoomMin, newPct, kZoomMax);
        if (!qFuzzyCompare(newPct, m_zoomPct)) {
            m_zoomMode = ZoomMode::Custom;
            m_zoomPct  = newPct;
            applyZoom();
            scheduleDebounce();
            emit zoomChanged(m_zoomPct);
        }
        event->accept();
    } else {
        QGraphicsView::wheelEvent(event);
    }
}

void PageCanvasView::keyPressEvent(QKeyEvent* event)
{
    if ((event->key() == Qt::Key_Tab || event->key() == Qt::Key_Backtab)
            && (event->modifiers() & Qt::ControlModifier)) {
        event->ignore();
        return;
    }
    QGraphicsView::keyPressEvent(event);
}

void PageCanvasView::scrollContentsBy(int dx, int dy)
{
    QGraphicsView::scrollContentsBy(dx, dy);
    // After scrolling, update which pages are visible (non-debounced for
    // responsive virtualization, but only request render after debounce).
    if (m_viewMode == PdfViewMode::Continuous) {
        const int top = topVisiblePage();
        if (top != m_currentPage) {
            m_currentPage = top;
            emit currentPageChanged(m_currentPage);
        }
        // Kick the debounce to render newly visible pages.
        scheduleDebounce();
    }
}

// ---------------------------------------------------------------------------
// Debounce timeout
// ---------------------------------------------------------------------------
void PageCanvasView::onDebounceTimeout()
{
    updateVisiblePages();
}

// ---------------------------------------------------------------------------
// Scene layout
// ---------------------------------------------------------------------------
void PageCanvasView::rebuildScene()
{
    // Reposition all items and fill placeholders.
    layoutPages();
    applyZoom();

    if (m_viewMode == PdfViewMode::SinglePage) {
        // Hide all but the current page.
        for (int i = 0; i < m_pageItems.size(); ++i) {
            m_pageItems[i].item->setVisible(i == m_currentPage);
        }
        if (!m_pageItems.isEmpty()) {
            const QRectF& r = m_pageItems[m_currentPage].sceneRect;
            m_scene->setSceneRect(r);
        }
    } else {
        for (auto& pi : m_pageItems) {
            pi.item->setVisible(true);
        }
        if (!m_pageItems.isEmpty()) {
            const QRectF& last = m_pageItems.back().sceneRect;
            m_scene->setSceneRect(0, 0,
                                  last.right(),
                                  last.bottom() + kPageGap);
        }
    }
}

void PageCanvasView::layoutPages()
{
    if (m_pageItems.isEmpty()) return;

    // In SinglePage mode we lay out all items at the same origin; visibility
    // is toggled.  In Continuous mode they are stacked vertically.
    constexpr double kPtToPx = 96.0 / 72.0;   // 1 PDF pt → screen px (logical)

    double y = kPageGap;
    double maxW = 0.0;

    for (int i = 0; i < m_pageItems.size(); ++i) {
        PageItem& pi = m_pageItems[i];
        const double pw = pi.ptSize.width()  * kPtToPx;
        const double ph = pi.ptSize.height() * kPtToPx;
        maxW = qMax(maxW, pw);

        if (m_viewMode == PdfViewMode::Continuous) {
            pi.sceneRect = QRectF(0, y, pw, ph);
            pi.item->setPos(0, y);
            y += ph + kPageGap;
        } else {
            // SinglePage: all at (0,0).
            pi.sceneRect = QRectF(0, 0, pw, ph);
            pi.item->setPos(0, 0);
        }

        // Fill placeholder if not yet rendered.
        if (!pi.rendered) {
            QImage ph2(qMin(static_cast<int>(pw), 64),
                       qMin(static_cast<int>(ph), 64),
                       QImage::Format_ARGB32);
            ph2.fill(QColor(0x3c, 0x3c, 0x3c));
            pi.item->setPixmap(QPixmap::fromImage(ph2));
        }
    }

    // Center pages horizontally in Continuous mode by shifting items.
    if (m_viewMode == PdfViewMode::Continuous && maxW > 0) {
        for (auto& pi : m_pageItems) {
            const double x = (maxW - pi.sceneRect.width()) / 2.0;
            pi.item->setX(x);
            pi.sceneRect.moveLeft(x);
        }
    }
}

// ---------------------------------------------------------------------------
// Zoom application
// ---------------------------------------------------------------------------
void PageCanvasView::applyZoom()
{
    if (m_pageItems.isEmpty()) return;

    const QRectF scr = m_scene->sceneRect();
    if (scr.isEmpty()) return;

    if (m_zoomMode == ZoomMode::FitPage) {
        if (m_viewMode == PdfViewMode::SinglePage) {
            fitInView(m_pageItems[m_currentPage].sceneRect, Qt::KeepAspectRatio);
        } else {
            fitInView(m_pageItems[m_currentPage].sceneRect, Qt::KeepAspectRatio);
        }
        const qreal sx = transform().m11();
        const qreal sy = transform().m22();
        m_zoomPct = qMin(sx, sy) * 100.0;
    } else if (m_zoomMode == ZoomMode::FitWidth) {
        const qreal vw = viewport()->width();
        const qreal pw = (m_viewMode == PdfViewMode::SinglePage)
                         ? m_pageItems[m_currentPage].sceneRect.width()
                         : scr.width();
        if (pw > 0) {
            const qreal scale = vw / pw;
            setTransform(QTransform::fromScale(scale, scale));
            m_zoomPct = scale * 100.0;
        }
    } else {
        const qreal scale = m_zoomPct / 100.0;
        setTransform(QTransform::fromScale(scale, scale));
    }
}

// ---------------------------------------------------------------------------
// Virtualization
// ---------------------------------------------------------------------------
void PageCanvasView::updateVisiblePages()
{
    if (m_pageItems.isEmpty()) return;

    if (m_viewMode == PdfViewMode::SinglePage) {
        // Only current page needs rendering.
        const int ver = ++m_pageItems[m_currentPage].renderVer;
        const double zoom = m_zoomPct / 100.0;
        const QSize ts = computeTargetSize(m_currentPage, zoom);
        emit renderPageRequested(m_currentPage, ver, ts, zoom);
        return;
    }

    // --- Continuous mode ---
    // Find which pages intersect the viewport.
    const QRectF vr = mapToScene(viewport()->rect()).boundingRect();

    int firstVisible = -1, lastVisible = -1;
    for (int i = 0; i < m_pageItems.size(); ++i) {
        if (m_pageItems[i].sceneRect.intersects(vr)) {
            if (firstVisible < 0) firstVisible = i;
            lastVisible = i;
        }
    }
    if (firstVisible < 0) return;   // Nothing visible (layout not done yet)

    // Prefetch ±kPrefetchExtra.
    const int renderFirst = qMax(0, firstVisible - kPrefetchExtra);
    const int renderLast  = qMin(m_pageItems.size() - 1, lastVisible + kPrefetchExtra);

    // Request renders for pages in the render window.
    const double zoom = m_zoomPct / 100.0;
    for (int i = renderFirst; i <= renderLast; ++i) {
        PageItem& pi = m_pageItems[i];
        if (!pi.rendered) {
            const int ver = ++pi.renderVer;
            const QSize ts = computeTargetSize(i, zoom);
            emit renderPageRequested(i, ver, ts, zoom);
        }
    }

    // Drop pixmaps for pages far outside the window.
    dropDistantPixmaps(firstVisible, lastVisible);
}

int PageCanvasView::topVisiblePage() const
{
    const QRectF vr = mapToScene(viewport()->rect()).boundingRect();
    for (int i = 0; i < m_pageItems.size(); ++i) {
        if (m_pageItems[i].sceneRect.bottom() >= vr.top()) {
            return i;
        }
    }
    return qMax(0, m_pageItems.size() - 1);
}

void PageCanvasView::dropDistantPixmaps(int visibleFirst, int visibleLast)
{
    const int keepFirst = qMax(0, visibleFirst - kDropDistance);
    const int keepLast  = qMin(m_pageItems.size() - 1, visibleLast + kDropDistance);

    for (int i = 0; i < m_pageItems.size(); ++i) {
        if (i < keepFirst || i > keepLast) {
            PageItem& pi = m_pageItems[i];
            if (pi.rendered) {
                // Drop pixmap and mark unrendered so it re-renders when needed.
                pi.item->setPixmap(QPixmap());
                pi.rendered = false;
                // Also show a tiny placeholder so the space is visible.
                showPagePlaceholder(i);
            }
        }
    }
}

QSize PageCanvasView::computeTargetSize(int pageIndex, double zoom) const
{
    if (pageIndex < 0 || pageIndex >= m_pageItems.size()) return {};
    constexpr double kPtToPx = 96.0 / 72.0;
    const QSizeF& ptSz = m_pageItems[pageIndex].ptSize;

    if (m_zoomMode == ZoomMode::FitWidth) {
        const double vw = viewport()->width();
        const double scale = (m_pageItems[pageIndex].sceneRect.width() > 0)
                             ? vw / m_pageItems[pageIndex].sceneRect.width()
                             : 1.0;
        const int w = static_cast<int>(ptSz.width()  * kPtToPx * scale);
        const int h = static_cast<int>(ptSz.height() * kPtToPx * scale);
        return QSize(qBound(16, w, 8192), qBound(16, h, 8192));
    } else if (m_zoomMode == ZoomMode::FitPage) {
        const double vw = viewport()->width();
        const double vh = viewport()->height();
        const double pw = ptSz.width()  * kPtToPx;
        const double ph = ptSz.height() * kPtToPx;
        const double scale = (pw > 0 && ph > 0) ? qMin(vw / pw, vh / ph) : 1.0;
        const int w = static_cast<int>(pw * scale);
        const int h = static_cast<int>(ph * scale);
        return QSize(qBound(16, w, 8192), qBound(16, h, 8192));
    } else {
        const int w = static_cast<int>(ptSz.width()  * kPtToPx * zoom);
        const int h = static_cast<int>(ptSz.height() * kPtToPx * zoom);
        return QSize(qBound(16, w, 8192), qBound(16, h, 8192));
    }
}

// ---------------------------------------------------------------------------
// Debounce
// ---------------------------------------------------------------------------
void PageCanvasView::scheduleDebounce()
{
    m_debounce->start(kDebounceMs);
}
