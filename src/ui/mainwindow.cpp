#include "mainwindow.h"
#include "tabmanager.h"
#include "homepagewidget.h"
#include "homecontentwidget.h"
#include "settingsdialog.h"
#include "pdftestwidget.h"
#include "../config/sessionmanager.h"
#include "../config/settingsmanager.h"
#include "../config/theme.h"
#include "../config/debuglog.h"
#include <QApplication>
#include <QFile>
#include <QDateTime>
#include <QStyle>
#include <QMenu>

// #region agent log (optional)
static inline void mwLog(const QString &hyp, const QString &msg, const QString &data = {})
{
    DebugLog::write(QStringLiteral("mainwindow.cpp"), hyp, msg, data);
}
// #endregion
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QAction>
#include <QActionGroup>
#include <QEvent>
#include <QResizeEvent>
#include <QKeyEvent>
#include <QCloseEvent>
#include <QTabWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QShortcut>
#include <QIcon>

MainWindow::MainWindow(bool noQss, bool noSession, QWidget *parent)
    : QMainWindow(parent)
{
    // Load persisted settings and apply theme before building any UI.
    AppSettings settings = SettingsManager::instance()->load();
    m_cachedSettings = settings;

    // QStyleSheetStyle (activated when any QSS is set on qApp) does not paint
    // QMainWindow's background by default on Windows — it defers to the native
    // frame.  WA_StyledBackground tells it to honour our QSS background rule.
    setAttribute(Qt::WA_StyledBackground, true);

    if (noQss) {
        qInfo() << "[MainWindow] --no-qss: clearing stylesheet";
        qobject_cast<QApplication*>(QCoreApplication::instance())->setStyleSheet(QString());
    } else {
        ThemeManager::Theme theme = ThemeManager::Dark;
        if (settings.appearance.theme == QStringLiteral("light"))
            theme = ThemeManager::Light;
        else if (settings.appearance.theme == QStringLiteral("system"))
            theme = ThemeManager::System;
        ThemeManager::instance()->applyTheme(theme);
    }

    setupUI();

    // Sync home start page menu actions.
    {
        bool useRecent = (settings.general.homeStartPage != QStringLiteral("browse"));
        if (m_homeRecentAct) m_homeRecentAct->setChecked(useRecent);
        if (m_homeBrowseAct) m_homeBrowseAct->setChecked(!useRecent);
    }

    // Session restore is deferred to restoreSession(), which is called via
    // QTimer::singleShot(0) from main() after w.show().  This ensures the
    // window is visible before any PDF files are opened (which would call
    // PdfRenderService::open() synchronously and hang the main thread).
    // Store the flags so restoreSession() can use them.
    m_noSession = noSession;

    // #region agent log — snapshot after full construction
    mwLog("H-3", "MainWindow ctor done post-fix",
        QString("{\"qssLen\":%1,\"styleName\":\"%2\",\"palWindow\":\"%3\",\"palBase\":\"%4\",\"ownPalWindow\":\"%5\",\"autoFill\":\"%6\",\"styledBg\":\"%7\"}")
            .arg(QString::number(qApp->styleSheet().length()),
                 QApplication::style() ? QString::fromLatin1(QApplication::style()->metaObject()->className()) : QStringLiteral("null"),
                 QApplication::palette().color(QPalette::Window).name(),
                 QApplication::palette().color(QPalette::Base).name(),
                 palette().color(QPalette::Window).name(),
                 autoFillBackground() ? QStringLiteral("true") : QStringLiteral("false"),
                 testAttribute(Qt::WA_StyledBackground) ? QStringLiteral("true") : QStringLiteral("false")));
    // #endregion
}

MainWindow::~MainWindow()
{
    saveSession();
}

void MainWindow::setupUI()
{
    setWindowTitle(QStringLiteral("PDFViewer"));
    resize(1200, 800);

    createMenuBar();
    createToolBar();
    createTabWidget();
    createStatusBar();

    connectSignals();
    setupKeyboardShortcuts();
}

void MainWindow::createMenuBar()
{
    QMenuBar *menuBar = this->menuBar();

    createFileMenu(menuBar);
    createEditMenu(menuBar);
    createViewMenu(menuBar);
    createToolsMenu(menuBar);
    createSignatureMenu(menuBar);
    createHelpMenu(menuBar);
}

