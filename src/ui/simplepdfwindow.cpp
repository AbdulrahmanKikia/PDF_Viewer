#include "simplepdfwindow.h"
#include "pdftabwidget.h"
#include "homepagewidget.h"
#include "settingsdialog.h"
#include "config/recentfilesmanager.h"
#include "config/sessionmanager.h"
#include "config/settingsmanager.h"
#include "config/theme.h"

#ifdef PDFVIEWER_HAS_PRINT
#  include <QPrinter>
#  include <QPrintDialog>
#  include <QPainter>
#endif


#include <QPdfDocument>
#include <QPdfLink>
#include <QPdfView>
#include <QPdfPageNavigator>
#include <QPdfSearchModel>
#include <QPdfBookmarkModel>

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QMimeData>
#include <QMetaObject>
#include <QModelIndex>
#include <QPointF>
#include <QProcess>
#include <QSettings>
#include <QScrollBar>
#include <QSpinBox>
#include <QStatusBar>
#include <QTabBar>
#include <QTabWidget>
#include <QTimer>
#include <QPointer>
#include <QToolBar>
#include <QTreeView>
#include <QUrl>
#include <QVBoxLayout>
#include <QWheelEvent>

SimplePdfWindow::SimplePdfWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setAcceptDrops(true);
    createUi();
    createMenuBar();
    restoreWindowGeometry();

    // Load persisted settings and apply them (theme, thumbnails, etc.)
    // before session restore so new tabs inherit correct defaults.
    SettingsManager::instance()->load();
    applySettingsToApp();

    openHomeTab();
    if (SettingsManager::instance()->current().general.reopenTabsOnStartup)
        restoreSession();
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void SimplePdfWindow::openFilePath(const QString &filePath)
{
    if (!filePath.isEmpty())
        openFileWithTabBehavior(filePath);
}

// ---------------------------------------------------------------------------
// Tab helpers
// ---------------------------------------------------------------------------

PdfTabWidget *SimplePdfWindow::currentTab() const
{
    return qobject_cast<PdfTabWidget *>(m_tabWidget->currentWidget());
}

bool SimplePdfWindow::isHomeTab(int index) const
{
    if (index < 0 || index >= m_tabWidget->count())
        return false;
    return m_tabWidget->widget(index) == m_homeTab;
}

HomePageWidget *SimplePdfWindow::homeTab() const
{
    return m_homeTab;
}

PdfTabWidget *SimplePdfWindow::openInNewTab(const QString &filePath)
{
    auto *tab = new PdfTabWidget(m_tabWidget);
    installTabEventFilter(tab);

    const int idx = m_tabWidget->addTab(tab, tr("New Tab"));

    connect(tab, &PdfTabWidget::documentLoaded, this, [this, tab](const QString &path) {
        const int i = m_tabWidget->indexOf(tab);
        if (i >= 0)
            updateTabTitle(i);
        RecentFilesManager::instance()->addFile(path);
    });

    connect(tab, &PdfTabWidget::pageChanged, this, [this, tab]() {
        if (tab == currentTab()) {
            updatePageControls();
            updateStatusLabel();
        }
    });

    connect(tab, &PdfTabWidget::zoomChanged, this, [this, tab]() {
        if (tab == currentTab())
            updateStatusLabel();
    });

    connect(tab->searchModel(), &QPdfSearchModel::countChanged, this, [this, tab]() {
        if (tab != currentTab())
            return;
        const int count = tab->searchModel()->rowCount(QModelIndex());
        if (count > 0 && !tab->searchModel()->searchString().isEmpty()) {
            tab->view()->setCurrentSearchResultIndex(0);
            const QPdfLink result = tab->searchModel()->resultAtIndex(0);
            if (result.isValid() && tab->navigator())
                tab->navigator()->jump(result.page(), result.location(), 0);
        }
        updateStatusLabel();
    });

    connect(tab->document(), &QPdfDocument::statusChanged, this, [this, tab]() {
        if (tab == currentTab() &&
            tab->document()->status() == QPdfDocument::Status::Ready) {
            updatePageControls();
            updateStatusLabel();
        }
    });

    connect(tab->tocView(), &QTreeView::clicked, this, [this](const QModelIndex &index) {
        onBookmarkActivated(index);
    });

    if (!filePath.isEmpty()) {
        if (!tab->loadFile(filePath)) {
            m_tabWidget->removeTab(idx);
            delete tab;
            return nullptr;
        }
    }

    // Apply default zoom from settings so new tabs respect viewing/defaultZoom.
    const QString defaultZoom = SettingsManager::instance()->current().viewing.defaultZoom;
    if (tab->view()) {
        if (defaultZoom == QLatin1String("fit-width"))
            tab->view()->setZoomMode(QPdfView::ZoomMode::FitToWidth);
        else if (defaultZoom == QLatin1String("fit-page"))
            tab->view()->setZoomMode(QPdfView::ZoomMode::FitInView);
        else {
            bool ok = false;
            const int pct = defaultZoom.toInt(&ok);
            if (ok && pct > 0) {
                tab->view()->setZoomMode(QPdfView::ZoomMode::Custom);
                tab->view()->setZoomFactor(qBound(0.1, pct / 100.0, 10.0));
            }
        }
    }

    m_tabWidget->setCurrentIndex(idx);
    return tab;
}

