#include "pdftabwidget.h"

#include <QPdfDocument>
#include <QPdfView>
#include <QPdfPageNavigator>
#include <QPdfSearchModel>
#include <QPdfBookmarkModel>

#include <QMessageBox>
#include <QPointF>
#include <QSplitter>
#include <QTreeView>
#include <QVBoxLayout>

PdfTabWidget::PdfTabWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void PdfTabWidget::setupUi()
{
    m_document    = new QPdfDocument(this);
    m_view        = new QPdfView(this);
    m_searchModel = new QPdfSearchModel(this);
    m_bookmarkModel = new QPdfBookmarkModel(this);

    m_view->setDocument(m_document);
    m_navigator = m_view->pageNavigator();
    m_view->setPageMode(QPdfView::PageMode::SinglePage);
    m_view->setZoomMode(QPdfView::ZoomMode::FitInView);
    m_view->setSearchModel(m_searchModel);

    m_searchModel->setDocument(m_document);
    m_bookmarkModel->setDocument(m_document);

    if (m_navigator) {
        connect(m_navigator, &QPdfPageNavigator::currentPageChanged, this, [this]() {
            emit pageChanged(m_navigator->currentPage());
        });
    }
    connect(m_view, &QPdfView::zoomFactorChanged, this, [this]() {
        emit zoomChanged(m_view->zoomFactor());
    });

    m_tocView = new QTreeView(this);
    m_tocView->setModel(m_bookmarkModel);
    m_tocView->setHeaderHidden(true);

    m_splitter = new QSplitter(this);
    m_splitter->addWidget(m_tocView);
    m_splitter->addWidget(m_view);
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);
    m_splitter->setSizes({220, 780});

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_splitter);
}

bool PdfTabWidget::loadFile(const QString &filePath)
{
    const QPdfDocument::Error err = m_document->load(filePath);
    if (err != QPdfDocument::Error::None) {
        QMessageBox::warning(this, tr("Open Failed"),
                             tr("Could not open the PDF file:\n%1").arg(filePath));
        m_document->close();
        return false;
    }

    m_filePath = filePath;
    if (m_navigator)
        m_navigator->jump(0, QPointF(), 0);
    emit documentLoaded(filePath);
    return true;
}

void PdfTabWidget::reloadFile()
{
    if (m_filePath.isEmpty())
        return;

    const int page = m_navigator ? m_navigator->currentPage() : 0;
    const double zoom = m_view->zoomFactor();
    const auto zoomMode = m_view->zoomMode();

    m_document->close();
    if (m_document->load(m_filePath) == QPdfDocument::Error::None) {
        if (m_navigator && page < m_document->pageCount())
            m_navigator->jump(page, QPointF(), 0);
        m_view->setZoomMode(zoomMode);
        if (zoomMode == QPdfView::ZoomMode::Custom)
            m_view->setZoomFactor(zoom);
    }
}

bool PdfTabWidget::hasDocument() const
{
    return m_document && m_document->status() == QPdfDocument::Status::Ready;
}

int PdfTabWidget::currentPage() const
{
    return m_navigator ? m_navigator->currentPage() : 0;
}

double PdfTabWidget::zoomFactor() const
{
    return m_view ? m_view->zoomFactor() : 1.0;
}

void PdfTabWidget::setTocVisible(bool visible)
{
    m_tocView->setVisible(visible);
}

bool PdfTabWidget::isTocVisible() const
{
    return m_tocView->isVisible();
}