void MainWindow::createFileMenu(QMenuBar *menuBar)
{
    QMenu *fileMenu = menuBar->addMenu(tr("&File"));

    m_newTabAct = fileMenu->addAction(tr("New Tab"), this, &MainWindow::onNewTab);
    m_newTabAct->setShortcut(QKeySequence::New);
    m_newTabAct->setIcon(QIcon(QStringLiteral(":/icons/new.svg")));

    m_openFileAct = fileMenu->addAction(tr("Open File..."), this, &MainWindow::onOpenFile);
    m_openFileAct->setShortcut(QKeySequence::Open);
    m_openFileAct->setIcon(QIcon(QStringLiteral(":/icons/open.svg")));

    QMenu *openRecentMenu = fileMenu->addMenu(tr("Open Recent"));
    // Placeholder: will be populated dynamically later

    fileMenu->addSeparator();

    m_saveAct = fileMenu->addAction(tr("Save"), this, &MainWindow::onSave);
    m_saveAct->setShortcut(QKeySequence::Save);
    m_saveAct->setIcon(QIcon(QStringLiteral(":/icons/save.svg")));

    m_saveAsAct = fileMenu->addAction(tr("Save As..."), this, &MainWindow::onSaveAs);
    m_saveAsAct->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S));

    QMenu *exportMenu = fileMenu->addMenu(tr("Export"));
    exportMenu->addAction(tr("Export as Images"));
    exportMenu->addAction(tr("Export as Text"));
    exportMenu->addAction(tr("Export as PDF"));

    fileMenu->addSeparator();

    m_printAct = fileMenu->addAction(tr("Print"), this, &MainWindow::onPrint);
    m_printAct->setShortcut(QKeySequence::Print);
    fileMenu->addAction(tr("Print Preview"));

    fileMenu->addSeparator();

    fileMenu->addAction(tr("Properties"));

    fileMenu->addSeparator();

    m_exitAct = fileMenu->addAction(tr("Exit"), this, &MainWindow::onExit);
    m_exitAct->setShortcut(QKeySequence::Quit);
}

void MainWindow::createEditMenu(QMenuBar *menuBar)
{
    QMenu *editMenu = menuBar->addMenu(tr("&Edit"));

    m_undoAct = editMenu->addAction(tr("Undo"), this, &MainWindow::onUndo);
    m_undoAct->setShortcut(QKeySequence::Undo);

    m_redoAct = editMenu->addAction(tr("Redo"), this, &MainWindow::onRedo);
    m_redoAct->setShortcut(QKeySequence::Redo);

    editMenu->addSeparator();

    m_cutAct = editMenu->addAction(tr("Cut"), this, &MainWindow::onCut);
    m_cutAct->setShortcut(QKeySequence::Cut);

    m_copyAct = editMenu->addAction(tr("Copy"), this, &MainWindow::onCopy);
    m_copyAct->setShortcut(QKeySequence::Copy);

    m_pasteAct = editMenu->addAction(tr("Paste"), this, &MainWindow::onPaste);
    m_pasteAct->setShortcut(QKeySequence::Paste);

    editMenu->addSeparator();

    m_selectAllAct = editMenu->addAction(tr("Select All"), this, &MainWindow::onSelectAll);
    m_selectAllAct->setShortcut(QKeySequence::SelectAll);

    editMenu->addSeparator();

    QAction *findAct = editMenu->addAction(tr("Find"), this, &MainWindow::onFind);
    findAct->setShortcut(QKeySequence::Find);

    m_findReplaceAct = editMenu->addAction(tr("Find & Replace"), this, &MainWindow::onFindReplace);
    m_findReplaceAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_H));

    editMenu->addSeparator();

    m_preferencesAct = editMenu->addAction(tr("Preferences"), this, &MainWindow::onPreferences);
    m_preferencesAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Comma));
    m_preferencesAct->setIcon(QIcon(QStringLiteral(":/icons/settings.svg")));
}