void SimplePdfWindow::openFileWithTabBehavior(const QString &filePath)
{
    const QString behavior = SettingsManager::instance()->current().general.tabOpenBehavior;
    PdfTabWidget *tab = currentTab();

    if (behavior == QLatin1String("new")) {
        openInNewTab(filePath);
        return;
    }

    if (behavior == QLatin1String("replace")) {
        if (tab && tab->loadFile(filePath)) {
            updateTabTitle(m_tabWidget->currentIndex());
            updatePageControls();
            updateStatusLabel();
            RecentFilesManager::instance()->addFile(filePath);
        } else {
            openInNewTab(filePath);
        }
        return;
    }

    if (behavior == QLatin1String("ask")) {
        QMessageBox msg(this);
        msg.setWindowTitle(tr("Open PDF"));
        msg.setText(tr("How would you like to open \"%1\"?")
                        .arg(QFileInfo(filePath).fileName()));
        QPushButton *newTabBtn = msg.addButton(tr("Open in new tab"), QMessageBox::ActionRole);
        QPushButton *replaceBtn = nullptr;
        if (tab)
            replaceBtn = msg.addButton(tr("Replace current tab"), QMessageBox::ActionRole);
        msg.addButton(tr("Cancel"), QMessageBox::RejectRole);
        msg.exec();

        if (replaceBtn && msg.clickedButton() == replaceBtn) {
            if (tab->loadFile(filePath)) {
                updateTabTitle(m_tabWidget->currentIndex());
                updatePageControls();
                updateStatusLabel();
                RecentFilesManager::instance()->addFile(filePath);
            } else {
                openInNewTab(filePath);
            }
        } else if (msg.clickedButton() == newTabBtn) {
            openInNewTab(filePath);
        }
        return;
    }

    openInNewTab(filePath);
}

HomePageWidget *SimplePdfWindow::openHomeTab()
{
    if (m_homeTab)
        return m_homeTab;

    m_homeTab = new HomePageWidget(m_tabWidget);
    connect(m_homeTab, &HomePageWidget::openFileRequested,
            this, &SimplePdfWindow::openFilePath);

    const int idx = m_tabWidget->insertTab(0, m_homeTab, tr("Home"));
    m_tabWidget->setTabToolTip(idx, tr("Home — recent files and file browser"));

    // Hide the close button on the home tab so it cannot be closed accidentally.
    m_tabWidget->tabBar()->setTabButton(idx, QTabBar::RightSide, nullptr);
    m_tabWidget->tabBar()->setTabButton(idx, QTabBar::LeftSide, nullptr);

    return m_homeTab;
}

void SimplePdfWindow::updateTabTitle(int index)
{
    auto *tab = qobject_cast<PdfTabWidget *>(m_tabWidget->widget(index));
    if (!tab)
        return;
    if (tab->filePath().isEmpty()) {
        m_tabWidget->setTabText(index, tr("New Tab"));
        m_tabWidget->setTabToolTip(index, QString());
    } else {
        const QFileInfo fi(tab->filePath());
        QString name = fi.fileName();
        if (name.length() > 20)
            name = name.left(17) + QStringLiteral("...");
        m_tabWidget->setTabText(index, name);
        m_tabWidget->setTabToolTip(index, fi.absoluteFilePath());
    }
}

void SimplePdfWindow::installTabEventFilter(PdfTabWidget *tab)
{
    tab->view()->viewport()->installEventFilter(this);
}

// ---------------------------------------------------------------------------
// UI construction
// ---------------------------------------------------------------------------

void SimplePdfWindow::createUi()
{
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setMovable(true);
    m_tabWidget->setDocumentMode(true);

    connect(m_tabWidget, &QTabWidget::currentChanged,
            this, &SimplePdfWindow::onTabChanged);
    connect(m_tabWidget, &QTabWidget::tabCloseRequested,
            this, &SimplePdfWindow::onTabCloseRequested);

    m_tabWidget->tabBar()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_tabWidget->tabBar(), &QWidget::customContextMenuRequested,
            this, &SimplePdfWindow::showTabContextMenu);

    setCentralWidget(m_tabWidget);

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

    setWindowTitle(tr("PDFViewer"));
    resize(1200, 800);
}

// ---------------------------------------------------------------------------
// Menu bar (6 menus per design spec)
// ---------------------------------------------------------------------------

void SimplePdfWindow::createMenuBar()
{
    // ── File ──
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));

    QAction *openAct = fileMenu->addAction(tr("&Open File..."));
    openAct->setShortcut(QKeySequence::Open);
    connect(openAct, &QAction::triggered, this, &SimplePdfWindow::openFile);

    m_recentFilesMenu = fileMenu->addMenu(tr("Open &Recent"));
    connect(m_recentFilesMenu, &QMenu::aboutToShow,
            this, &SimplePdfWindow::populateRecentFilesMenu);

    fileMenu->addSeparator();

    QAction *newTabAct = fileMenu->addAction(tr("New &Tab"));
    newTabAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_T));
    connect(newTabAct, &QAction::triggered, this, &SimplePdfWindow::newTab);

    QAction *closeTabAct = fileMenu->addAction(tr("&Close Tab"));
    closeTabAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_W));
    connect(closeTabAct, &QAction::triggered, this, &SimplePdfWindow::closeCurrentTab);

    fileMenu->addSeparator();

    m_printAct = fileMenu->addAction(tr("&Print..."));
    m_printAct->setShortcut(QKeySequence::Print);
#ifdef PDFVIEWER_HAS_PRINT
    m_printAct->setEnabled(false);   // enabled dynamically when a document is open
    connect(m_printAct, &QAction::triggered, this, &SimplePdfWindow::printDocument);
#else
    m_printAct->setEnabled(false);
    m_printAct->setToolTip(tr("Print support not available in this build"));
