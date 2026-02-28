// PageCanvasView.h
// Multi-page QGraphicsView canvas for PDF display.
//
// Modes
// ------
//   Continuous (default) – all pages arranged vertically, 16 px gap, centered.
//     Virtualized: only visible pages ±2 are rendered; distant page pixmaps are
//     dropped to cap memory.
//
//   SinglePage – one page at a time; prev/next changes current page.
//
// Zoom
// -----
//   FitWidth  – page width fills viewport width.
//   FitPage   – entire page fits viewport.
//   Custom    – absolute percent (25–400).
//   Resize and zoom changes are debounced 150 ms before a render is requested.
//
// Interaction
// -----------
//   Wheel       → vertical scroll.
//   Ctrl+Wheel  → zoom.
//   Left-drag   → pan (ScrollHandDrag).

#ifndef PAGECANVASVIEW_H
#define PAGECANVASVIEW_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QHash>
#include <QVector>

class QTimer;
class PdfRenderService;
struct RenderKey;
class QImage;

// ---------------------------------------------------------------------------
// ZoomMode
// ---------------------------------------------------------------------------
enum class ZoomMode {
    FitWidth,
    FitPage,
    Custom
};

// ---------------------------------------------------------------------------
// PdfViewMode — named with "Pdf" prefix to avoid clash with HomeContentWidget's ViewMode
// ---------------------------------------------------------------------------
enum class PdfViewMode {
    Continuous,   // default
    SinglePage
};

// ---------------------------------------------------------------------------
// Per-page scene entry
// ---------------------------------------------------------------------------
struct PageItem {
    QGraphicsPixmapItem* item     = nullptr;
    QRectF               sceneRect;           // bounding rect in scene coords
    QSizeF               ptSize;              // page size in PDF points
    bool                 rendered  = false;   // true when a real pixmap is set
    int                  renderVer = 0;       // incremented on each render request
};

// ---------------------------------------------------------------------------
// PageCanvasView
// ---------------------------------------------------------------------------
class PageCanvasView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit PageCanvasView(QWidget* parent = nullptr);

    // ---- Document setup ----
    // Called by PdfViewerWidget after the document is open.
    // pageSizes: page sizes in PDF points (1 pt = 1/72 inch), 0-based.
    void setupPages(const QVector<QSizeF>& pageSizes);

    // Reset to empty state (tab closing / new document).
    void clearPages();

    // ---- Feed rendered image back in ----
    // key.pageIndex identifies which page; key.targetW/H and renderVer allow
    // stale-result detection.  Safe to call from any thread via signal.
    void showRenderedPage(int pageIndex, int renderVer, const QImage& image);

    // Show grey placeholder for a page (called while render is in flight).
    void showPagePlaceholder(int pageIndex);

    // ---- View mode ----
    void       setViewMode(PdfViewMode mode);
    PdfViewMode viewMode() const { return m_viewMode; }

    // ---- Page navigation (used in SinglePage mode and thumbnail clicks) ----
    void setCurrentPage(int pageIndex);
    int  currentPage()  const { return m_currentPage; }
    int  pageCount()    const { return m_pageItems.size(); }

    // ---- Zoom ----
    void     setZoomMode(ZoomMode mode);
    void     setZoomPercent(double pct);
    double   zoomPercent() const { return m_zoomPct; }
    ZoomMode zoomMode()    const { return m_zoomMode; }

    // ---- State (saved/restored by PdfViewerWidget for tab switching) ----
    QPointF scrollPos() const;
    void    setScrollPos(const QPointF& pos);

signals:
    // Emitted when the canvas needs pages rendered or re-rendered.
    // pageIndex, renderVer, targetSize (pixels), zoomFactor.
    void renderPageRequested(int pageIndex, int renderVer,
                             QSize targetSize, double zoomFactor);

    // Emitted when Ctrl+Wheel changes zoom.
    void zoomChanged(double newZoomPct);

    // Emitted when scrolling changes the top-visible page in Continuous mode.
    void currentPageChanged(int pageIndex);

protected:
    void resizeEvent(QResizeEvent* event)  override;
    void wheelEvent(QWheelEvent*  event)   override;
    void keyPressEvent(QKeyEvent* event)   override;
    void scrollContentsBy(int dx, int dy)  override;

private slots:
    void onDebounceTimeout();

private:
    // Layout helpers
    void  rebuildScene();
    void  layoutPages();         // Positions items in scene; does NOT clear pixmaps.
    void  applyZoom();

    // Virtualization
    void  updateVisiblePages();  // Determine visible range and request renders.
    int   topVisiblePage()  const;
    QRect visibleSceneRect() const;

    // Render request helpers
    void  requestPageRender(int pageIndex);
    void  dropDistantPixmaps(int visibleFirst, int visibleLast);
    QSize computeTargetSize(int pageIndex, double zoom) const;

    void  scheduleDebounce();

    // ---- Scene ----
    QGraphicsScene*       m_scene      = nullptr;
    QVector<PageItem>     m_pageItems;

    // ---- View state ----
    PdfViewMode m_viewMode = PdfViewMode::Continuous;
    ZoomMode  m_zoomMode    = ZoomMode::FitWidth;
    double    m_zoomPct     = 100.0;
    int       m_currentPage = 0;

    // ---- Constants ----
    static constexpr int    kPageGap       = 16;   // px between pages in scene
    static constexpr double kZoomMin       = 25.0;
    static constexpr double kZoomMax       = 400.0;
    static constexpr double kZoomStep      = 20.0; // per wheel notch
    static constexpr int    kDebounceMs    = 150;
    static constexpr int    kPrefetchExtra = 2;    // pages beyond visible to keep rendered
    static constexpr int    kDropDistance  = 5;    // pages beyond visible+prefetch to drop

    // ---- Debounce timer ----
    QTimer* m_debounce = nullptr;

    // ---- Render version counter (per page) is stored in PageItem::renderVer ----
    // Global token so we can detect stale responses across a mode change.
    int m_epochToken = 0;
};

#endif // PAGECANVASVIEW_H