void MainWindow::createViewMenu(QMenuBar *menuBar)
{
    QMenu *viewMenu = menuBar->addMenu(tr("&View"));

    m_zoomInAct = viewMenu->addAction(tr("Zoom In"), this, &MainWindow::onZoomIn);
    m_zoomInAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Plus));

    m_zoomOutAct = viewMenu->addAction(tr("Zoom Out"), this, &MainWindow::onZoomOut);
    m_zoomOutAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Minus));

    m_fitWidthAct = viewMenu->addAction(tr("Fit Width"), this, &MainWindow::onFitWidth);
    m_fitWidthAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_1));

    m_fitPageAct = viewMenu->addAction(tr("Fit Page"), this, &MainWindow::onFitPage);
    m_fitPageAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_2));

    viewMenu->addSeparator();

    m_navigationPaneAct = viewMenu->addAction(tr("Navigation Pane"), this, &MainWindow::onNavigationPane);
    m_navigationPaneAct->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N));
    m_navigationPaneAct->setCheckable(true);

    viewMenu->addAction(tr("Properties Pane"));

    viewMenu->addSeparator();

    m_fullScreenAct = viewMenu->addAction(tr("Full Screen"), this, &MainWindow::onFullScreen);
    m_fullScreenAct->setShortcut(QKeySequence::FullScreen);

    m_darkModeAct = viewMenu->addAction(tr("Dark Mode"), this, &MainWindow::onDarkMode);
    m_darkModeAct->setCheckable(true);
    m_darkModeAct->setChecked(true);

    m_lightModeAct = viewMenu->addAction(tr("Light Mode"), this, &MainWindow::onLightMode);
    m_lightModeAct->setCheckable(true);

    viewMenu->addSeparator();

    m_reloadAct = viewMenu->addAction(tr("Reload"), this, &MainWindow::onReload);
    m_reloadAct->setShortcut(QKeySequence::Refresh);

    viewMenu->addSeparator();

    QMenu *homePageMenu = viewMenu->addMenu(tr("Home Page"));
    m_homePageGroup = new QActionGroup(this);
    
    m_homeRecentAct = new QAction(tr("Recent"), this);
    m_homeRecentAct->setCheckable(true);
    m_homeRecentAct->setChecked(true);
    m_homeRecentAct->setActionGroup(m_homePageGroup);
    connect(m_homeRecentAct, &QAction::triggered, this, &MainWindow::onHomeRecent);
    homePageMenu->addAction(m_homeRecentAct);

    m_homeBrowseAct = new QAction(tr("Browse"), this);
    m_homeBrowseAct->setCheckable(true);
    m_homeBrowseAct->setActionGroup(m_homePageGroup);
    connect(m_homeBrowseAct, &QAction::triggered, this, &MainWindow::onHomeBrowse);
    homePageMenu->addAction(m_homeBrowseAct);
}

void MainWindow::createToolsMenu(QMenuBar *menuBar)
{
    QMenu *toolsMenu = menuBar->addMenu(tr("&Tools"));

    QMenu *annotationsMenu = toolsMenu->addMenu(tr("Annotations"));
    annotationsMenu->addAction(tr("Add Comment"), this, &MainWindow::onAddComment);
    annotationsMenu->addAction(tr("Highlight"), this, &MainWindow::onHighlight);
    annotationsMenu->addAction(tr("Underline"), this, &MainWindow::onUnderline);
    annotationsMenu->addAction(tr("Draw"), this, &MainWindow::onDraw);
    annotationsMenu->addAction(tr("Erase"), this, &MainWindow::onErase);

    toolsMenu->addSeparator();

    toolsMenu->addAction(tr("Text Recognition (OCR)"), this, &MainWindow::onTextRecognition);
    toolsMenu->addAction(tr("Extract Text"), this, &MainWindow::onExtractText);

    toolsMenu->addSeparator();

    QMenu *fileManagementMenu = toolsMenu->addMenu(tr("File Management"));
    fileManagementMenu->addAction(tr("Rename"), this, &MainWindow::onRename);
    fileManagementMenu->addAction(tr("Duplicate"), this, &MainWindow::onDuplicate);
    fileManagementMenu->addAction(tr("Move to Trash"), this, &MainWindow::onMoveToTrash);
    fileManagementMenu->addAction(tr("Show in Folder"), this, &MainWindow::onShowInFolder);

    // Developer-only: PDF rendering test harness.
    toolsMenu->addSeparator();
    QAction* devRenderAct = toolsMenu->addAction(
        tr("Dev: Render Test…"), this, &MainWindow::onTestPdfRenderer);
    devRenderAct->setStatusTip(tr("Open the PDF renderer test harness (developer tool)"));
}