#endif

    fileMenu->addSeparator();

    QAction *propsAct = fileMenu->addAction(tr("P&roperties"));
    propsAct->setEnabled(false);

    fileMenu->addSeparator();

    QAction *exitAct = fileMenu->addAction(tr("E&xit"));
    exitAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q));
    connect(exitAct, &QAction::triggered, this, &QWidget::close);

    // ── Edit ──
    QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));

    QAction *undoAct = editMenu->addAction(tr("&Undo"));
    undoAct->setShortcut(QKeySequence::Undo);
    undoAct->setEnabled(false);

    QAction *redoAct = editMenu->addAction(tr("&Redo"));
    redoAct->setShortcut(QKeySequence::Redo);
    redoAct->setEnabled(false);

    editMenu->addSeparator();

    QAction *copyAct = editMenu->addAction(tr("&Copy"));
    copyAct->setShortcut(QKeySequence::Copy);
    copyAct->setEnabled(false);

    QAction *selectAllAct = editMenu->addAction(tr("Select &All"));
    selectAllAct->setShortcut(QKeySequence::SelectAll);
    selectAllAct->setEnabled(false);

    editMenu->addSeparator();

    QAction *findAct = editMenu->addAction(tr("&Find..."));
    findAct->setShortcut(QKeySequence::Find);
    connect(findAct, &QAction::triggered, this, &SimplePdfWindow::focusSearchBar);

    QAction *findReplaceAct = editMenu->addAction(tr("Find && &Replace"));
    findReplaceAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_H));
    findReplaceAct->setEnabled(false);

    editMenu->addSeparator();

    QAction *prefsAct = editMenu->addAction(tr("&Preferences"));
    prefsAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Comma));
    connect(prefsAct, &QAction::triggered, this, &SimplePdfWindow::showSettings);

    // ── View ──
    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));

    QAction *zoomInMenuAct = viewMenu->addAction(tr("Zoom &In"));
    zoomInMenuAct->setShortcut(QKeySequence::ZoomIn);
    connect(zoomInMenuAct, &QAction::triggered, this, &SimplePdfWindow::zoomIn);

    QAction *zoomOutMenuAct = viewMenu->addAction(tr("Zoom &Out"));
    zoomOutMenuAct->setShortcut(QKeySequence::ZoomOut);
    connect(zoomOutMenuAct, &QAction::triggered, this, &SimplePdfWindow::zoomOut);

    QAction *resetZoomMenuAct = viewMenu->addAction(tr("&Reset Zoom"));
    resetZoomMenuAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_0));
    connect(resetZoomMenuAct, &QAction::triggered, this, &SimplePdfWindow::resetZoom);

    QAction *fitWidthMenuAct = viewMenu->addAction(tr("Fit &Width"));
    connect(fitWidthMenuAct, &QAction::triggered, this, &SimplePdfWindow::fitWidth);

    QAction *fitPageMenuAct = viewMenu->addAction(tr("Fit &Page"));
    connect(fitPageMenuAct, &QAction::triggered, this, &SimplePdfWindow::fitPage);

    viewMenu->addSeparator();

    m_navPaneAction = viewMenu->addAction(tr("&Navigation Pane"));
    m_navPaneAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N));
    m_navPaneAction->setCheckable(true);
    m_navPaneAction->setChecked(true);
    connect(m_navPaneAction, &QAction::toggled, this, [this](bool checked) {
        PdfTabWidget *tab = currentTab();
        if (tab)
            tab->setTocVisible(checked);
    });

    viewMenu->addSeparator();

    QAction *fullScreenAct = viewMenu->addAction(tr("&Full Screen"));
    fullScreenAct->setShortcut(QKeySequence(Qt::Key_F11));
    connect(fullScreenAct, &QAction::triggered, this, &SimplePdfWindow::toggleFullScreen);

    viewMenu->addSeparator();

    auto *themeGroup = new QActionGroup(this);
    themeGroup->setExclusive(true);

    m_darkModeAction = viewMenu->addAction(tr("&Dark Mode"));
    m_darkModeAction->setCheckable(true);
    m_darkModeAction->setChecked(true);
    themeGroup->addAction(m_darkModeAction);
    connect(m_darkModeAction, &QAction::triggered, this, &SimplePdfWindow::toggleDarkMode);

    m_lightModeAction = viewMenu->addAction(tr("&Light Mode"));
    m_lightModeAction->setCheckable(true);
    themeGroup->addAction(m_lightModeAction);
    connect(m_lightModeAction, &QAction::triggered, this, &SimplePdfWindow::toggleLightMode);

    viewMenu->addSeparator();

    QAction *reloadAct = viewMenu->addAction(tr("Re&load"));
    reloadAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
    connect(reloadAct, &QAction::triggered, this, &SimplePdfWindow::reloadDocument);

    // ── Tools ──
    QMenu *toolsMenu = menuBar()->addMenu(tr("&Tools"));

    QAction *ocrAct = toolsMenu->addAction(tr("Text &Recognition"));
    ocrAct->setEnabled(false);
    QAction *extractAct = toolsMenu->addAction(tr("&Extract Text"));
    extractAct->setEnabled(false);

    toolsMenu->addSeparator();

    QMenu *fileMgmtMenu = toolsMenu->addMenu(tr("&File Management"));
    QAction *renameAct = fileMgmtMenu->addAction(tr("&Rename"));
    renameAct->setEnabled(false);
    QAction *duplicateAct = fileMgmtMenu->addAction(tr("&Duplicate"));
    duplicateAct->setEnabled(false);
    QAction *trashAct = fileMgmtMenu->addAction(tr("&Move to Trash"));
    trashAct->setEnabled(false);

    QAction *showFolderAct = fileMgmtMenu->addAction(tr("&Show in Folder"));
    connect(showFolderAct, &QAction::triggered, this, &SimplePdfWindow::showInFolder);

    // ── Help ──
    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));

    QAction *shortcutsAct = helpMenu->addAction(tr("&Keyboard Shortcuts"));
    shortcutsAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Slash));
    connect(shortcutsAct, &QAction::triggered, this, &SimplePdfWindow::showKeyboardShortcuts);

    helpMenu->addSeparator();

    QAction *aboutAct = helpMenu->addAction(tr("&About PDFViewer"));
    connect(aboutAct, &QAction::triggered, this, &SimplePdfWindow::showAbout);

    // ── Non-menu shortcuts ──
    QAction *pgUpAct = new QAction(this);
    pgUpAct->setShortcut(QKeySequence(Qt::Key_PageUp));
    connect(pgUpAct, &QAction::triggered, this, &SimplePdfWindow::goPreviousPage);
    addAction(pgUpAct);

    QAction *pgDnAct = new QAction(this);
    pgDnAct->setShortcut(QKeySequence(Qt::Key_PageDown));
    connect(pgDnAct, &QAction::triggered, this, &SimplePdfWindow::goNextPage);
    addAction(pgDnAct);

    QAction *homeAct = new QAction(this);
    homeAct->setShortcut(QKeySequence(Qt::Key_Home));
    connect(homeAct, &QAction::triggered, this, &SimplePdfWindow::goFirstPage);
    addAction(homeAct);

    QAction *endAct = new QAction(this);
    endAct->setShortcut(QKeySequence(Qt::Key_End));
    connect(endAct, &QAction::triggered, this, &SimplePdfWindow::goLastPage);
    addAction(endAct);

    QAction *escAct = new QAction(this);
    escAct->setShortcut(QKeySequence(Qt::Key_Escape));
    connect(escAct, &QAction::triggered, this, &SimplePdfWindow::clearSearch);
    addAction(escAct);

    // Tab switching shortcuts
    QAction *nextTabAct = new QAction(this);
    nextTabAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Tab));
    connect(nextTabAct, &QAction::triggered, this, &SimplePdfWindow::nextTab);
    addAction(nextTabAct);

    QAction *prevTabAct = new QAction(this);
    prevTabAct->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Tab));
    connect(prevTabAct, &QAction::triggered, this, &SimplePdfWindow::previousTab);
    addAction(prevTabAct);

    // Ctrl+1..8 jump to tab N, Ctrl+9 jump to last tab
    for (int i = 1; i <= 9; ++i) {
        auto *jumpAct = new QAction(this);
        jumpAct->setShortcut(QKeySequence(Qt::CTRL | static_cast<Qt::Key>(Qt::Key_0 + i)));
        connect(jumpAct, &QAction::triggered, this, [this, i]() {
            if (i == 9) {
                m_tabWidget->setCurrentIndex(m_tabWidget->count() - 1);
            } else {
                const int target = i - 1;
                if (target < m_tabWidget->count())
                    m_tabWidget->setCurrentIndex(target);
            }
        });
        addAction(jumpAct);
    }
}

