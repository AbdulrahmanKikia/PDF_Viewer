// PdfViewerWidget.cpp
#include "PdfViewerWidget.h"
#include "PageCanvasView.h"
#include "ThumbnailSidebarWidget.h"
#include "../../pdf/PdfRenderService.h"
#include "../../pdf/PdfRendererFactory.h"
#include "../../pdf/IPdfRenderer.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolBar>
#include <QAction>
#include <QSpinBox>
#include <QLabel>
#include <QComboBox>
#include <QSplitter>
#include <QFileInfo>
#include <QKeyEvent>
#include <QShortcut>
#include <QPushButton>
#include <QAbstractButton>
#include <QtMath>
#include <QDebug>

// ---------------------------------------------------------------------------
// Zoom preset table
// ---------------------------------------------------------------------------
const QList<QPair<QString,double>> PdfViewerWidget::kZoomPresets = {
    { QStringLiteral("Fit Width"),  -1.0 },
    { QStringLiteral("Fit Page"),   -2.0 },
    { QStringLiteral("—"),           0.0 },
    { QStringLiteral("25%"),        25.0  },
    { QStringLiteral("50%"),        50.0  },
    { QStringLiteral("75%"),        75.0  },
    { QStringLiteral("100%"),      100.0  },
    { QStringLiteral("125%"),      125.0  },
    { QStringLiteral("150%"),      150.0  },
    { QStringLiteral("200%"),      200.0  },
    { QStringLiteral("300%"),      300.0  },
    { QStringLiteral("400%"),      400.0  },
};

// ---------------------------------------------------------------------------
// Constructor / destructor
// ---------------------------------------------------------------------------
PdfViewerWidget::PdfViewerWidget(const QString& filePath, QWidget* parent)
    : QWidget(parent)
    , m_filePath(filePath)
{
    qDebug() << "[PdfViewerWidget] ctor" << filePath;
    setupUI();
    openDocument();
    qDebug() << "[PdfViewerWidget] setupUI + openDocument done, size=" << size();
}

PdfViewerWidget::~PdfViewerWidget() = default;

// ---------------------------------------------------------------------------
// Per-tab state getters
// ---------------------------------------------------------------------------
double PdfViewerWidget::zoomPercent() const
{
    return m_canvas ? m_canvas->zoomPercent() : 100.0;
}

ZoomMode PdfViewerWidget::zoomMode() const
{
    return m_canvas ? m_canvas->zoomMode() : ZoomMode::FitWidth;
}

PdfViewMode PdfViewerWidget::viewMode() const
{
    return m_canvas ? m_canvas->viewMode() : PdfViewMode::Continuous;
}

QPointF PdfViewerWidget::scrollPos() const
{
    return m_canvas ? m_canvas->scrollPos() : QPointF();
}

bool PdfViewerWidget::sidebarVisible() const
{
    return m_sidebar ? m_sidebar->isVisible() : true;
}

QList<int> PdfViewerWidget::splitterSizes() const
{
    return m_splitter ? m_splitter->sizes() : QList<int>{};
}

// ---------------------------------------------------------------------------
// Per-tab state setters
// ---------------------------------------------------------------------------
void PdfViewerWidget::setCurrentPage(int page)
{
    navigateToPage(page);
}

void PdfViewerWidget::setZoomPercent(double pct)
{
    if (m_canvas) {
        m_canvas->setZoomPercent(pct);
        updateZoomCombo();
    }
}

void PdfViewerWidget::setZoomMode(ZoomMode mode)
{
    if (m_canvas) {
        m_canvas->setZoomMode(mode);
        updateZoomCombo();
    }
}

void PdfViewerWidget::setViewMode(PdfViewMode mode)
{
    if (m_canvas) {
        m_canvas->setViewMode(mode);
        updateViewModeButtons();
        updateStatus();
    }
}

void PdfViewerWidget::setScrollPos(const QPointF& pos)
{
    if (m_canvas) m_canvas->setScrollPos(pos);
}

void PdfViewerWidget::setSidebarVisible(bool visible)
{
    if (m_sidebar)    m_sidebar->setVisible(visible);
    if (m_sidebarAct) m_sidebarAct->setChecked(visible);
}