void MainWindow::createSignatureMenu(QMenuBar *menuBar)
{
    QMenu *signatureMenu = menuBar->addMenu(tr("&Signature"));

    signatureMenu->addAction(tr("Sign Document"), this, &MainWindow::onSignDocument);
    signatureMenu->addAction(tr("Add Signature Field"), this, &MainWindow::onAddSignatureField);

    signatureMenu->addSeparator();

    signatureMenu->addAction(tr("Verify Signatures"), this, &MainWindow::onVerifySignatures);
    signatureMenu->addAction(tr("Remove Signature"), this, &MainWindow::onRemoveSignature);

    signatureMenu->addSeparator();

    signatureMenu->addAction(tr("Certificate Manager"), this, &MainWindow::onCertificateManager);
    signatureMenu->addAction(tr("Digital ID"), this, &MainWindow::onDigitalID);
}

void MainWindow::createHelpMenu(QMenuBar *menuBar)
{
    QMenu *helpMenu = menuBar->addMenu(tr("&Help"));

    helpMenu->addAction(tr("Getting Started"), this, &MainWindow::onGettingStarted);
    helpMenu->addAction(tr("Documentation"), this, &MainWindow::onDocumentation);

    m_keyboardShortcutsAct = helpMenu->addAction(tr("Keyboard Shortcuts"), this, &MainWindow::onKeyboardShortcuts);
    m_keyboardShortcutsAct->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Question));

    helpMenu->addSeparator();

    helpMenu->addAction(tr("Report Bug"), this, &MainWindow::onReportBug);
    helpMenu->addAction(tr("Request Feature"), this, &MainWindow::onRequestFeature);

    helpMenu->addSeparator();

    helpMenu->addAction(tr("About PDFViewer"), this, &MainWindow::onAbout);
    helpMenu->addAction(tr("Check for Updates"), this, &MainWindow::onCheckUpdates);
}

void MainWindow::createToolBar()
{
    QToolBar *toolBar = addToolBar(tr("Main Toolbar"));
    toolBar->setObjectName(QStringLiteral("mainToolbar"));
    toolBar->setMovable(false);

    toolBar->addAction(m_newTabAct);
    toolBar->addAction(m_openFileAct);
    toolBar->addAction(m_saveAct);
    toolBar->addSeparator();
    toolBar->addAction(m_printAct);
    toolBar->addSeparator();
    toolBar->addAction(m_preferencesAct);
}

void MainWindow::createTabWidget()
{
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setObjectName(QStringLiteral("mainTabWidget"));
    m_tabManager = new TabManager(m_tabWidget, this);

    connect(m_tabManager, &TabManager::tabChanged, this, &MainWindow::onTabChanged);

    setCentralWidget(m_tabWidget);
}

void MainWindow::createStatusBar()
{
    QStatusBar *statusBar = this->statusBar();
    statusBar->setObjectName(QStringLiteral("mainStatusBar"));
    statusBar->showMessage(tr("Ready"));
}

void MainWindow::connectSignals()
{
    // Theme changes
    connect(ThemeManager::instance(), &ThemeManager::themeChanged,
            this, [this](ThemeManager::Theme theme) {
        if (m_darkModeAct)  m_darkModeAct->setChecked(theme == ThemeManager::Dark);
        if (m_lightModeAct) m_lightModeAct->setChecked(theme == ThemeManager::Light);
    });
}

