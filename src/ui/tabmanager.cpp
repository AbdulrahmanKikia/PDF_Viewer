#include "tabmanager.h"
#include "homepagewidget.h"
#include "pdf/PdfViewerWidget.h"
#include "../config/recentfilesmanager.h"
#include <QTabWidget>
#include <QTabBar>
#include <QMenu>
#include <QAction>
#include <QFileInfo>
#include <QLabel>
#include <QVBoxLayout>
#include <QIcon>

// PDFViewerTab helper
PdfViewerWidget* PDFViewerTab::pdfViewer() const
{
    return qobject_cast<PdfViewerWidget*>(widget);
}

TabManager::TabManager(QTabWidget* tabWidget, QObject* parent)
    : QObject(parent)
    , m_tabWidget(tabWidget)
    , m_contextMenu(nullptr)
    , m_contextMenuTabIndex(-1)
{
    if (!m_tabWidget) {
        return;
    }

    // Enable close buttons on tabs
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setMovable(true);

    // Connect signals
    connect(m_tabWidget, &QTabWidget::tabCloseRequested,
            this, &TabManager::onTabCloseRequested);
    connect(m_tabWidget, &QTabWidget::currentChanged,
            this, &TabManager::onCurrentTabChanged);

    // Setup context menu for tab bar
    m_tabWidget->tabBar()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_tabWidget->tabBar(), &QTabBar::customContextMenuRequested,
            this, &TabManager::onTabContextMenuRequested);

    setupTabContextMenu();

    // Create initial home tab
    createNewTab(true);
}

TabManager::~TabManager()
{
    qDeleteAll(m_tabs);
}

void TabManager::createNewTab(bool isHome)
{
    QWidget* widget = nullptr;
    if (isHome) {
        HomePageWidget* homeWidget = new HomePageWidget(m_tabWidget);
        connect(homeWidget, &HomePageWidget::openFileRequested,
                this, &TabManager::openPDFInNewTab);
        widget = homeWidget;
    } else {
        widget = new QWidget(m_tabWidget);
    }

    QString title = isHome ? QStringLiteral("Home") : QStringLiteral("Untitled");

    PDFViewerTab* tab = new PDFViewerTab(widget, QString(), title, isHome);
    m_tabs.append(tab);

    int index = m_tabWidget->addTab(widget, title);
    m_tabWidget->setTabIcon(index, QIcon(isHome
        ? QStringLiteral(":/icons/app.svg")
        : QStringLiteral(":/icons/new.svg")));
    updateTabBar();
    m_tabWidget->setCurrentIndex(index);
    emit tabCountChanged(m_tabs.size());
}

void TabManager::openPDFInNewTab(const QString& filePath)
{
    if (filePath.isEmpty()) {
        return;
    }

    QFileInfo fileInfo(filePath);
    QString title = fileInfo.fileName();
    if (title.length() > 24) {
        title = title.left(21) + QStringLiteral("...");
    }

    // Add to recent files
    RecentFilesManager::instance()->addFile(filePath);

    // Real PDF viewer widget
    auto* viewer = new PdfViewerWidget(filePath, m_tabWidget);

    PDFViewerTab* tab = new PDFViewerTab(viewer, filePath, title, false);
    m_tabs.append(tab);

    int index = m_tabWidget->addTab(viewer, title);
    m_tabWidget->setTabIcon(index, QIcon(QStringLiteral(":/icons/open.svg")));

    // Update tab title if the viewer emits a refined title (e.g., truncated differently).
    connect(viewer, &PdfViewerWidget::titleChanged, this, [this, tab](const QString& t) {
        const int idx = m_tabWidget->indexOf(tab->widget);
        if (idx >= 0) {
            tab->title = t;
            setTabTitle(idx, t);
        }
    });

    updateTabBar();
    m_tabWidget->setCurrentIndex(index);

    emit tabCountChanged(m_tabs.size());
}

void TabManager::closeTab(int index)
{
    if (index < 0 || index >= m_tabs.size()) {
        return;
    }

    PDFViewerTab* tab = m_tabs.at(index);

    // Prevent closing the last tab if it's the home tab
    if (m_tabs.size() == 1 && tab->isHomeTab) {
        return;
    }

    // Prevent closing home tab if it's the only home tab
    if (tab->isHomeTab && findHomeTabIndex() == index && m_tabs.size() > 1) {
        // Allow closing if there are other tabs
    }

    m_tabWidget->removeTab(index);
    delete tab->widget;
    delete tab;
    m_tabs.removeAt(index);

    updateTabBar();

    // Switch to another tab if current was closed
    if (m_tabWidget->currentIndex() == -1 && !m_tabs.isEmpty()) {
        int newIndex = (index < m_tabs.size()) ? index : m_tabs.size() - 1;
        m_tabWidget->setCurrentIndex(newIndex);
    }

    emit tabClosed(index);
    emit tabCountChanged(m_tabs.size());
}

