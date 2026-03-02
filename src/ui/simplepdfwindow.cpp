#include "simplepdfwindow.h"
#include "pdftabwidget.h"

#include <QPdfDocument>
#include <QPdfLink>
#include <QPdfView>
#include <QPdfPageNavigator>
#include <QPdfSearchModel>
#include <QPdfBookmarkModel>

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QMetaObject>
#include <QModelIndex>
#include <QPointF>
#include <QSettings>
#include <QScrollBar>
#include <QSpinBox>
#include <QStatusBar>
#include <QTimer>
#include <QPointer>
#include <QToolBar>
#include <QTreeView>
#include <QUrl>
#include <QWheelEvent>

SimplePdfWindow::SimplePdfWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setAcceptDrops(true);
    createUi();
    createMenuBar();
    restoreWindowGeometry();
    restoreViewerPreferences();
}

void SimplePdfWindow::openFilePath(const QString &filePath)
{
    if (!filePath.isEmpty() && m_viewer)
        m_viewer->loadFile(filePath);
}

// ---------------------------------------------------------------------------
// UI
// ---------------------------------------------------------------------------

void SimplePdfWindow::createUi()
{
    m_viewer = new PdfTabWidget(this);
    setCentralWidget(m_viewer);

    connect(m_viewer, &PdfTabWidget::documentLoaded, this, [this](const QString &) {
        updateWindowTitle();
        updatePageControls();
        updateStatusLabel();
    });
    connect(m_viewer, &PdfTabWidget::pageChanged, this, [this]() {
        updatePageControls();
        updateStatusLabel();
    });
    connect(m_viewer, &PdfTabWidget::zoomChanged, this, [this]() { updateStatusLabel(); });
    connect(m_viewer->document(), &QPdfDocument::statusChanged, this, [this]() {
        if (m_viewer->document()->status() == QPdfDocument::Status::Ready) {
            updatePageControls();
            updateStatusLabel();
        }
    });
    connect(m_viewer->tocView(), &QTreeView::clicked, this, &SimplePdfWindow::onBookmarkActivated);

    m_viewer->view()->viewport()->installEventFilter(this);

    QToolBar *tb = addToolBar(tr("Main"));
    tb->setMovable(false);

    tb->addAction(tr("Open"), this, &SimplePdfWindow::openFile);

    tb->addSeparator();
    tb->addAction(tr("Prev"), this, &SimplePdfWindow::goPreviousPage);
    tb->addAction(tr("Next"), this, &SimplePdfWindow::goNextPage);

    m_pageSpin = new QSpinBox(this);
    m_pageSpin->setMinimum(1);
    m_pageSpin->setMaximum(1);
    m_pageSpin->setValue(1);
    m_pageSpin->setFixedWidth(60);
    connect(m_pageSpin, &QSpinBox::editingFinished, this, &SimplePdfWindow::gotoPageFromSpin);
    tb->addWidget(m_pageSpin);
    m_pageCountLabel = new QLabel(QStringLiteral("/ 0"), this);
    tb->addWidget(m_pageCountLabel);

    tb->addSeparator();
    tb->addAction(tr("Zoom In"), this, &SimplePdfWindow::zoomIn);
    tb->addAction(tr("Zoom Out"), this, &SimplePdfWindow::zoomOut);
    tb->addAction(tr("Fit Width"), this, &SimplePdfWindow::fitWidth);
    tb->addAction(tr("Fit Page"), this, &SimplePdfWindow::fitPage);
    tb->addAction(tr("Reset Zoom"), this, &SimplePdfWindow::resetZoom);

    m_statusLabel = new QLabel(tr("Ready"), this);
    statusBar()->addWidget(m_statusLabel);

    setWindowTitle(tr("PDF Viewer"));
    resize(900, 700);
}