void PdfViewerWidget::setSplitterSizes(const QList<int>& sizes)
{
    if (m_splitter && sizes.size() == 2)
        m_splitter->setSizes(sizes);
}

// ---------------------------------------------------------------------------
// UI construction
// ---------------------------------------------------------------------------
void PdfViewerWidget::setupUI()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ---- Toolbar ----
    auto* toolbar = new QToolBar(this);
    toolbar->setMovable(false);
    toolbar->setFloatable(false);
    toolbar->setIconSize(QSize(16, 16));
    setupToolbar(toolbar);
    root->addWidget(toolbar);

    // ---- Splitter (sidebar | canvas) ----
    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->setChildrenCollapsible(true);
    m_splitter->setHandleWidth(4);

    m_sidebar = new ThumbnailSidebarWidget(m_splitter);
    m_sidebar->setMinimumWidth(100);
    m_sidebar->setMaximumWidth(220);

    m_canvas = new PageCanvasView(m_splitter);
    m_canvas->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_splitter->addWidget(m_sidebar);
    m_splitter->addWidget(m_canvas);
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);
    m_splitter->setSizes({140, 800});

    root->addWidget(m_splitter, 1);

    // ---- Status bar ----
    auto* statusWidget = new QWidget(this);
    auto* statusLayout = new QHBoxLayout(statusWidget);
    statusLayout->setContentsMargins(8, 2, 8, 2);
    statusLayout->setSpacing(0);
    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet(QStringLiteral(
        "QLabel { color: #aaaaaa; font-size: 11px; }"));
    statusLayout->addWidget(m_statusLabel);
    statusWidget->setStyleSheet(
        QStringLiteral("background: #2b2b2b; border-top: 1px solid #404040;"));
    root->addWidget(statusWidget);

    // ---- Connect canvas signals ----
    connect(m_canvas, &PageCanvasView::renderPageRequested,
            this,     &PdfViewerWidget::onRenderPageRequested);
    connect(m_canvas, &PageCanvasView::zoomChanged,
            this,     &PdfViewerWidget::onZoomChanged);
    connect(m_canvas, &PageCanvasView::currentPageChanged,
            this,     &PdfViewerWidget::onCurrentPageChanged);

    // ---- Connect thumbnail sidebar ----
    connect(m_sidebar, &ThumbnailSidebarWidget::pageClicked,
            this, [this](int pageIdx) { navigateToPage(pageIdx); });

    updateStatus(QStringLiteral("Loading…"));
}

