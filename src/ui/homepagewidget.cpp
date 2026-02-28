#include "homepagewidget.h"
#include "locationssidebarwidget.h"
#include "homecontentwidget.h"
#include <QSplitter>
#include <QVBoxLayout>
#include <QStandardPaths>

HomePageWidget::HomePageWidget(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("HomePageWidget"));
    setupUI();
}

HomePageWidget::~HomePageWidget()
{
}

void HomePageWidget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // Splitter: sidebar (left) + content (right, switches between Recent/Browse)
    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->setObjectName(QStringLiteral("HomeSplitter"));
    m_splitter->setChildrenCollapsible(false);

    m_sidebar = new LocationsSidebarWidget(this);
    m_sidebar->setMinimumWidth(160);
    m_sidebar->setMaximumWidth(220);
    m_splitter->addWidget(m_sidebar);

    m_content = new HomeContentWidget(this);
    m_splitter->addWidget(m_content);

    // Sidebar ~20%, content ~80%
    m_splitter->setSizes({ 180, 620 });

    connect(m_sidebar, &LocationsSidebarWidget::locationSelected,
            this, &HomePageWidget::onLocationSelected);
    connect(m_content, &HomeContentWidget::openFileRequested,
            this, &HomePageWidget::openFileRequested);

    mainLayout->addWidget(m_splitter);
}

void HomePageWidget::onLocationSelected(const QString &path)
{
    m_content->setRootPath(path);
}

void HomePageWidget::focusSearchBar()
{
    if (m_content) {
        m_content->focusSearchBar();
    }
}