void SimplePdfWindow::createMenuBar()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    QAction *openAct = fileMenu->addAction(tr("&Open..."));
    openAct->setShortcut(QKeySequence::Open);
    connect(openAct, &QAction::triggered, this, &SimplePdfWindow::openFile);
    fileMenu->addSeparator();
    QAction *exitAct = fileMenu->addAction(tr("E&xit"));
    exitAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q));
    connect(exitAct, &QAction::triggered, this, &QWidget::close);

    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
    m_continuousScrollAction = viewMenu->addAction(tr("&Continuous Scroll"));
    m_continuousScrollAction->setCheckable(true);
    m_continuousScrollAction->setChecked(false);
    connect(m_continuousScrollAction, &QAction::toggled, this, &SimplePdfWindow::setContinuousScroll);
    viewMenu->addSeparator();
    viewMenu->addAction(tr("Zoom &In"), this, &SimplePdfWindow::zoomIn)->setShortcut(QKeySequence::ZoomIn);
    viewMenu->addAction(tr("Zoom &Out"), this, &SimplePdfWindow::zoomOut)->setShortcut(QKeySequence::ZoomOut);
    viewMenu->addAction(tr("Fit &Width"), this, &SimplePdfWindow::fitWidth);
    viewMenu->addAction(tr("Fit &Page"), this, &SimplePdfWindow::fitPage);
    viewMenu->addAction(tr("&Reset Zoom"), this, &SimplePdfWindow::resetZoom)->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_0));

    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(tr("&About"), this, &SimplePdfWindow::showAbout);

    addAction(tr("Prev Page"), this, &SimplePdfWindow::goPreviousPage)->setShortcut(QKeySequence(Qt::Key_PageUp));
    addAction(tr("Next Page"), this, &SimplePdfWindow::goNextPage)->setShortcut(QKeySequence(Qt::Key_PageDown));
    addAction(tr("First Page"), this, &SimplePdfWindow::goFirstPage)->setShortcut(QKeySequence(Qt::Key_Home));
    addAction(tr("Last Page"), this, &SimplePdfWindow::goLastPage)->setShortcut(QKeySequence(Qt::Key_End));
}

// ---------------------------------------------------------------------------
// File
// ---------------------------------------------------------------------------

void SimplePdfWindow::openFile()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("Open PDF"), QString(), tr("PDF Files (*.pdf)"));
    if (!path.isEmpty())
        openFilePath(path);
}

// ---------------------------------------------------------------------------
// Page navigation
// ---------------------------------------------------------------------------

void SimplePdfWindow::goNextPage()
{
    if (!m_viewer || !m_viewer->hasDocument() || !m_viewer->navigator())
        return;
    const int page = m_viewer->navigator()->currentPage();
    if (page + 1 < m_viewer->document()->pageCount()) {
        m_viewer->navigator()->jump(page + 1, QPointF(), 0);
        updatePageControls();
        updateStatusLabel();
    }
}

void SimplePdfWindow::goPreviousPage()
{
    if (!m_viewer || !m_viewer->hasDocument() || !m_viewer->navigator())
        return;
    const int page = m_viewer->navigator()->currentPage();
    if (page > 0) {
        m_viewer->navigator()->jump(page - 1, QPointF(), 0);
        updatePageControls();
        updateStatusLabel();
    }
}

void SimplePdfWindow::goFirstPage()
{
    if (!m_viewer || !m_viewer->hasDocument() || !m_viewer->navigator())
        return;
    m_viewer->navigator()->jump(0, QPointF(), 0);
    updatePageControls();
    updateStatusLabel();
}

void SimplePdfWindow::goLastPage()
{
    if (!m_viewer || !m_viewer->hasDocument() || !m_viewer->navigator())
        return;
    m_viewer->navigator()->jump(m_viewer->document()->pageCount() - 1, QPointF(), 0);
    updatePageControls();
    updateStatusLabel();
}

void SimplePdfWindow::gotoPageFromSpin()
{
    if (!m_viewer || !m_viewer->hasDocument() || !m_viewer->navigator())
        return;
    const int target = m_pageSpin->value() - 1;
    if (target >= 0 && target < m_viewer->document()->pageCount()) {
        m_viewer->navigator()->jump(target, QPointF(), 0);
        updatePageControls();
        updateStatusLabel();
    }
}

// ---------------------------------------------------------------------------
// Zoom
// ---------------------------------------------------------------------------

void SimplePdfWindow::zoomIn()
{
    if (!m_viewer) return;
    const double newZoom = qBound(0.1, m_viewer->view()->zoomFactor() * 1.25, 10.0);
    if (m_viewer->view()->pageMode() == QPdfView::PageMode::MultiPage) {
        const int page = m_viewer->navigator()->currentPage();
        m_viewer->view()->setZoomMode(QPdfView::ZoomMode::Custom);
        m_viewer->view()->setZoomFactor(newZoom);
        scheduleZoomAnchorJump(m_viewer, page);
    } else {
        zoomAnchoredToViewportCenter(m_viewer, newZoom);
    }
    updateStatusLabel();
}

void SimplePdfWindow::zoomOut()
{
    if (!m_viewer) return;
    const double newZoom = qBound(0.1, m_viewer->view()->zoomFactor() / 1.25, 10.0);
    if (m_viewer->view()->pageMode() == QPdfView::PageMode::MultiPage) {
        const int page = m_viewer->navigator()->currentPage();
        m_viewer->view()->setZoomMode(QPdfView::ZoomMode::Custom);
        m_viewer->view()->setZoomFactor(newZoom);
        scheduleZoomAnchorJump(m_viewer, page);
    } else {
        zoomAnchoredToViewportCenter(m_viewer, newZoom);
    }
    updateStatusLabel();
}