void PdfViewerWidget::setupToolbar(QToolBar* toolbar)
{
    const QString kToolbarStyle = QStringLiteral(
        "QToolBar { background: #2b2b2b; border-bottom: 1px solid #404040; padding: 2px; }"
        "QToolButton { color: #cccccc; padding: 3px 6px; border-radius: 3px; }"
        "QToolButton:hover { background: #3e3e3e; }"
        "QToolButton:pressed { background: #555555; }"
        "QToolButton:checked { background: #0078d4; color: white; }");
    toolbar->setStyleSheet(kToolbarStyle);

    // Sidebar toggle
    m_sidebarAct = toolbar->addAction(QStringLiteral("☰"));
    m_sidebarAct->setToolTip(QStringLiteral("Toggle thumbnail sidebar"));
    m_sidebarAct->setCheckable(true);
    m_sidebarAct->setChecked(true);
    connect(m_sidebarAct, &QAction::triggered, this, &PdfViewerWidget::onToggleSidebar);

    toolbar->addSeparator();

    // Prev / Next
    auto* prevAct = toolbar->addAction(QStringLiteral("◀"));
    prevAct->setToolTip(QStringLiteral("Previous page  (Left / PageUp)"));
    connect(prevAct, &QAction::triggered, this, &PdfViewerWidget::onPrevPage);

    // Page spin box
    m_spinBox = new QSpinBox(this);
    m_spinBox->setMinimum(1);
    m_spinBox->setMaximum(1);
    m_spinBox->setValue(1);
    m_spinBox->setFixedWidth(52);
    m_spinBox->setAlignment(Qt::AlignCenter);
    m_spinBox->setStyleSheet(QStringLiteral(
        "QSpinBox { color: #cccccc; background: #3c3c3c; border: 1px solid #555; "
        "border-radius: 3px; padding: 1px 2px; }"
        "QSpinBox::up-button, QSpinBox::down-button { width: 0; }"));
    toolbar->addWidget(m_spinBox);

    m_pageLabel = new QLabel(QStringLiteral(" / 0"), this);
    m_pageLabel->setStyleSheet(QStringLiteral("color: #aaaaaa; margin-right: 4px;"));
    toolbar->addWidget(m_pageLabel);

    auto* nextAct = toolbar->addAction(QStringLiteral("▶"));
    nextAct->setToolTip(QStringLiteral("Next page  (Right / PageDown)"));
    connect(nextAct, &QAction::triggered, this, &PdfViewerWidget::onNextPage);

    toolbar->addSeparator();

    // Zoom out / combo / zoom in
    auto* zoomOutAct = toolbar->addAction(QStringLiteral("−"));
    zoomOutAct->setToolTip(QStringLiteral("Zoom out  (Ctrl+−)"));
    connect(zoomOutAct, &QAction::triggered, this, &PdfViewerWidget::onZoomOut);

    m_zoomCombo = new QComboBox(this);
    m_zoomCombo->setFixedWidth(110);
    m_zoomCombo->setEditable(true);
    m_zoomCombo->setInsertPolicy(QComboBox::NoInsert);
    m_zoomCombo->setStyleSheet(QStringLiteral(
        "QComboBox { color: #cccccc; background: #3c3c3c; border: 1px solid #555; "
        "border-radius: 3px; padding: 1px 4px; }"
        "QComboBox::drop-down { border: none; }"
        "QComboBox QAbstractItemView { background: #3c3c3c; color: #cccccc; }"));
    for (const auto& p : kZoomPresets) {
        m_zoomCombo->addItem(p.first, p.second);
    }
    m_zoomCombo->setCurrentIndex(0);   // Default: Fit Width
    connect(m_zoomCombo, &QComboBox::currentIndexChanged,
            this, &PdfViewerWidget::onZoomComboChanged);
    toolbar->addWidget(m_zoomCombo);

    auto* zoomInAct = toolbar->addAction(QStringLiteral("+"));
    zoomInAct->setToolTip(QStringLiteral("Zoom in  (Ctrl++)"));
    connect(zoomInAct, &QAction::triggered, this, &PdfViewerWidget::onZoomIn);

    toolbar->addSeparator();

    // ---- View-mode toggle ----
    // Two checkable QPushButtons (Continuous / Single Page).
    const QString kBtnStyle = QStringLiteral(
        "QPushButton { color: #cccccc; background: #3c3c3c; border: 1px solid #555; "
        "border-radius: 3px; padding: 3px 8px; font-size: 12px; }"
        "QPushButton:hover { background: #4a4a4a; }"
        "QPushButton:checked { background: #0078d4; color: white; border-color: #0060b0; }");

    auto* btnCont = new QPushButton(QStringLiteral("Continuous"), this);
    btnCont->setCheckable(true);
    btnCont->setChecked(true);   // Default
    btnCont->setToolTip(QStringLiteral("Continuous vertical scroll"));
    btnCont->setStyleSheet(kBtnStyle);
    m_btnContinuous = btnCont;
    connect(btnCont, &QPushButton::clicked, this, &PdfViewerWidget::onContinuousModeClicked);
    toolbar->addWidget(btnCont);

    auto* btnSingle = new QPushButton(QStringLiteral("Single Page"), this);
    btnSingle->setCheckable(true);
    btnSingle->setChecked(false);
    btnSingle->setToolTip(QStringLiteral("Show one page at a time"));
    btnSingle->setStyleSheet(kBtnStyle);
    m_btnSinglePage = btnSingle;
    connect(btnSingle, &QPushButton::clicked, this, &PdfViewerWidget::onSinglePageModeClicked);
    toolbar->addWidget(btnSingle);

    // Spin box Enter
    connect(m_spinBox, &QSpinBox::editingFinished,
            this, &PdfViewerWidget::onSpinBoxCommitted);

    // Keyboard shortcuts
    auto* scLeft  = new QShortcut(QKeySequence(Qt::Key_Left),  this);
    auto* scRight = new QShortcut(QKeySequence(Qt::Key_Right), this);
    auto* scPgUp  = new QShortcut(QKeySequence(Qt::Key_PageUp), this);
    auto* scPgDn  = new QShortcut(QKeySequence(Qt::Key_PageDown), this);
    auto* scPlus  = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Plus), this);
    auto* scEqual = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Equal), this);
    auto* scMinus = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Minus), this);
    auto* scZero  = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_0), this);
    connect(scLeft,  &QShortcut::activated, this, &PdfViewerWidget::onPrevPage);
    connect(scPgUp,  &QShortcut::activated, this, &PdfViewerWidget::onPrevPage);
    connect(scRight, &QShortcut::activated, this, &PdfViewerWidget::onNextPage);
    connect(scPgDn,  &QShortcut::activated, this, &PdfViewerWidget::onNextPage);
    connect(scPlus,  &QShortcut::activated, this, &PdfViewerWidget::onZoomIn);
    connect(scEqual, &QShortcut::activated, this, &PdfViewerWidget::onZoomIn);
    connect(scMinus, &QShortcut::activated, this, &PdfViewerWidget::onZoomOut);
    connect(scZero,  &QShortcut::activated, this, [this]() {
        if (m_canvas) {
            m_canvas->setZoomPercent(100.0);
            updateZoomCombo();
        }
    });
}

