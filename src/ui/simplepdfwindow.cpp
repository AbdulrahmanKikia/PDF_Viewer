#include "simplepdfwindow.h"

#include <QPdfDocument>
#include <QPdfView>
#include <QPdfPageNavigator>
#include <QPdfSearchModel>
#include <QPdfBookmarkModel>

#include <QAction>
#include <QFileDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QSpinBox>
#include <QStatusBar>
#include <QToolBar>
#include <QTreeView>
#include <QSplitter>
#include <QVBoxLayout>
#include <QFileInfo>
#include <QPointF>
#include <QModelIndex>
#include <QWheelEvent>

SimplePdfWindow::SimplePdfWindow(QWidget *parent)
    : QMainWindow(parent)
{
    createUi();
}

void SimplePdfWindow::createUi()
{
    // Core QtPdf objects (use view's page navigator, do not create a separate one)
    m_document = new QPdfDocument(this);
    m_view     = new QPdfView(this);
    m_searchModel = new QPdfSearchModel(this);
    m_bookmarkModel = new QPdfBookmarkModel(this);

    m_view->setDocument(m_document);
    m_navigator = m_view->pageNavigator(); // use the view's navigator so page changes stay in sync
    m_view->setPageMode(QPdfView::PageMode::MultiPage);
    m_view->setZoomMode(QPdfView::ZoomMode::FitInView);
    m_view->setSearchModel(m_searchModel);

    m_searchModel->setDocument(m_document);
    m_bookmarkModel->setDocument(m_document);

    connect(m_document, &QPdfDocument::statusChanged,
            this, &SimplePdfWindow::onDocumentStatusChanged);

    // Ctrl+wheel zoom on the PDF viewport
    m_view->viewport()->installEventFilter(this);

    // Keep page counter and status bar in sync when scrolling or zooming
    if (m_navigator) {
        connect(m_navigator, &QPdfPageNavigator::currentPageChanged,
                this, &SimplePdfWindow::onCurrentPageChanged);
    }
    connect(m_view, &QPdfView::zoomFactorChanged,
            this, &SimplePdfWindow::updateStatusLabel);

    // TOC sidebar
    m_tocView = new QTreeView(this);
    m_tocView->setModel(m_bookmarkModel);
    m_tocView->setHeaderHidden(true);
    connect(m_tocView, &QTreeView::clicked,
            this, &SimplePdfWindow::onBookmarkActivated);

    // Splitter: TOC | View
    m_splitter = new QSplitter(this);
    m_splitter->addWidget(m_tocView);
    m_splitter->addWidget(m_view);
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);
    m_splitter->setSizes({220, 780});

    setCentralWidget(m_splitter);

    // Toolbar
    QToolBar *tb = addToolBar(tr("Main"));
    tb->setMovable(false);

    QAction *openAct = tb->addAction(tr("Open"));
    connect(openAct, &QAction::triggered, this, &SimplePdfWindow::openFile);

    tb->addSeparator();

    QAction *prevAct = tb->addAction(tr("Prev"));
    connect(prevAct, &QAction::triggered, this, &SimplePdfWindow::goPreviousPage);

    QAction *nextAct = tb->addAction(tr("Next"));
    connect(nextAct, &QAction::triggered, this, &SimplePdfWindow::goNextPage);

    m_pageSpin = new QSpinBox(this);
    m_pageSpin->setMinimum(1);
    m_pageSpin->setMaximum(1);
    m_pageSpin->setValue(1);
    m_pageSpin->setFixedWidth(60);
    connect(m_pageSpin, &QSpinBox::editingFinished,
            this, &SimplePdfWindow::gotoPageFromSpin);
    tb->addWidget(m_pageSpin);

    m_pageCountLabel = new QLabel(QStringLiteral("/ 0"), this);
    tb->addWidget(m_pageCountLabel);

    tb->addSeparator();

    QAction *zoomInAct = tb->addAction(tr("Zoom In"));
    connect(zoomInAct, &QAction::triggered, this, &SimplePdfWindow::zoomIn);

    QAction *zoomOutAct = tb->addAction(tr("Zoom Out"));
    connect(zoomOutAct, &QAction::triggered, this, &SimplePdfWindow::zoomOut);

    QAction *fitWidthAct = tb->addAction(tr("Fit Width"));
    connect(fitWidthAct, &QAction::triggered, this, &SimplePdfWindow::fitWidth);

    QAction *fitPageAct = tb->addAction(tr("Fit Page"));
    connect(fitPageAct, &QAction::triggered, this, &SimplePdfWindow::fitPage);

    QAction *resetZoomAct = tb->addAction(tr("Reset Zoom"));
    connect(resetZoomAct, &QAction::triggered, this, &SimplePdfWindow::resetZoom);

    tb->addSeparator();

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("Search text"));
    m_searchEdit->setClearButtonEnabled(true);
    connect(m_searchEdit, &QLineEdit::textChanged,
            this, &SimplePdfWindow::onSearchTextChanged);
    tb->addWidget(m_searchEdit);

    QAction *findNextAct = tb->addAction(tr("Next Match"));
    connect(findNextAct, &QAction::triggered, this, &SimplePdfWindow::findNext);

    QAction *findPrevAct = tb->addAction(tr("Prev Match"));
    connect(findPrevAct, &QAction::triggered, this, &SimplePdfWindow::findPrevious);

    // Status bar
    m_statusLabel = new QLabel(tr("Ready"), this);
    statusBar()->addWidget(m_statusLabel);

    setWindowTitle(tr("PDFViewer - Simple"));
    resize(1200, 800);
}