void MainWindow::setupKeyboardShortcuts()
{
    // Tab shortcuts are handled in keyPressEvent
    // Additional shortcuts can be added here if needed
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        // Ctrl+T: New Tab
        if (event->key() == Qt::Key_T && !(event->modifiers() & Qt::ShiftModifier)) {
            onNewTab();
            return;
        }

        // Ctrl+Tab: Next Tab
        if (event->key() == Qt::Key_Tab && !(event->modifiers() & Qt::ShiftModifier)) {
            if (m_tabManager) {
                m_tabManager->switchToNextTab();
            }
            return;
        }

        // Ctrl+Shift+Tab: Previous Tab
        if (event->key() == Qt::Key_Tab && (event->modifiers() & Qt::ShiftModifier)) {
            if (m_tabManager) {
                m_tabManager->switchToPreviousTab();
            }
            return;
        }

        // Ctrl+1 through Ctrl+8: Jump to tab
        if (event->key() >= Qt::Key_1 && event->key() <= Qt::Key_8) {
            if (m_tabManager) {
                int tabNum = event->key() - Qt::Key_1;
                m_tabManager->switchToTabNumber(tabNum + 1);
            }
            return;
        }

        // Ctrl+9: Last tab
        if (event->key() == Qt::Key_9) {
            if (m_tabManager) {
                m_tabManager->switchToLastTab();
            }
            return;
        }

        // Ctrl+W: Close current tab
        if (event->key() == Qt::Key_W && !(event->modifiers() & Qt::ShiftModifier)) {
            if (m_tabManager && m_tabManager->currentIndex() >= 0) {
                m_tabManager->closeTab(m_tabManager->currentIndex());
            }
            return;
        }

        // Ctrl+Shift+W: Close all tabs
        if (event->key() == Qt::Key_W && (event->modifiers() & Qt::ShiftModifier)) {
            if (m_tabManager) {
                m_tabManager->closeAllTabs();
            }
            return;
        }

        // Ctrl+F: Focus search bar (when Home tab is active)
        if (event->key() == Qt::Key_F && !(event->modifiers() & Qt::ShiftModifier)) {
            onFind();
            return;
        }
    }

    QMainWindow::keyPressEvent(event);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveSession();
    event->accept();
}

void MainWindow::saveSession()
{
    if (!m_tabManager) {
        return;
    }

    QList<PDFViewerTab*> tabs = m_tabManager->getAllTabs();
    QList<TabSessionData> sessionTabs;

    int activeIndex = m_tabManager->currentIndex();

    for (PDFViewerTab* tab : tabs) {
        TabSessionData sessionTab;
        if (tab->isHomeTab) {
            sessionTab.type = QStringLiteral("home");
        } else {
            sessionTab.type = QStringLiteral("pdf");
            sessionTab.filePath = tab->filePath;
        }
        sessionTab.currentPage = tab->currentPage;
        sessionTab.zoomLevel = tab->zoomLevel;
        sessionTab.isActive = (tabs.indexOf(tab) == activeIndex);
        sessionTabs.append(sessionTab);
    }

    SessionManager::instance()->saveSession(sessionTabs, activeIndex);
}

void MainWindow::restoreSession()
{
    // Called after show() so the window is already visible when PDFs are opened.
    if (!m_noSession && m_cachedSettings.general.reopenTabsOnStartup) {
        loadSession();
    }
}

void MainWindow::loadSession()
{
    if (!m_tabManager) {
        return;
    }

    QList<TabSessionData> tabs;
    int activeTabIndex = 0;

    if (!SessionManager::instance()->loadSession(tabs, activeTabIndex)) {
        // No session to restore, keep the default home tab created by TabManager
        return;
    }

    // Clear existing tabs (the default home tab created by TabManager)
    while (m_tabManager->tabCount() > 0) {
        m_tabManager->closeTab(0);
    }

    // Restore tabs from session
    for (const TabSessionData& tabData : tabs) {
        bool isHome = (tabData.type == QStringLiteral("home"));
        m_tabManager->restoreTab(tabData.filePath, tabData.currentPage, tabData.zoomLevel, isHome, tabData.isActive);
    }

    // Ensure at least one tab exists (home tab)
    if (m_tabManager->tabCount() == 0) {
        m_tabManager->createNewTab(true);
    }

    // Set active tab
    if (activeTabIndex >= 0 && activeTabIndex < m_tabManager->tabCount()) {
        m_tabManager->switchToTab(activeTabIndex);
    } else if (m_tabManager->tabCount() > 0) {
        // Fallback to first tab if activeTabIndex is invalid
        m_tabManager->switchToTab(0);
    }
}

// File menu slots
void MainWindow::onNewTab()
{
    if (m_tabManager) {
        m_tabManager->createNewTab(false);
    }
}