void TabManager::closeAllTabs()
{
    // Keep at least one home tab
    int homeIndex = findHomeTabIndex();
    while (m_tabs.size() > 1) {
        int indexToClose = (m_tabs.size() - 1);
        if (indexToClose == homeIndex) {
            indexToClose = 0;
        }
        closeTab(indexToClose);
    }
}

void TabManager::switchToTab(int index)
{
    if (index >= 0 && index < m_tabs.size()) {
        m_tabWidget->setCurrentIndex(index);
    }
}

void TabManager::switchToNextTab()
{
    int current = m_tabWidget->currentIndex();
    int next = (current + 1) % m_tabs.size();
    m_tabWidget->setCurrentIndex(next);
}

void TabManager::switchToPreviousTab()
{
    int current = m_tabWidget->currentIndex();
    int prev = (current - 1 + m_tabs.size()) % m_tabs.size();
    m_tabWidget->setCurrentIndex(prev);
}

void TabManager::switchToTabNumber(int n)
{
    if (n >= 1 && n <= 8 && n <= m_tabs.size()) {
        m_tabWidget->setCurrentIndex(n - 1);
    }
}

void TabManager::switchToLastTab()
{
    if (!m_tabs.isEmpty()) {
        m_tabWidget->setCurrentIndex(m_tabs.size() - 1);
    }
}

PDFViewerTab* TabManager::currentTab() const
{
    int index = m_tabWidget->currentIndex();
    if (index >= 0 && index < m_tabs.size()) {
        return m_tabs.at(index);
    }
    return nullptr;
}

PDFViewerTab* TabManager::tabAt(int index) const
{
    if (index >= 0 && index < m_tabs.size()) {
        return m_tabs.at(index);
    }
    return nullptr;
}

int TabManager::tabCount() const
{
    return m_tabs.size();
}

int TabManager::currentIndex() const
{
    return m_tabWidget->currentIndex();
}

void TabManager::setTabDirty(int index, bool dirty)
{
    if (index >= 0 && index < m_tabs.size()) {
        PDFViewerTab* tab = m_tabs.at(index);
        if (tab->isDirty != dirty) {
            tab->isDirty = dirty;
            updateTabBar();
            emit tabDirtyChanged(index, dirty);
        }
    }
}

void TabManager::setTabTitle(int index, const QString& title)
{
    if (index >= 0 && index < m_tabs.size()) {
        PDFViewerTab* tab = m_tabs.at(index);
        tab->title = title;
        QString displayTitle = tab->isDirty ? title + QStringLiteral("*") : title;
        m_tabWidget->setTabText(index, displayTitle);
        emit tabTitleChanged(index, title);
    }
}

void TabManager::setTabPage(int index, int page)
{
    if (index >= 0 && index < m_tabs.size()) {
        m_tabs.at(index)->currentPage = page;
    }
}

void TabManager::setTabZoom(int index, float zoom)
{
    if (index >= 0 && index < m_tabs.size()) {
        m_tabs.at(index)->zoomLevel = zoom;
    }
}

void TabManager::showTabContextMenu(int index, const QPoint& pos)
{
    m_contextMenuTabIndex = index;
    if (m_contextMenu) {
        m_contextMenu->exec(pos);
    }
}

QList<PDFViewerTab*> TabManager::getAllTabs() const
{
    return m_tabs;
}

void TabManager::restoreTab(const QString& filePath, int currentPage, float zoomLevel, bool isHome, bool isActive)
{
    if (isHome) {
        createNewTab(true);
    } else {
        // Guard: skip PDF tabs whose file no longer exists on disk.
        // Without this guard, MuPDF blocks the main thread trying to open a
        // missing file (e.g. from a removed USB drive or network share).
        if (filePath.isEmpty() || !QFileInfo::exists(filePath)) {
            qWarning() << "[TabManager] restoreTab: skipping missing file" << filePath;
            return;
        }
        openPDFInNewTab(filePath);
    }

    int index = m_tabs.size() - 1;
    if (index >= 0 && index < m_tabs.size()) {
        PDFViewerTab* tab = m_tabs.at(index);
        tab->currentPage = currentPage;
        tab->zoomLevel   = zoomLevel;

        if (auto* viewer = tab->pdfViewer()) {
            viewer->setCurrentPage(currentPage - 1);   // 0-based
            viewer->setZoomPercent(zoomLevel);
            if (!tab->scrollPos.isNull())
                viewer->setScrollPos(tab->scrollPos);
            viewer->setSidebarVisible(tab->sidebarVisible);
            if (!tab->splitterSizes.isEmpty())
                viewer->setSplitterSizes(tab->splitterSizes);
        }
    }

    if (isActive) {
        m_tabWidget->setCurrentIndex(index);
    }
}