void SimplePdfWindow::openFile()
{
    const QString filePath = QFileDialog::getOpenFileName(
        this, tr("Open PDF File"), QString(), tr("PDF Files (*.pdf)"));
    if (filePath.isEmpty())
        return;

    const QPdfDocument::Error err = m_document->load(filePath);
    if (err != QPdfDocument::Error::None) {
        QMessageBox::warning(this, tr("Open Failed"),
                             tr("Could not open the PDF file:\n%1").arg(filePath));
        m_document->close();
        return;
    }

    m_currentFilePath = filePath;
    if (m_navigator)
        m_navigator->jump(0, QPointF(), 0);
    updateWindowTitle(filePath);
    updatePageControls();
    updateStatusLabel();
}

void SimplePdfWindow::goNextPage()
{
    if (!m_document || m_document->pageCount() == 0 || !m_navigator)
        return;
    const int page = m_navigator->currentPage();
    if (page + 1 < m_document->pageCount()) {
        m_navigator->jump(page + 1, QPointF(), 0);
        updatePageControls();
        updateStatusLabel();
    }
}

void SimplePdfWindow::goPreviousPage()
{
    if (!m_document || m_document->pageCount() == 0 || !m_navigator)
        return;
    const int page = m_navigator->currentPage();
    if (page > 0) {
        m_navigator->jump(page - 1, QPointF(), 0);
        updatePageControls();
        updateStatusLabel();
    }
}

void SimplePdfWindow::gotoPageFromSpin()
{
    if (!m_document || m_document->pageCount() == 0 || !m_navigator)
        return;
    const int target = m_pageSpin->value() - 1;
    if (target >= 0 && target < m_document->pageCount()) {
        m_navigator->jump(target, QPointF(), 0);
        updatePageControls();
        updateStatusLabel();
    }
}

void SimplePdfWindow::zoomIn()
{
    m_view->setZoomMode(QPdfView::ZoomMode::Custom);
    m_view->setZoomFactor(m_view->zoomFactor() * 1.25);
}

void SimplePdfWindow::zoomOut()
{
    m_view->setZoomMode(QPdfView::ZoomMode::Custom);
    m_view->setZoomFactor(m_view->zoomFactor() / 1.25);
}

void SimplePdfWindow::fitWidth()
{
    m_view->setZoomMode(QPdfView::ZoomMode::FitToWidth);
    updateStatusLabel();
}