// ---------------------------------------------------------------------------
// Tab management slots
// ---------------------------------------------------------------------------

void SimplePdfWindow::newTab()
{
    openInNewTab();
}

void SimplePdfWindow::closeCurrentTab()
{
    closeTab(m_tabWidget->currentIndex());
}

void SimplePdfWindow::closeTab(int index)
{
    if (index < 0 || index >= m_tabWidget->count())
        return;

    // The home tab is permanent — it cannot be closed.
    if (isHomeTab(index))
        return;

    QWidget *w = m_tabWidget->widget(index);
    m_tabWidget->removeTab(index);
    delete w;
}

void SimplePdfWindow::nextTab()
{
    const int count = m_tabWidget->count();
    if (count <= 1)
        return;
    m_tabWidget->setCurrentIndex((m_tabWidget->currentIndex() + 1) % count);
}

void SimplePdfWindow::previousTab()
{
    const int count = m_tabWidget->count();
    if (count <= 1)
        return;
    m_tabWidget->setCurrentIndex((m_tabWidget->currentIndex() - 1 + count) % count);
}

void SimplePdfWindow::onTabChanged(int index)
{
    Q_UNUSED(index);
    syncUiToCurrentTab();
}

void SimplePdfWindow::onTabCloseRequested(int index)
{
    closeTab(index);
}

void SimplePdfWindow::syncUiToCurrentTab()
{
    PdfTabWidget *tab = currentTab();
    if (!tab) {
        // Home tab or empty state — reset toolbar controls.
        setWindowTitle(tr("PDFViewer"));
        m_pageCountLabel->setText(QStringLiteral("/ 0"));
        m_pageSpin->setMaximum(1);
        m_pageSpin->setValue(1);
        m_statusLabel->setText(tr("Ready"));
        if (m_printAct)
            m_printAct->setEnabled(false);
        return;
    }

    updateWindowTitle();
    updatePageControls();
    updateStatusLabel();

    if (m_navPaneAction)
        m_navPaneAction->setChecked(tab->isTocVisible());

    const QString tabSearch = tab->searchModel()->searchString();
    if (m_searchEdit->text() != tabSearch) {
        QSignalBlocker blocker(m_searchEdit);
        m_searchEdit->setText(tabSearch);
    }

#ifdef PDFVIEWER_HAS_PRINT
    if (m_printAct)
        m_printAct->setEnabled(tab->hasDocument());
#endif
}

// ---------------------------------------------------------------------------
// Tab context menu
// ---------------------------------------------------------------------------

void SimplePdfWindow::showTabContextMenu(const QPoint &pos)
{
    const int tabIndex = m_tabWidget->tabBar()->tabAt(pos);
    if (tabIndex < 0)
        return;

    QMenu menu(this);

    const bool thisIsHome = isHomeTab(tabIndex);

    QAction *closeAct = menu.addAction(tr("Close Tab"));
    closeAct->setEnabled(!thisIsHome);
    connect(closeAct, &QAction::triggered, this, [this, tabIndex]() {
        closeTab(tabIndex);
    });

    QAction *closeOthersAct = menu.addAction(tr("Close Other Tabs"));
    closeOthersAct->setEnabled(m_tabWidget->count() > 1);
    connect(closeOthersAct, &QAction::triggered, this, [this, tabIndex]() {
        for (int i = m_tabWidget->count() - 1; i >= 0; --i) {
            if (i != tabIndex && !isHomeTab(i)) {
                QWidget *w = m_tabWidget->widget(i);
                m_tabWidget->removeTab(i);
                delete w;
            }
        }
    });

    QAction *closeRightAct = menu.addAction(tr("Close Tabs to the Right"));
    closeRightAct->setEnabled(tabIndex < m_tabWidget->count() - 1);
    connect(closeRightAct, &QAction::triggered, this, [this, tabIndex]() {
        for (int i = m_tabWidget->count() - 1; i > tabIndex; --i) {
            if (!isHomeTab(i)) {
                QWidget *w = m_tabWidget->widget(i);
                m_tabWidget->removeTab(i);
                delete w;
            }
        }
    });

    menu.addSeparator();

    QAction *dupAct = menu.addAction(tr("Duplicate Tab"));
    auto *srcTab = qobject_cast<PdfTabWidget *>(m_tabWidget->widget(tabIndex));
    dupAct->setEnabled(srcTab && srcTab->hasDocument());
    connect(dupAct, &QAction::triggered, this, [this, srcTab]() {
        if (srcTab && !srcTab->filePath().isEmpty())
            openInNewTab(srcTab->filePath());
    });

    QAction *reloadAct = menu.addAction(tr("Reload Tab"));
    reloadAct->setEnabled(srcTab && srcTab->hasDocument());
    connect(reloadAct, &QAction::triggered, this, [srcTab]() {
        if (srcTab)
            srcTab->reloadFile();
    });

    menu.exec(m_tabWidget->tabBar()->mapToGlobal(pos));
}

