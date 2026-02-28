// PdfViewerWidget.h
// Full PDF viewer widget embedded in each tab.
//
// Layout (vertical):
//   [Toolbar]  ☰ | ◀ spinBox/count ▶ | − zoomCombo + | [Continuous][Single Page]
//   [QSplitter]
//     Left:  ThumbnailSidebarWidget (collapsible)
//     Right: PageCanvasView (QGraphicsView, continuous scroll default)
//   [StatusBar]   "Page X of Y"  |  backend  |  mode
//
// View modes
// ----------
//   Continuous (default): all pages rendered vertically; only visible ±2 rendered.
//   Single-page: one page at a time; toolbar prev/next and thumbnails navigate.
//
// Async rendering
// ---------------
//   PdfViewerWidget owns a PdfRenderService.
//   PageCanvasView emits renderPageRequested() → PdfViewerWidget submits the key.
//   PdfRenderService emits pageRendered() → PdfViewerWidget routes to canvas or sidebar.
//   Per-page renderVer tokens guard against stale responses.
//
// Per-tab state (saved/restored by TabManager):
//   currentPage, viewMode, zoomMode, zoomPercent, scrollPos, sidebarVisible, splitterSizes.

#ifndef PDFVIEWERWIDGET_H
#define PDFVIEWERWIDGET_H

#include <QWidget>
#include <QPointF>
#include <QVector>
#include <memory>

#include "PageCanvasView.h"   // ZoomMode, PdfViewMode

class PageCanvasView;
class ThumbnailSidebarWidget;
class PdfRenderService;
struct RenderKey;
class QImage;
class QSplitter;
class QToolBar;
class QSpinBox;
class QLabel;
class QComboBox;
class QAction;
class QKeyEvent;
class QAbstractButton;

class PdfViewerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PdfViewerWidget(const QString& filePath, QWidget* parent = nullptr);
    ~PdfViewerWidget() override;

    // ---- Document info ----
    QString filePath()  const { return m_filePath; }
    int     pageCount() const { return m_pageCount; }

    // ---- Per-tab state (get/set for TabManager) ----
    int        currentPage()    const { return m_currentPage; }
    double     zoomPercent()    const;
    ZoomMode   zoomMode()       const;
    PdfViewMode viewMode()      const;
    QPointF    scrollPos()      const;
    bool       sidebarVisible() const;
    QList<int> splitterSizes()  const;

    void setCurrentPage(int page);
    void setZoomPercent(double pct);
    void setZoomMode(ZoomMode mode);
    void setViewMode(PdfViewMode mode);
    void setScrollPos(const QPointF& pos);
    void setSidebarVisible(bool visible);
    void setSplitterSizes(const QList<int>& sizes);

signals:
    void titleChanged(const QString& title);

protected:
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    // Render service signals
    void onPageRendered(const RenderKey& key, const QImage& image);
    void onRenderError(const RenderKey& key,  const QString& message);

    // Canvas signals
    void onRenderPageRequested(int pageIndex, int renderVer,
                               QSize targetSize, double zoomFactor);
    void onZoomChanged(double pct);
    void onCurrentPageChanged(int pageIndex);

    // Toolbar slots
    void onPrevPage();
    void onNextPage();
    void onZoomIn();
    void onZoomOut();
    void onZoomComboChanged(int index);
    void onSpinBoxCommitted();
    void onToggleSidebar();
    void onContinuousModeClicked();
    void onSinglePageModeClicked();

private:
    void setupUI();
    void setupToolbar(QToolBar* toolbar);
    void openDocument();
    void navigateToPage(int pageIndex);
    void updatePageControls();
    void updateZoomCombo();
    void updateViewModeButtons();
    void updateStatus(const QString& extra = {});
    QString makeTitle() const;

    // Render helper
    void requestRender(int pageIndex, int renderVer, QSize targetSize, double zoom);

    // ---- Document state ----
    QString m_filePath;
    int     m_pageCount   = 0;
    int     m_currentPage = 0;   // 0-based

    // Per-page render version: mirrors what PageCanvasView tracks per page.
    // Updated in onRenderPageRequested; checked in onPageRendered.
    QVector<int>   m_pageRenderVer;
    QVector<QSize> m_pageRenderSize;  // Expected size for the current version

    std::unique_ptr<PdfRenderService> m_renderService;

    // ---- UI ----
    QSplitter*              m_splitter      = nullptr;
    ThumbnailSidebarWidget* m_sidebar       = nullptr;
    PageCanvasView*         m_canvas        = nullptr;
    QSpinBox*               m_spinBox       = nullptr;
    QLabel*                 m_pageLabel     = nullptr;
    QComboBox*              m_zoomCombo     = nullptr;
    QLabel*                 m_statusLabel   = nullptr;
    QAction*                m_sidebarAct    = nullptr;
    QAbstractButton*        m_btnContinuous = nullptr;
    QAbstractButton*        m_btnSinglePage = nullptr;

    // ---- Zoom presets ----
    static const QList<QPair<QString,double>> kZoomPresets;
};

#endif // PDFVIEWERWIDGET_H