void SimplePdfWindow::fitPage()
{
    m_view->setZoomMode(QPdfView::ZoomMode::FitInView);
    updateStatusLabel();
}

void SimplePdfWindow::resetZoom()
{
    m_view->setZoomMode(QPdfView::ZoomMode::Custom);
    m_view->setZoomFactor(1.0);
    updateStatusLabel();
}

void SimplePdfWindow::onSearchTextChanged(const QString &text)
{
    m_searchModel->setSearchString(text);
    if (!text.isEmpty())
        findNext();
    else
        updateStatusLabel();
}

void SimplePdfWindow::findNext()
{
    if (!m_view || !m_searchModel)
        return;
    const int count = m_searchModel->rowCount(QModelIndex());
    if (count <= 0)
        return;
    int idx = m_view->currentSearchResultIndex();
    idx = (idx + 1) % count;
    m_view->setCurrentSearchResultIndex(idx);
    updateStatusLabel();
}

void SimplePdfWindow::findPrevious()
{
    if (!m_view || !m_searchModel)
        return;
    const int count = m_searchModel->rowCount(QModelIndex());
    if (count <= 0)
        return;
    int idx = m_view->currentSearchResultIndex();
    idx = (idx - 1 + count) % count;
    m_view->setCurrentSearchResultIndex(idx);
    updateStatusLabel();
}

void SimplePdfWindow::onBookmarkActivated(const QModelIndex &index)
{
    if (!index.isValid() || !m_navigator)
        return;
    const QVariant pageVar = m_bookmarkModel->data(
        index, static_cast<int>(QPdfBookmarkModel::Role::Page));
    const int page = pageVar.toInt();
    if (page >= 0 && page < m_document->pageCount()) {
        m_navigator->jump(page, QPointF(), 0);
        updatePageControls();
        updateStatusLabel();
    }
}

void SimplePdfWindow::onDocumentStatusChanged()
{
    if (!m_document)
        return;
    if (m_document->status() == QPdfDocument::Status::Ready) {
        updatePageControls();
        updateStatusLabel();
    }
}

void SimplePdfWindow::updateWindowTitle(const QString &filePath)
{
    const QFileInfo fi(filePath);
    setWindowTitle(tr("%1 - PDFViewer").arg(fi.fileName()));
}

void SimplePdfWindow::updatePageControls()
{
    const int pageCount = m_document ? m_document->pageCount() : 0;
    m_pageSpin->setMaximum(qMax(1, pageCount));
    const int current = m_navigator ? m_navigator->currentPage() : 0;
    m_pageSpin->setValue(pageCount > 0 ? current + 1 : 1);
    m_pageCountLabel->setText(QStringLiteral("/ %1").arg(pageCount));
}

void SimplePdfWindow::updateStatusLabel()
{
    if (!m_document || m_document->pageCount() == 0) {
        m_statusLabel->setText(tr("No document"));
        return;
    }
    const int current = m_navigator ? (m_navigator->currentPage() + 1) : 1;
    const int total   = m_document->pageCount();
    const double zoom = m_view->zoomFactor() * 100.0;
    m_statusLabel->setText(
        tr("Page %1 of %2  •  Zoom %3%")
            .arg(current)
            .arg(total)
            .arg(QString::number(static_cast<int>(zoom))));
}

bool SimplePdfWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_view->viewport() && event->type() == QEvent::Wheel) {
        auto *we = static_cast<QWheelEvent *>(event);
        if (we->modifiers() & Qt::ControlModifier) {
            const int delta = we->angleDelta().y();
            if (delta == 0)
                return QMainWindow::eventFilter(obj, event);

            m_view->setZoomMode(QPdfView::ZoomMode::Custom);
            const double factor = (delta > 0) ? 1.1 : (1.0 / 1.1);
            const double newZoom = qBound(0.1, m_view->zoomFactor() * factor, 10.0);
            m_view->setZoomFactor(newZoom);
            return true; // consumed
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void SimplePdfWindow::onCurrentPageChanged()
{
    updatePageControls();
    updateStatusLabel();
}