// ---------------------------------------------------------------------------
// File opening
// ---------------------------------------------------------------------------

void SimplePdfWindow::openFile()
{
    const QString filePath = QFileDialog::getOpenFileName(
        this, tr("Open PDF File"), QString(), tr("PDF Files (*.pdf)"));
    if (filePath.isEmpty())
        return;
    openFileWithTabBehavior(filePath);
}

// ---------------------------------------------------------------------------
// Page navigation (delegates to current tab)
// ---------------------------------------------------------------------------

void SimplePdfWindow::goNextPage()
{
    PdfTabWidget *tab = currentTab();
    if (!tab || !tab->hasDocument() || !tab->navigator())
        return;
    const int page = tab->navigator()->currentPage();
    if (page + 1 < tab->document()->pageCount()) {
        tab->navigator()->jump(page + 1, QPointF(), 0);
        updatePageControls();
        updateStatusLabel();
    }
}

void SimplePdfWindow::goPreviousPage()
{
    PdfTabWidget *tab = currentTab();
    if (!tab || !tab->hasDocument() || !tab->navigator())
        return;
    const int page = tab->navigator()->currentPage();
    if (page > 0) {
        tab->navigator()->jump(page - 1, QPointF(), 0);
        updatePageControls();
        updateStatusLabel();
    }
}

void SimplePdfWindow::goFirstPage()
{
    PdfTabWidget *tab = currentTab();
    if (!tab || !tab->hasDocument() || !tab->navigator())
        return;
    tab->navigator()->jump(0, QPointF(), 0);
    updatePageControls();
    updateStatusLabel();
}

void SimplePdfWindow::goLastPage()
{
    PdfTabWidget *tab = currentTab();
    if (!tab || !tab->hasDocument() || !tab->navigator())
        return;
    tab->navigator()->jump(tab->document()->pageCount() - 1, QPointF(), 0);
    updatePageControls();
    updateStatusLabel();
}

void SimplePdfWindow::gotoPageFromSpin()
{
    PdfTabWidget *tab = currentTab();
    if (!tab || !tab->hasDocument() || !tab->navigator())
        return;
    const int target = m_pageSpin->value() - 1;
    if (target >= 0 && target < tab->document()->pageCount()) {
        tab->navigator()->jump(target, QPointF(), 0);
        updatePageControls();
        updateStatusLabel();
    }
}

// ---------------------------------------------------------------------------
// Zoom (delegates to current tab)
// ---------------------------------------------------------------------------

void SimplePdfWindow::zoomIn()
{
    PdfTabWidget *tab = currentTab();
    if (!tab) return;
    const double newZoom = qBound(0.1, tab->view()->zoomFactor() * 1.25, 10.0);
    zoomAnchoredToViewportCenter(tab, newZoom);
}

void SimplePdfWindow::zoomOut()
{
    PdfTabWidget *tab = currentTab();
    if (!tab) return;
    const double newZoom = qBound(0.1, tab->view()->zoomFactor() / 1.25, 10.0);
    zoomAnchoredToViewportCenter(tab, newZoom);
}

void SimplePdfWindow::fitWidth()
{
    PdfTabWidget *tab = currentTab();
    if (!tab) return;
    const int savedPage = tab->navigator()->currentPage();
    tab->view()->setZoomMode(QPdfView::ZoomMode::FitToWidth);
    scheduleZoomAnchorJump(tab, savedPage);
    updateStatusLabel();
}

void SimplePdfWindow::fitPage()
{
    PdfTabWidget *tab = currentTab();
    if (!tab) return;
    const int savedPage = tab->navigator()->currentPage();
    tab->view()->setZoomMode(QPdfView::ZoomMode::FitInView);
    scheduleZoomAnchorJump(tab, savedPage);
    updateStatusLabel();
}

void SimplePdfWindow::resetZoom()
{
    PdfTabWidget *tab = currentTab();
    if (!tab) return;
    const int savedPage = tab->navigator()->currentPage();
    tab->view()->setZoomMode(QPdfView::ZoomMode::Custom);
    tab->view()->setZoomFactor(1.0);
    scheduleZoomAnchorJump(tab, savedPage);
    updateStatusLabel();
}

void SimplePdfWindow::scheduleZoomAnchorJump(PdfTabWidget *tab, int page)
{
    if (!tab || !tab->navigator())
        return;
    QPointer<PdfTabWidget> p(tab);
    QTimer::singleShot(0, this, [p, page]() {
        if (p && p->navigator())
            p->navigator()->jump(page, QPointF(), 0);
    });
}

void SimplePdfWindow::zoomAnchoredToViewportCenter(PdfTabWidget *tab, double newZoom)
{
    if (!tab || !tab->view())
        return;
    QPdfView *v = tab->view();
    const double oldZoom = v->zoomFactor();
    if (oldZoom <= 0)
        return;

    // Viewport center in content coordinates (SinglePage mode: one page, so this scales with zoom)
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
    *connection = connect(v, &QPdfView::zoomFactorChanged, this, [connection, pView, targetScrollX, targetScrollY]() {
        disconnect(*connection);
        delete connection;
        if (!pView)
            return;
        QTimer::singleShot(0, pView, [pView, targetScrollX, targetScrollY]() {
            if (!pView)
                return;
            QScrollBar *hBar = pView->horizontalScrollBar();
            QScrollBar *vBar = pView->verticalScrollBar();
            hBar->setValue(qBound(0, targetScrollX, hBar->maximum()));
            vBar->setValue(qBound(0, targetScrollY, vBar->maximum()));
        });
    });
}

// ---------------------------------------------------------------------------
// Search (delegates to current tab)
// ---------------------------------------------------------------------------

void SimplePdfWindow::onSearchTextChanged(const QString &text)
{
    PdfTabWidget *tab = currentTab();
    if (!tab) return;
    tab->searchModel()->setSearchString(text);
    if (text.isEmpty())
        updateStatusLabel();
}