void SimplePdfWindow::fitWidth()
{
    if (!m_viewer) return;
    const int page = m_viewer->navigator()->currentPage();
    m_viewer->view()->setZoomMode(QPdfView::ZoomMode::FitToWidth);
    scheduleZoomAnchorJump(m_viewer, page);
    updateStatusLabel();
}

void SimplePdfWindow::fitPage()
{
    if (!m_viewer) return;
    const int page = m_viewer->navigator()->currentPage();
    m_viewer->view()->setZoomMode(QPdfView::ZoomMode::FitInView);
    scheduleZoomAnchorJump(m_viewer, page);
    updateStatusLabel();
}

void SimplePdfWindow::resetZoom()
{
    if (!m_viewer) return;
    const int page = m_viewer->navigator()->currentPage();
    m_viewer->view()->setZoomMode(QPdfView::ZoomMode::Custom);
    m_viewer->view()->setZoomFactor(1.0);
    scheduleZoomAnchorJump(m_viewer, page);
    updateStatusLabel();
}

void SimplePdfWindow::scheduleZoomAnchorJump(PdfTabWidget *tab, int page)
{
    if (!tab || !tab->navigator()) return;
    QPointer<PdfTabWidget> p(tab);
    QTimer::singleShot(0, this, [p, page]() {
        if (p && p->navigator())
            p->navigator()->jump(page, QPointF(), 0);
    });
}

void SimplePdfWindow::zoomAnchoredToViewportCenter(PdfTabWidget *tab, double newZoom)
{
    if (!tab || !tab->view()) return;
    QPdfView *v = tab->view();
    const double oldZoom = v->zoomFactor();
    if (oldZoom <= 0) return;

    QPoint center = v->viewport()->rect().center();
    const double centerContentX = v->horizontalScrollBar()->value() + center.x();
    const double centerContentY = v->verticalScrollBar()->value() + center.y();
    const double zoomRatio = newZoom / oldZoom;
    const int targetScrollX = qRound(centerContentX * zoomRatio - center.x());
    const int targetScrollY = qRound(centerContentY * zoomRatio - center.y());

    v->setZoomMode(QPdfView::ZoomMode::Custom);
    v->setZoomFactor(newZoom);

    QPointer<QPdfView> pView(v);
    auto *connection = new QMetaObject::Connection;
    *connection = connect(v, &QPdfView::zoomFactorChanged, this, [this, connection, pView, targetScrollX, targetScrollY]() {
        disconnect(*connection);
        delete connection;
        if (!pView) return;
        QTimer::singleShot(0, pView, [pView, targetScrollX, targetScrollY]() {
            if (!pView) return;
            QScrollBar *hBar = pView->horizontalScrollBar();
            QScrollBar *vBar = pView->verticalScrollBar();
            hBar->setValue(qBound(0, targetScrollX, hBar->maximum()));
            vBar->setValue(qBound(0, targetScrollY, vBar->maximum()));
        });
    });
}

// ---------------------------------------------------------------------------
// Bookmarks
// ---------------------------------------------------------------------------

void SimplePdfWindow::onBookmarkActivated(const QModelIndex &index)
{
    if (!index.isValid() || !m_viewer || !m_viewer->navigator()) return;
    const QVariant pageVar = m_viewer->bookmarkModel()->data(index, static_cast<int>(QPdfBookmarkModel::Role::Page));
    const int page = pageVar.toInt();
    if (page >= 0 && page < m_viewer->document()->pageCount()) {
        m_viewer->navigator()->jump(page, QPointF(), 0);
        updatePageControls();
        updateStatusLabel();
    }
}

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------

void SimplePdfWindow::updateWindowTitle()
{
    if (!m_viewer || m_viewer->filePath().isEmpty()) {
        setWindowTitle(tr("PDF Viewer"));
        return;
    }
    setWindowTitle(tr("%1 - PDF Viewer").arg(QFileInfo(m_viewer->filePath()).fileName()));
}

void SimplePdfWindow::updatePageControls()
{
    const int pageCount = (m_viewer && m_viewer->hasDocument()) ? m_viewer->document()->pageCount() : 0;
    m_pageSpin->setMaximum(qMax(1, pageCount));
    const int current = (m_viewer && m_viewer->navigator()) ? m_viewer->navigator()->currentPage() : 0;
    m_pageSpin->setValue(pageCount > 0 ? current + 1 : 1);
    m_pageCountLabel->setText(QStringLiteral("/ %1").arg(pageCount));
}