void MainWindow::onOpenFile()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open PDF File"), QString(),
                                                     tr("PDF Files (*.pdf)"));
    if (!fileName.isEmpty() && m_tabManager) {
        m_tabManager->openPDFInNewTab(fileName);
    }
}

void MainWindow::onSave()
{
    // Placeholder: will be implemented when PDF saving is added
}

void MainWindow::onSaveAs()
{
    // Placeholder: will be implemented when PDF saving is added
}

void MainWindow::onPrint()
{
    // Placeholder: will be implemented when printing is added
}

void MainWindow::onExit()
{
    close();
}

// Edit menu slots
void MainWindow::onUndo()
{
    // Placeholder
}

void MainWindow::onRedo()
{
    // Placeholder
}

void MainWindow::onCut()
{
    // Forward to HomeContentWidget if Browse view is active
    if (m_tabManager) {
        PDFViewerTab* tab = m_tabManager->currentTab();
        if (tab && tab->isHomeTab && tab->widget) {
            HomePageWidget* homePage = qobject_cast<HomePageWidget*>(tab->widget);
            if (homePage && homePage->contentWidget()) {
                HomeContentWidget* content = homePage->contentWidget();
                if (content->activeHomePage() == HomeStartPage::Browse) {
                    content->onCut();
                }
            }
        }
    }
}

void MainWindow::onCopy()
{
    // Forward to HomeContentWidget if Browse view is active
    if (m_tabManager) {
        PDFViewerTab* tab = m_tabManager->currentTab();
        if (tab && tab->isHomeTab && tab->widget) {
            HomePageWidget* homePage = qobject_cast<HomePageWidget*>(tab->widget);
            if (homePage && homePage->contentWidget()) {
                HomeContentWidget* content = homePage->contentWidget();
                if (content->activeHomePage() == HomeStartPage::Browse) {
                    content->onCopy();
                }
            }
        }
    }
}

void MainWindow::onPaste()
{
    // Forward to HomeContentWidget if Browse view is active
    if (m_tabManager) {
        PDFViewerTab* tab = m_tabManager->currentTab();
        if (tab && tab->isHomeTab && tab->widget) {
            HomePageWidget* homePage = qobject_cast<HomePageWidget*>(tab->widget);
            if (homePage && homePage->contentWidget()) {
                HomeContentWidget* content = homePage->contentWidget();
                if (content->activeHomePage() == HomeStartPage::Browse) {
                    content->onPaste();
                }
            }
        }
    }
}

void MainWindow::onSelectAll()
{
    // Forward to HomeContentWidget if Browse view is active
    if (m_tabManager) {
        PDFViewerTab* tab = m_tabManager->currentTab();
        if (tab && tab->isHomeTab && tab->widget) {
            HomePageWidget* homePage = qobject_cast<HomePageWidget*>(tab->widget);
            if (homePage && homePage->contentWidget()) {
                HomeContentWidget* content = homePage->contentWidget();
                if (content->activeHomePage() == HomeStartPage::Browse) {
                    content->selectAll();
                }
            }
        }
    }
}

void MainWindow::onFind()
{
    // Focus search bar if Home tab is active
    if (m_tabManager) {
        PDFViewerTab* tab = m_tabManager->currentTab();
        if (tab && tab->isHomeTab && tab->widget) {
            HomePageWidget* homePage = qobject_cast<HomePageWidget*>(tab->widget);
            if (homePage) {
                homePage->focusSearchBar();
            }
        }
    }
}

void MainWindow::onFindReplace()
{
    // Placeholder
}

void MainWindow::onPreferences()
{
    SettingsDialog dlg(this);

    // Qt::UniqueConnection cannot be used with a lambda slot — it requires a
    // pointer-to-member-function of a QObject subclass and will assert at
    // runtime otherwise.  Use the member-function-pointer form so that
    // UniqueConnection correctly deduplicates if the dialog is opened twice.
    connect(SettingsManager::instance(), &SettingsManager::settingsApplied,
            this, &MainWindow::onSettingsApplied,
            Qt::UniqueConnection);

    dlg.exec();
}