void SimplePdfWindow::findNext()
{
    PdfTabWidget *tab = currentTab();
    if (!tab) return;
    const int count = tab->searchModel()->rowCount(QModelIndex());
    if (count <= 0)
        return;
    int idx = tab->view()->currentSearchResultIndex();
    idx = (idx + 1) % count;
    tab->view()->setCurrentSearchResultIndex(idx);

    const QPdfLink result = tab->searchModel()->resultAtIndex(idx);
    if (result.isValid() && tab->navigator())
        tab->navigator()->jump(result.page(), result.location(), 0);

    updateStatusLabel();
}

void SimplePdfWindow::findPrevious()
{
    PdfTabWidget *tab = currentTab();
    if (!tab) return;
    const int count = tab->searchModel()->rowCount(QModelIndex());
    if (count <= 0)
        return;
    int idx = tab->view()->currentSearchResultIndex();
    idx = (idx - 1 + count) % count;
    tab->view()->setCurrentSearchResultIndex(idx);

    const QPdfLink result = tab->searchModel()->resultAtIndex(idx);
    if (result.isValid() && tab->navigator())
        tab->navigator()->jump(result.page(), result.location(), 0);

    updateStatusLabel();
}

void SimplePdfWindow::focusSearchBar()
{
    m_searchEdit->setFocus();
    m_searchEdit->selectAll();
}

void SimplePdfWindow::clearSearch()
{
    if (m_isFullScreen) {
        toggleFullScreen();
        return;
    }
    if (!m_searchEdit->text().isEmpty()) {
        m_searchEdit->clear();
        m_searchEdit->clearFocus();
        PdfTabWidget *tab = currentTab();
        if (tab)
            tab->view()->setFocus();
    }
}

// ---------------------------------------------------------------------------
// Bookmarks / TOC
// ---------------------------------------------------------------------------

void SimplePdfWindow::onBookmarkActivated(const QModelIndex &index)
{
    PdfTabWidget *tab = currentTab();
    if (!index.isValid() || !tab || !tab->navigator())
        return;
    const QVariant pageVar = tab->bookmarkModel()->data(
        index, static_cast<int>(QPdfBookmarkModel::Role::Page));
    const int page = pageVar.toInt();
    if (page >= 0 && page < tab->document()->pageCount()) {
        tab->navigator()->jump(page, QPointF(), 0);
        updatePageControls();
        updateStatusLabel();
    }
}

// ---------------------------------------------------------------------------
// View actions
// ---------------------------------------------------------------------------

void SimplePdfWindow::toggleFullScreen()
{
    if (m_isFullScreen) {
        showNormal();
        m_isFullScreen = false;
    } else {
        showFullScreen();
        m_isFullScreen = true;
    }
}

void SimplePdfWindow::toggleDarkMode()
{
    ThemeManager::instance()->applyTheme(ThemeManager::Dark);
    m_darkModeAction->setChecked(true);
}

void SimplePdfWindow::toggleLightMode()
{
    ThemeManager::instance()->applyTheme(ThemeManager::Light);
    m_lightModeAction->setChecked(true);
}

void SimplePdfWindow::reloadDocument()
{
    PdfTabWidget *tab = currentTab();
    if (!tab) return;
    tab->reloadFile();
    updatePageControls();
    updateStatusLabel();
}

void SimplePdfWindow::showInFolder()
{
    PdfTabWidget *tab = currentTab();
    if (!tab || tab->filePath().isEmpty())
        return;
#ifdef Q_OS_WIN
    QProcess::startDetached(QStringLiteral("explorer.exe"),
                            {QStringLiteral("/select,"),
                             QDir::toNativeSeparators(tab->filePath())});
#else
    QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(tab->filePath()).absolutePath()));
#endif
}

void SimplePdfWindow::showAbout()
{
    QMessageBox::about(this, tr("About PDFViewer"),
        tr("<h3>PDFViewer %1</h3>"
           "<p>A modern PDF viewer built with Qt %2.</p>"
           "<p>Built for Windows, macOS, and Linux.</p>")
            .arg(QApplication::applicationVersion(), QString::fromLatin1(qVersion())));
}

void SimplePdfWindow::showKeyboardShortcuts()
{
    static const char *shortcuts = R"(
<table cellpadding="4">
<tr><td><b>Ctrl+O</b></td><td>Open file</td></tr>
<tr><td><b>Ctrl+T</b></td><td>New tab</td></tr>
<tr><td><b>Ctrl+W</b></td><td>Close tab</td></tr>
<tr><td><b>Ctrl+Tab</b></td><td>Next tab</td></tr>
<tr><td><b>Ctrl+Shift+Tab</b></td><td>Previous tab</td></tr>
<tr><td><b>Ctrl+1..8</b></td><td>Jump to tab</td></tr>
<tr><td><b>Ctrl+9</b></td><td>Last tab</td></tr>
<tr><td><b>Ctrl+F</b></td><td>Find / search text</td></tr>
<tr><td><b>Ctrl++</b></td><td>Zoom in</td></tr>
<tr><td><b>Ctrl+-</b></td><td>Zoom out</td></tr>
<tr><td><b>Ctrl+0</b></td><td>Reset zoom (100%)</td></tr>
<tr><td><b>PgUp / PgDn</b></td><td>Previous / next page</td></tr>
<tr><td><b>Home / End</b></td><td>First / last page</td></tr>
<tr><td><b>F11</b></td><td>Toggle full screen</td></tr>
<tr><td><b>Ctrl+R</b></td><td>Reload document</td></tr>
<tr><td><b>Ctrl+Q</b></td><td>Exit</td></tr>
<tr><td><b>Escape</b></td><td>Clear search / exit full screen</td></tr>
</table>
)";
    QMessageBox box(this);
    box.setWindowTitle(tr("Keyboard Shortcuts"));
    box.setTextFormat(Qt::RichText);
    box.setText(QString::fromLatin1(shortcuts));
    box.exec();
}

// ---------------------------------------------------------------------------
// Recent files submenu
// ---------------------------------------------------------------------------