// ---------------------------------------------------------------------------
// Document loading
// ---------------------------------------------------------------------------
void PdfViewerWidget::openDocument()
{
    auto renderer = PdfRendererFactory::create();
    m_renderService = std::make_unique<PdfRenderService>(std::move(renderer));

    connect(m_renderService.get(), &PdfRenderService::pageRendered,
            this, &PdfViewerWidget::onPageRendered,
            Qt::QueuedConnection);
    connect(m_renderService.get(), &PdfRenderService::renderError,
            this, &PdfViewerWidget::onRenderError,
            Qt::QueuedConnection);

    if (!m_renderService->open(m_filePath)) {
        updateStatus(QStringLiteral("Error: ") + m_renderService->lastError());
        return;
    }

    m_pageCount   = m_renderService->pageCount();
    m_currentPage = 0;

    qDebug() << "[PdfViewerWidget] document open:"
             << m_filePath << "pages=" << m_pageCount;

    // Gather page sizes.
    QVector<QSizeF> pageSizes(m_pageCount);
    for (int i = 0; i < m_pageCount; ++i) {
        pageSizes[i] = m_renderService->pageSizePoints(i);
    }

    // Initialise per-page version tracking.
    m_pageRenderVer.resize(m_pageCount);
    m_pageRenderVer.fill(0);
    m_pageRenderSize.resize(m_pageCount);
    m_pageRenderSize.fill(QSize());

    // Update controls.
    m_spinBox->setMaximum(qMax(1, m_pageCount));
    m_spinBox->setValue(1);
    m_pageLabel->setText(QStringLiteral(" / %1").arg(m_pageCount));

    // Setup thumbnail sidebar.
    m_sidebar->setup(m_renderService.get(), m_filePath, m_pageCount);

    // Hand all page sizes to the canvas; it will build the scene.
    m_canvas->setupPages(pageSizes);

    updateStatus();
    emit titleChanged(makeTitle());
}

// ---------------------------------------------------------------------------
// Navigation
// ---------------------------------------------------------------------------
void PdfViewerWidget::navigateToPage(int pageIndex)
{
    if (!m_renderService || !m_renderService->isOpen() || m_pageCount == 0) return;
    pageIndex = qBound(0, pageIndex, m_pageCount - 1);

    m_currentPage = pageIndex;
    m_canvas->setCurrentPage(pageIndex);
    m_sidebar->setCurrentPage(pageIndex);
    updatePageControls();
    updateStatus();
}