void MainWindow::onSettingsApplied(const AppSettings& s)
{
    // Sync home page menu actions with new setting.
    bool useRecent = (s.general.homeStartPage != QStringLiteral("browse"));
    if (m_homeRecentAct) m_homeRecentAct->setChecked(useRecent);
    if (m_homeBrowseAct) m_homeBrowseAct->setChecked(!useRecent);

    // If a home tab is open, switch its view to match the new default.
    if (m_tabManager) {
        for (PDFViewerTab* tab : m_tabManager->getAllTabs()) {
            if (tab && tab->isHomeTab && tab->widget) {
                HomePageWidget* hp = qobject_cast<HomePageWidget*>(tab->widget);
                if (hp && hp->contentWidget()) {
                    HomeStartPage page = useRecent ? HomeStartPage::Recent : HomeStartPage::Browse;
                    hp->contentWidget()->setActiveHomePage(page);
                }
            }
        }
    }

    // Sync View menu Dark/Light mode checkboxes.
    if (m_darkModeAct && m_lightModeAct) {
        bool isDark = (s.appearance.theme != QStringLiteral("light"));
        m_darkModeAct->setChecked(isDark);
        m_lightModeAct->setChecked(!isDark);
    }
}

// View menu slots
void MainWindow::onZoomIn()
{
    // Placeholder: will be implemented when PDF rendering is added
}

void MainWindow::onZoomOut()
{
    // Placeholder: will be implemented when PDF rendering is added
}

void MainWindow::onFitWidth()
{
    // Placeholder: will be implemented when PDF rendering is added
}

void MainWindow::onFitPage()
{
    // Placeholder: will be implemented when PDF rendering is added
}

void MainWindow::onNavigationPane()
{
    // Placeholder: will toggle navigation pane
}

void MainWindow::onFullScreen()
{
    if (isFullScreen()) {
        showNormal();
    } else {
        showFullScreen();
    }
}

void MainWindow::onDarkMode()
{
    ThemeManager::instance()->applyTheme(ThemeManager::Dark);
    m_darkModeAct->setChecked(true);
    m_lightModeAct->setChecked(false);
}

void MainWindow::onLightMode()
{
    ThemeManager::instance()->applyTheme(ThemeManager::Light);
    m_darkModeAct->setChecked(false);
    m_lightModeAct->setChecked(true);
}

void MainWindow::onReload()
{
    // Placeholder: will reload current PDF
}

void MainWindow::onHomeRecent()
{
    if (m_tabManager) {
        PDFViewerTab* tab = m_tabManager->currentTab();
        if (tab && tab->isHomeTab && tab->widget) {
            HomePageWidget* homePage = qobject_cast<HomePageWidget*>(tab->widget);
            if (homePage && homePage->contentWidget()) {
                homePage->contentWidget()->setActiveHomePage(HomeStartPage::Recent);
                m_homeRecentAct->setChecked(true);
                m_homeBrowseAct->setChecked(false);
            }
        }
    }
}

void MainWindow::onHomeBrowse()
{
    if (m_tabManager) {
        PDFViewerTab* tab = m_tabManager->currentTab();
        if (tab && tab->isHomeTab && tab->widget) {
            HomePageWidget* homePage = qobject_cast<HomePageWidget*>(tab->widget);
            if (homePage && homePage->contentWidget()) {
                homePage->contentWidget()->setActiveHomePage(HomeStartPage::Browse);
                m_homeRecentAct->setChecked(false);
                m_homeBrowseAct->setChecked(true);
            }
        }
    }
}

// Tools menu slots (placeholders)
void MainWindow::onAddComment() {}
void MainWindow::onHighlight() {}
void MainWindow::onUnderline() {}
void MainWindow::onDraw() {}
void MainWindow::onErase() {}
void MainWindow::onTextRecognition() {}
void MainWindow::onExtractText() {}
void MainWindow::onRename() {}
void MainWindow::onDuplicate() {}
void MainWindow::onMoveToTrash() {}
void MainWindow::onShowInFolder() {}

// Signature menu slots (placeholders)
void MainWindow::onSignDocument() {}
void MainWindow::onAddSignatureField() {}
void MainWindow::onVerifySignatures() {}
void MainWindow::onRemoveSignature() {}
void MainWindow::onCertificateManager() {}
void MainWindow::onDigitalID() {}