void SimplePdfWindow::populateRecentFilesMenu()
{
    m_recentFilesMenu->clear();

    const QList<RecentFileEntry> entries =
        RecentFilesManager::instance()->getRecentFiles(10);

    if (entries.isEmpty()) {
        QAction *emptyAct = m_recentFilesMenu->addAction(tr("(No recent files)"));
        emptyAct->setEnabled(false);
        return;
    }

    for (const RecentFileEntry &entry : entries) {
        QAction *act = m_recentFilesMenu->addAction(entry.displayName);
        act->setToolTip(entry.filePath);
        connect(act, &QAction::triggered, this, [this, path = entry.filePath]() {
            openFileWithTabBehavior(path);
        });
    }

    m_recentFilesMenu->addSeparator();
    QAction *clearAct = m_recentFilesMenu->addAction(tr("Clear Recent Files"));
    connect(clearAct, &QAction::triggered, this, []() {
        RecentFilesManager::instance()->clearRecentFiles();
    });
}

// ---------------------------------------------------------------------------
// State helpers
// ---------------------------------------------------------------------------

void SimplePdfWindow::updateWindowTitle()
{
    PdfTabWidget *tab = currentTab();
    if (!tab || tab->filePath().isEmpty()) {
        setWindowTitle(tr("PDFViewer"));
        return;
    }
    const QFileInfo fi(tab->filePath());
    setWindowTitle(tr("%1 - PDFViewer").arg(fi.fileName()));
}

void SimplePdfWindow::updatePageControls()
{
    PdfTabWidget *tab = currentTab();
    const int pageCount = (tab && tab->hasDocument()) ? tab->document()->pageCount() : 0;
    m_pageSpin->setMaximum(qMax(1, pageCount));
    const int current = (tab && tab->navigator()) ? tab->navigator()->currentPage() : 0;
    m_pageSpin->setValue(pageCount > 0 ? current + 1 : 1);
    m_pageCountLabel->setText(QStringLiteral("/ %1").arg(pageCount));
}

void SimplePdfWindow::updateStatusLabel()
{
    PdfTabWidget *tab = currentTab();
    if (!tab || !tab->hasDocument()) {
        m_statusLabel->setText(tr("No document"));
        return;
    }

    const int current = tab->navigator() ? (tab->navigator()->currentPage() + 1) : 1;
    const int total   = tab->document()->pageCount();
    const double zoom = tab->view()->zoomFactor() * 100.0;

    QString text = tr("Page %1 of %2  \u2022  Zoom %3%")
                       .arg(current)
                       .arg(total)
                       .arg(QString::number(static_cast<int>(zoom)));

    const QString searchStr = tab->searchModel()->searchString();
    if (!searchStr.isEmpty()) {
        const int matchCount = tab->searchModel()->rowCount(QModelIndex());
        if (matchCount > 0) {
            const int matchIdx = tab->view()->currentSearchResultIndex() + 1;
            text += tr("  \u2022  Match %1 of %2").arg(matchIdx).arg(matchCount);
        } else {
            text += tr("  \u2022  No matches");
        }
    }

    m_statusLabel->setText(text);
}

// ---------------------------------------------------------------------------
// Window geometry persistence
// ---------------------------------------------------------------------------

void SimplePdfWindow::saveWindowGeometry()
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("window"));
    settings.setValue(QStringLiteral("geometry"), saveGeometry());
    settings.setValue(QStringLiteral("state"), saveState());
    settings.endGroup();
}

void SimplePdfWindow::restoreWindowGeometry()
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("window"));
    const QByteArray geom = settings.value(QStringLiteral("geometry")).toByteArray();
    const QByteArray state = settings.value(QStringLiteral("state")).toByteArray();
    settings.endGroup();

    if (!geom.isEmpty())
        restoreGeometry(geom);
    if (!state.isEmpty())
        restoreState(state);
}

// ---------------------------------------------------------------------------
// Session persistence
// ---------------------------------------------------------------------------

void SimplePdfWindow::saveSession()
{
    QList<TabSessionData> tabs;
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        if (isHomeTab(i)) {
            TabSessionData data;
            data.type = QStringLiteral("home");
            data.isActive = (i == m_tabWidget->currentIndex());
            tabs.append(data);
            continue;
        }
        auto *tab = qobject_cast<PdfTabWidget *>(m_tabWidget->widget(i));
        if (!tab)
            continue;
        TabSessionData data;
        data.type = QStringLiteral("pdf");
        data.filePath = tab->filePath();
        data.currentPage = tab->currentPage();
        data.zoomLevel = static_cast<float>(tab->zoomFactor() * 100.0);
        data.isActive = (i == m_tabWidget->currentIndex());
        tabs.append(data);
    }
    SessionManager::instance()->saveSession(tabs, m_tabWidget->currentIndex());
}

void SimplePdfWindow::restoreSession()
{
    QList<TabSessionData> tabs;
    int activeIndex = 0;
    if (!SessionManager::instance()->loadSession(tabs, activeIndex))
        tabs.clear();

    bool anyRestored = false;
    for (const TabSessionData &data : std::as_const(tabs)) {
        // Home tab entries are satisfied by the permanent home tab already open.
        if (data.type == QLatin1String("home"))
            continue;

        if (data.filePath.isEmpty())
            continue;
        if (!QFileInfo::exists(data.filePath))
            continue;

        PdfTabWidget *tab = openInNewTab(data.filePath);
        if (!tab)
            continue;

        anyRestored = true;

        if (data.currentPage > 0 && tab->navigator() &&
            data.currentPage <= tab->document()->pageCount()) {
            tab->navigator()->jump(data.currentPage - 1, QPointF(), 0);
        }

        const double factor = static_cast<double>(data.zoomLevel) / 100.0;
        if (factor > 0.0 && factor != 1.0) {
            const int pageToAnchor = tab->navigator() ? tab->navigator()->currentPage() : 0;
            tab->view()->setZoomMode(QPdfView::ZoomMode::Custom);
            tab->view()->setZoomFactor(factor);
            scheduleZoomAnchorJump(tab, pageToAnchor);
        }
    }

    if (!anyRestored) {
        // No PDF tabs restored — show the home tab (already open at index 0).
        m_tabWidget->setCurrentIndex(0);
    } else if (activeIndex >= 0 && activeIndex < m_tabWidget->count()) {
        m_tabWidget->setCurrentIndex(activeIndex);
    }
}