// ---------------------------------------------------------------------------
// Render dispatch
// ---------------------------------------------------------------------------
void PdfViewerWidget::requestRender(int pageIndex, int /*renderVer*/,
                                    QSize targetSize, double zoom)
{
    if (!m_renderService || !m_renderService->isOpen()) return;
    if (pageIndex < 0 || pageIndex >= m_pageCount)     return;

    RenderKey key;
    key.filePath    = m_filePath;
    key.pageIndex   = pageIndex;
    key.zoom        = static_cast<float>(zoom);
    key.targetW     = targetSize.width();
    key.targetH     = targetSize.height();
    key.rotationDeg = 0;
    key.quality     = RenderQuality::Balanced;

    m_renderService->requestRender(key);
}

void PdfViewerWidget::onRenderPageRequested(int pageIndex, int renderVer,
                                            QSize targetSize, double zoomFactor)
{
    if (pageIndex < 0 || pageIndex >= m_pageCount) return;

    // Record the version and size we're about to request.
    m_pageRenderVer[pageIndex]  = renderVer;
    m_pageRenderSize[pageIndex] = targetSize;

    requestRender(pageIndex, renderVer, targetSize, zoomFactor);
    updateStatus(QStringLiteral("Rendering…"));
}

void PdfViewerWidget::onPageRendered(const RenderKey& key, const QImage& image)
{
    if (key.filePath != m_filePath) return;

    const int pageIndex = key.pageIndex;

    // Thumbnail check: small renders go to the sidebar.
    if (key.targetW > 0 && key.targetW <= 200) {
        m_sidebar->onThumbnailRendered(key, image);
        return;
    }

    // Main page render — stale check against our per-page version.
    if (pageIndex < 0 || pageIndex >= m_pageCount) return;

    // A response is fresh if the requested size matches what we last asked for.
    const QSize expectedSize = m_pageRenderSize.value(pageIndex);
    if (QSize(key.targetW, key.targetH) != expectedSize) {
        return;   // Stale: a newer request supersedes this one.
    }

    // Route to canvas using the stored renderVer for that page.
    if (m_canvas && pageIndex < m_canvas->pageCount()) {
        m_canvas->showRenderedPage(pageIndex,
                                   m_pageRenderVer.value(pageIndex),
                                   image);
    }

    updateStatus(QStringLiteral("Backend: %1").arg(m_renderService->backendName()));
}

void PdfViewerWidget::onRenderError(const RenderKey& key, const QString& message)
{
    if (key.filePath != m_filePath) return;
    updateStatus(QStringLiteral("Render error: ") + message);
}

// ---------------------------------------------------------------------------
// Canvas signal handlers
// ---------------------------------------------------------------------------
void PdfViewerWidget::onZoomChanged(double pct)
{
    Q_UNUSED(pct);
    updateZoomCombo();
}

void PdfViewerWidget::onCurrentPageChanged(int pageIndex)
{
    m_currentPage = pageIndex;
    updatePageControls();
    m_sidebar->setCurrentPage(pageIndex);
    updateStatus();
}

// ---------------------------------------------------------------------------
// Toolbar slots
// ---------------------------------------------------------------------------
void PdfViewerWidget::onPrevPage()
{
    navigateToPage(m_currentPage - 1);
}

void PdfViewerWidget::onNextPage()
{
    navigateToPage(m_currentPage + 1);
}

void PdfViewerWidget::onZoomIn()
{
    if (!m_canvas) return;
    const double pct = m_canvas->zoomPercent();
    for (const auto& p : kZoomPresets) {
        if (p.second > pct + 0.5) {
            m_canvas->setZoomPercent(p.second);
            updateZoomCombo();
            return;
        }
    }
    m_canvas->setZoomPercent(qMin(pct + 25.0, 400.0));
    updateZoomCombo();
}

void PdfViewerWidget::onZoomOut()
{
    if (!m_canvas) return;
    const double pct = m_canvas->zoomPercent();
    for (int i = kZoomPresets.size() - 1; i >= 0; --i) {
        const double v = kZoomPresets[i].second;
        if (v > 0 && v < pct - 0.5) {
            m_canvas->setZoomPercent(v);
            updateZoomCombo();
            return;
        }
    }
    m_canvas->setZoomPercent(qMax(pct - 25.0, 25.0));
    updateZoomCombo();
}