void TabManager::updateTabBar()
{
    for (int i = 0; i < m_tabs.size(); ++i) {
        PDFViewerTab* tab = m_tabs.at(i);
        QString displayTitle = tab->isDirty ? tab->title + QStringLiteral("*") : tab->title;
        m_tabWidget->setTabText(i, displayTitle);
    }
}

void TabManager::setupTabContextMenu()
{
    m_contextMenu = new QMenu(m_tabWidget);

    QAction* closeTabAct = m_contextMenu->addAction(QStringLiteral("Close Tab"));
    connect(closeTabAct, &QAction::triggered, this, [this]() {
        if (m_contextMenuTabIndex >= 0) {
            closeTab(m_contextMenuTabIndex);
        }
    });

    QAction* closeOtherTabsAct = m_contextMenu->addAction(QStringLiteral("Close Other Tabs"));
    connect(closeOtherTabsAct, &QAction::triggered, this, [this]() {
        if (m_contextMenuTabIndex >= 0) {
            int keepIndex = m_contextMenuTabIndex;
            for (int i = m_tabs.size() - 1; i >= 0; --i) {
                if (i != keepIndex) {
                    closeTab(i);
                }
            }
        }
    });

    QAction* closeTabsToRightAct = m_contextMenu->addAction(QStringLiteral("Close Tabs to the Right"));
    connect(closeTabsToRightAct, &QAction::triggered, this, [this]() {
        if (m_contextMenuTabIndex >= 0) {
            for (int i = m_tabs.size() - 1; i > m_contextMenuTabIndex; --i) {
                closeTab(i);
            }
        }
    });

    m_contextMenu->addSeparator();

    QAction* duplicateTabAct = m_contextMenu->addAction(QStringLiteral("Duplicate Tab"));
    connect(duplicateTabAct, &QAction::triggered, this, [this]() {
        if (m_contextMenuTabIndex >= 0 && m_contextMenuTabIndex < m_tabs.size()) {
            PDFViewerTab* tab = m_tabs.at(m_contextMenuTabIndex);
            if (tab->isHomeTab) {
                createNewTab(true);
            } else {
                openPDFInNewTab(tab->filePath);
            }
        }
    });

    m_contextMenu->addSeparator();

    QAction* reloadTabAct = m_contextMenu->addAction(QStringLiteral("Reload Tab"));
    connect(reloadTabAct, &QAction::triggered, this, [this]() {
        // Placeholder: will be implemented when PDF loading is added
        if (m_contextMenuTabIndex >= 0) {
            // Reload logic here
        }
    });

    QAction* pinTabAct = m_contextMenu->addAction(QStringLiteral("Pin Tab"));
    pinTabAct->setCheckable(true);
    connect(pinTabAct, &QAction::triggered, this, [this](bool checked) {
        // Placeholder: pin tab functionality
        Q_UNUSED(checked);
    });
}

QString TabManager::generateTabTitle(const QString& filePath, bool isHome) const
{
    if (isHome) {
        return QStringLiteral("Home");
    }
    QFileInfo fileInfo(filePath);
    QString title = fileInfo.fileName();
    if (title.length() > 20) {
        title = title.left(17) + QStringLiteral("...");
    }
    return title;
}

int TabManager::findHomeTabIndex() const
{
    for (int i = 0; i < m_tabs.size(); ++i) {
        if (m_tabs.at(i)->isHomeTab) {
            return i;
        }
    }
    return -1;
}

void TabManager::onTabCloseRequested(int index)
{
    closeTab(index);
}

void TabManager::onCurrentTabChanged(int index)
{
    // Save state of the tab that just became inactive.
    // We can't know the "previous" index reliably here, so we save all PDF tabs.
    for (auto* tab : m_tabs) {
        if (auto* viewer = tab->pdfViewer()) {
            tab->currentPage    = viewer->currentPage() + 1;  // 1-based for storage
            tab->scrollPos      = viewer->scrollPos();
            tab->sidebarVisible = viewer->sidebarVisible();
            tab->splitterSizes  = viewer->splitterSizes();
        }
    }

    emit tabChanged(index);
}

void TabManager::onTabContextMenuRequested(const QPoint& pos)
{
    int index = m_tabWidget->tabBar()->tabAt(pos);
    if (index >= 0) {
        showTabContextMenu(index, m_tabWidget->tabBar()->mapToGlobal(pos));
    }
}