// Help menu slots
void MainWindow::onGettingStarted()
{
    QMessageBox::information(this, tr("Getting Started"), tr("Getting Started guide will be available soon."));
}

void MainWindow::onDocumentation()
{
    QMessageBox::information(this, tr("Documentation"), tr("Documentation will be available soon."));
}

void MainWindow::onKeyboardShortcuts()
{
    QString shortcuts = tr(
        "Tab Management:\n"
        "  Ctrl+T          New Tab\n"
        "  Ctrl+Tab         Next Tab\n"
        "  Ctrl+Shift+Tab   Previous Tab\n"
        "  Ctrl+1-8         Jump to Tab 1-8\n"
        "  Ctrl+9           Last Tab\n"
        "  Ctrl+W           Close Tab\n"
        "  Ctrl+Shift+W     Close All Tabs\n\n"
        "File Operations:\n"
        "  Ctrl+O          Open File\n"
        "  Ctrl+S          Save\n"
        "  Ctrl+Shift+S    Save As\n"
        "  Ctrl+P          Print\n\n"
        "View:\n"
        "  Ctrl++          Zoom In\n"
        "  Ctrl+-          Zoom Out\n"
        "  Ctrl+1          Fit Width\n"
        "  Ctrl+2          Fit Page\n"
        "  F11             Full Screen\n"
        "  Ctrl+R          Reload\n\n"
        "Application:\n"
        "  Ctrl+,          Preferences\n"
        "  Ctrl+Q          Exit\n"
    );
    QMessageBox::information(this, tr("Keyboard Shortcuts"), shortcuts);
}

void MainWindow::onReportBug()
{
    QMessageBox::information(this, tr("Report Bug"), tr("Bug reporting will be available soon."));
}

void MainWindow::onRequestFeature()
{
    QMessageBox::information(this, tr("Request Feature"), tr("Feature requests will be available soon."));
}

void MainWindow::onAbout()
{
    QMessageBox::about(this, tr("About PDFViewer"),
                       tr("PDFViewer\n\n"
                          "A modern PDF viewer application.\n\n"
                          "Version 1.0.0"));
}

void MainWindow::onCheckUpdates()
{
    QMessageBox::information(this, tr("Check for Updates"), tr("Update checking will be available soon."));
}

void MainWindow::onTestPdfRenderer()
{
    PdfTestWidget* dlg = new PdfTestWidget(this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->show();
}

void MainWindow::onTabChanged(int index)
{
    Q_UNUSED(index);
    // Update status bar or other UI elements based on current tab
    if (m_tabManager) {
        PDFViewerTab* tab = m_tabManager->currentTab();
        if (tab) {
            QString status = tab->isHomeTab ? tr("Home") : tr("Ready");
            statusBar()->showMessage(status);
            
            // Update Home page menu actions state and connect to homePageChanged signal
            if (tab->isHomeTab && tab->widget) {
                HomePageWidget* homePage = qobject_cast<HomePageWidget*>(tab->widget);
                if (homePage && homePage->contentWidget()) {
                    HomeContentWidget* content = homePage->contentWidget();
                    HomeStartPage currentPage = content->activeHomePage();
                    m_homeRecentAct->setChecked(currentPage == HomeStartPage::Recent);
                    m_homeBrowseAct->setChecked(currentPage == HomeStartPage::Browse);
                    
                    // Qt::UniqueConnection requires a pointer-to-member-function
                    // slot — it asserts (and crashes) when given a lambda.
                    // Use the named slot form so UniqueConnection can deduplicate
                    // correctly when tabs are switched back and forth.
                    connect(content, &HomeContentWidget::homePageChanged,
                            this, &MainWindow::onHomePageChanged,
                            Qt::UniqueConnection);
                }
            }
        }
    }
}

void MainWindow::onHomePageChanged(HomeStartPage page)
{
    if (m_homeRecentAct) m_homeRecentAct->setChecked(page == HomeStartPage::Recent);
    if (m_homeBrowseAct) m_homeBrowseAct->setChecked(page == HomeStartPage::Browse);
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        setWindowTitle(tr("PDFViewer"));
        statusBar()->showMessage(tr("Ready"));
    }
    QMainWindow::changeEvent(event);
}