void PdfViewerWidget::onZoomComboChanged(int index)
{
    if (!m_canvas) return;
    const double val = m_zoomCombo->itemData(index).toDouble();
    if (val == 0.0) { updateZoomCombo(); return; }
    if (val < 0) {
        if (val == -1.0) m_canvas->setZoomMode(ZoomMode::FitWidth);
        if (val == -2.0) m_canvas->setZoomMode(ZoomMode::FitPage);
    } else {
        m_canvas->setZoomPercent(val);
    }
}

void PdfViewerWidget::onSpinBoxCommitted()
{
    navigateToPage(m_spinBox->value() - 1);
}

void PdfViewerWidget::onToggleSidebar()
{
    if (!m_sidebar) return;
    const bool nowVisible = !m_sidebar->isVisible();
    m_sidebar->setVisible(nowVisible);
    if (m_sidebarAct) m_sidebarAct->setChecked(nowVisible);
}

void PdfViewerWidget::onContinuousModeClicked()
{
    if (m_canvas) m_canvas->setViewMode(PdfViewMode::Continuous);
    updateViewModeButtons();
    updateStatus();
}

void PdfViewerWidget::onSinglePageModeClicked()
{
    if (m_canvas) m_canvas->setViewMode(PdfViewMode::SinglePage);
    updateViewModeButtons();
    updateStatus();
}

// ---------------------------------------------------------------------------
// Key events
// ---------------------------------------------------------------------------
void PdfViewerWidget::keyPressEvent(QKeyEvent* event)
{
    if ((event->key() == Qt::Key_Tab || event->key() == Qt::Key_Backtab)
            && (event->modifiers() & Qt::ControlModifier)) {
        event->ignore();
        return;
    }
    QWidget::keyPressEvent(event);
}

// ---------------------------------------------------------------------------
// Helper updates
// ---------------------------------------------------------------------------
void PdfViewerWidget::updatePageControls()
{
    if (m_spinBox) {
        const bool b = m_spinBox->blockSignals(true);
        m_spinBox->setValue(m_currentPage + 1);
        m_spinBox->blockSignals(b);
    }
    if (m_pageLabel)
        m_pageLabel->setText(QStringLiteral(" / %1").arg(m_pageCount));
}

void PdfViewerWidget::updateZoomCombo()
{
    if (!m_canvas || !m_zoomCombo) return;
    const bool b = m_zoomCombo->blockSignals(true);
    const ZoomMode mode = m_canvas->zoomMode();
    if (mode == ZoomMode::FitWidth) {
        m_zoomCombo->setCurrentIndex(0);
    } else if (mode == ZoomMode::FitPage) {
        m_zoomCombo->setCurrentIndex(1);
    } else {
        const double pct = m_canvas->zoomPercent();
        bool found = false;
        for (int i = 0; i < kZoomPresets.size(); ++i) {
            if (kZoomPresets[i].second > 0
                    && qAbs(kZoomPresets[i].second - pct) < 0.5) {
                m_zoomCombo->setCurrentIndex(i);
                found = true;
                break;
            }
        }
        if (!found) {
            m_zoomCombo->setEditText(QStringLiteral("%1%").arg(
                static_cast<int>(qRound(pct))));
        }
    }
    m_zoomCombo->blockSignals(b);
}

void PdfViewerWidget::updateViewModeButtons()
{
    if (!m_canvas) return;
    const bool isCont = (m_canvas->viewMode() == PdfViewMode::Continuous);
    if (m_btnContinuous) m_btnContinuous->setChecked(isCont);
    if (m_btnSinglePage) m_btnSinglePage->setChecked(!isCont);
}

void PdfViewerWidget::updateStatus(const QString& extra)
{
    if (!m_statusLabel) return;
    QString text;
    if (m_pageCount > 0) {
        const QString modeStr = (m_canvas && m_canvas->viewMode() == PdfViewMode::SinglePage)
                                ? QStringLiteral("Single Page")
                                : QStringLiteral("Continuous");
        text = QStringLiteral("Page %1 of %2  •  %3")
               .arg(m_currentPage + 1).arg(m_pageCount).arg(modeStr);
    } else {
        text = QStringLiteral("No document");
    }
    if (!extra.isEmpty()) text += QStringLiteral("  •  ") + extra;
    m_statusLabel->setText(text);
}

QString PdfViewerWidget::makeTitle() const
{
    return QFileInfo(m_filePath).fileName();
}