void SimplePdfWindow::updateStatusLabel()
{
    if (!m_viewer || !m_viewer->hasDocument()) {
        m_statusLabel->setText(tr("No document"));
        return;
    }
    const int current = m_viewer->navigator() ? (m_viewer->navigator()->currentPage() + 1) : 1;
    const int total = m_viewer->document()->pageCount();
    const double zoom = m_viewer->view()->zoomFactor() * 100.0;
    m_statusLabel->setText(tr("Page %1 of %2  \u2022  Zoom %3%").arg(current).arg(total).arg(qRound(zoom)));
}

// ---------------------------------------------------------------------------
// Geometry
// ---------------------------------------------------------------------------

void SimplePdfWindow::saveWindowGeometry()
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("window"));
    settings.setValue(QStringLiteral("geometry"), saveGeometry());
    settings.setValue(QStringLiteral("state"), saveState());
    settings.setValue(QStringLiteral("continuousScroll"), m_continuousScrollAction && m_continuousScrollAction->isChecked());
    settings.endGroup();
}

void SimplePdfWindow::restoreWindowGeometry()
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("window"));
    const QByteArray geom = settings.value(QStringLiteral("geometry")).toByteArray();
    const QByteArray state = settings.value(QStringLiteral("state")).toByteArray();
    settings.endGroup();
    if (!geom.isEmpty()) restoreGeometry(geom);
    if (!state.isEmpty()) restoreState(state);
}

void SimplePdfWindow::restoreViewerPreferences()
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("window"));
    const bool continuous = settings.value(QStringLiteral("continuousScroll"), false).toBool();
    settings.endGroup();
    setContinuousScroll(continuous);
}

void SimplePdfWindow::setContinuousScroll(bool enabled)
{
    if (!m_viewer) return;
    m_viewer->view()->setPageMode(enabled ? QPdfView::PageMode::MultiPage : QPdfView::PageMode::SinglePage);
    if (m_continuousScrollAction && m_continuousScrollAction->isChecked() != enabled)
        m_continuousScrollAction->setChecked(enabled);
    QSettings settings;
    settings.beginGroup(QStringLiteral("window"));
    settings.setValue(QStringLiteral("continuousScroll"), enabled);
    settings.endGroup();
}

// ---------------------------------------------------------------------------
// Events
// ---------------------------------------------------------------------------

void SimplePdfWindow::closeEvent(QCloseEvent *event)
{
    saveWindowGeometry();
    QMainWindow::closeEvent(event);
}

bool SimplePdfWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (m_viewer && obj == m_viewer->view()->viewport() && event->type() == QEvent::Wheel) {
        auto *we = static_cast<QWheelEvent *>(event);
        if (we->modifiers() & Qt::ControlModifier && we->angleDelta().y() != 0) {
            const double factor = (we->angleDelta().y() > 0) ? 1.1 : (1.0 / 1.1);
            const double newZoom = qBound(0.1, m_viewer->view()->zoomFactor() * factor, 10.0);
            if (m_viewer->view()->pageMode() == QPdfView::PageMode::MultiPage) {
                const int page = m_viewer->navigator()->currentPage();
                m_viewer->view()->setZoomMode(QPdfView::ZoomMode::Custom);
                m_viewer->view()->setZoomFactor(newZoom);
                scheduleZoomAnchorJump(m_viewer, page);
            } else {
                zoomAnchoredToViewportCenter(m_viewer, newZoom);
            }
            updateStatusLabel();
            return true;
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void SimplePdfWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        for (const QUrl &url : event->mimeData()->urls()) {
            if (url.isLocalFile() && url.toLocalFile().endsWith(QLatin1String(".pdf"), Qt::CaseInsensitive)) {
                event->acceptProposedAction();
                return;
            }
        }
    }
}

void SimplePdfWindow::dropEvent(QDropEvent *event)
{
    for (const QUrl &url : event->mimeData()->urls()) {
        const QString path = url.toLocalFile();
        if (path.endsWith(QLatin1String(".pdf"), Qt::CaseInsensitive)) {
            openFilePath(path);
            break;
        }
    }
    event->acceptProposedAction();
}

// ---------------------------------------------------------------------------
// Help
// ---------------------------------------------------------------------------

void SimplePdfWindow::showAbout()
{
    QMessageBox::about(this, tr("About PDF Viewer"),
        tr("<h3>PDF Viewer</h3><p>Single-document PDF viewer. Qt %1.</p>").arg(QString::fromLatin1(qVersion())));
}