// ---------------------------------------------------------------------------
// Events
// ---------------------------------------------------------------------------

void SimplePdfWindow::closeEvent(QCloseEvent *event)
{
    saveSession();
    saveWindowGeometry();
    QMainWindow::closeEvent(event);
}

bool SimplePdfWindow::eventFilter(QObject *obj, QEvent *event)
{
    PdfTabWidget *tab = currentTab();
    if (tab && obj == tab->view()->viewport() && event->type() == QEvent::Wheel) {
        auto *we = static_cast<QWheelEvent *>(event);
        if (we->modifiers() & Qt::ControlModifier) {
            const int delta = we->angleDelta().y();
            if (delta == 0)
                return QMainWindow::eventFilter(obj, event);

            const double factor = (delta > 0) ? 1.1 : (1.0 / 1.1);
            const double newZoom = qBound(0.1, tab->view()->zoomFactor() * factor, 10.0);
            zoomAnchoredToViewportCenter(tab, newZoom);
            return true;
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void SimplePdfWindow::dragEnterEvent(QDragEnterEvent *event)
{
    const QMimeData *mime = event->mimeData();
    if (!mime->hasUrls())
        return;
    for (const QUrl &url : mime->urls()) {
        if (url.isLocalFile() && url.toLocalFile().endsWith(QLatin1String(".pdf"), Qt::CaseInsensitive)) {
            event->acceptProposedAction();
            return;
        }
    }
}

void SimplePdfWindow::dropEvent(QDropEvent *event)
{
    const QMimeData *mime = event->mimeData();
    if (!mime->hasUrls())
        return;
    for (const QUrl &url : mime->urls()) {
        const QString path = url.toLocalFile();
        if (path.endsWith(QLatin1String(".pdf"), Qt::CaseInsensitive)) {
            openFileWithTabBehavior(path);
        }
    }
    event->acceptProposedAction();
}

// ---------------------------------------------------------------------------
// Settings
// ---------------------------------------------------------------------------

void SimplePdfWindow::showSettings()
{
    SettingsDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted)
        applySettingsToApp();
}

void SimplePdfWindow::applySettingsToApp()
{
    const AppSettings s = SettingsManager::instance()->current();

    // Apply theme.
    const QString &themeName = s.appearance.theme;
    ThemeManager::Theme theme = ThemeManager::Dark;
    if (themeName == QLatin1String("light"))
        theme = ThemeManager::Light;
    else if (themeName == QLatin1String("system"))
        theme = ThemeManager::System;
    ThemeManager::instance()->applyTheme(theme);

    // Keep the View menu theme toggles in sync.
    if (m_darkModeAction && m_lightModeAction) {
        m_darkModeAction->setChecked(theme == ThemeManager::Dark);
        m_lightModeAction->setChecked(theme == ThemeManager::Light);
    }

    // Apply TOC (thumbnail sidebar) visibility and default zoom to all open PDF tabs.
    const QString &defaultZoom = s.viewing.defaultZoom;
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        auto *tab = qobject_cast<PdfTabWidget *>(m_tabWidget->widget(i));
        if (tab) {
            tab->setTocVisible(s.viewing.showThumbnails);
            if (tab->view()) {
                if (defaultZoom == QLatin1String("fit-width"))
                    tab->view()->setZoomMode(QPdfView::ZoomMode::FitToWidth);
                else if (defaultZoom == QLatin1String("fit-page"))
                    tab->view()->setZoomMode(QPdfView::ZoomMode::FitInView);
                else {
                    bool ok = false;
                    const int pct = defaultZoom.toInt(&ok);
                    if (ok && pct > 0) {
                        tab->view()->setZoomMode(QPdfView::ZoomMode::Custom);
                        tab->view()->setZoomFactor(qBound(0.1, pct / 100.0, 10.0));
                    }
                }
            }
        }
    }

    // Sync the Navigation Pane action state with the new setting.
    if (m_navPaneAction)
        m_navPaneAction->setChecked(s.viewing.showThumbnails);
}

// ---------------------------------------------------------------------------
// Print
// ---------------------------------------------------------------------------

void SimplePdfWindow::printDocument()
{
#ifdef PDFVIEWER_HAS_PRINT
    PdfTabWidget *tab = currentTab();
    if (!tab || !tab->hasDocument())
        return;

    QPrinter printer(QPrinter::HighResolution);
    QPrintDialog dlg(&printer, this);
    dlg.setWindowTitle(tr("Print Document"));
    if (dlg.exec() != QDialog::Accepted)
        return;

    QPdfDocument *doc = tab->document();
    const int pageCount = doc->pageCount();

    // Determine page range from the dialog selection.
    const int firstPage = (printer.printRange() == QPrinter::PageRange)
                              ? printer.fromPage() - 1
                              : 0;
    const int lastPage  = (printer.printRange() == QPrinter::PageRange)
                              ? qMin(printer.toPage() - 1, pageCount - 1)
                              : pageCount - 1;

    QPainter painter;
    if (!painter.begin(&printer)) {
        QMessageBox::warning(this, tr("Print"), tr("Could not start printing."));
        return;
    }

    for (int page = firstPage; page <= lastPage; ++page) {
        const QSizeF pageSizePt = doc->pagePointSize(page);
        const QRect paintRect   = painter.viewport();

        // Scale page to fill the printable area while preserving aspect ratio.
        const double scaleX = paintRect.width()  / pageSizePt.width();
        const double scaleY = paintRect.height() / pageSizePt.height();
        const double scale  = qMin(scaleX, scaleY);

        const QSize renderSize(
            static_cast<int>(pageSizePt.width()  * scale),
            static_cast<int>(pageSizePt.height() * scale));

        const QImage pageImage = doc->render(page, renderSize);
        if (!pageImage.isNull()) {
            const int offsetX = (paintRect.width()  - renderSize.width())  / 2;
            const int offsetY = (paintRect.height() - renderSize.height()) / 2;
            painter.drawImage(offsetX, offsetY, pageImage);
        }

        if (page < lastPage)
            printer.newPage();

        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }

    painter.end();
#endif
}
