#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QKeyEvent>
#include "../config/settingsmanager.h"
#include "homecontentwidget.h"   // HomeStartPage enum

class QMenuBar;
class QToolBar;
class QStatusBar;
class QWidget;
class QTabWidget;
class QMenu;
class QAction;
class QCloseEvent;
class TabManager;
class SessionManager;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    // noQss  – skip all stylesheet loading (--no-qss flag)
    // noSession – skip session restore; always open a fresh home tab
    explicit MainWindow(bool noQss = false, bool noSession = false,
                        QWidget *parent = nullptr);
    ~MainWindow();

    // Called via QTimer::singleShot(0) from main() after show(), so the window
    // is already painted before any PDF files are opened from the saved session.
    void restoreSession();

protected:
    void changeEvent(QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private slots:
    // File menu
    void onNewTab();
    void onOpenFile();
    void onSave();
    void onSaveAs();
    void onPrint();
    void onExit();

    // Edit menu
    void onUndo();
    void onRedo();
    void onCut();
    void onCopy();
    void onPaste();
    void onSelectAll();
    void onFind();
    void onFindReplace();
    void onPreferences();

    // View menu
    void onZoomIn();
    void onZoomOut();
    void onFitWidth();
    void onFitPage();
    void onNavigationPane();
    void onFullScreen();
    void onDarkMode();
    void onLightMode();
    void onReload();
    void onHomeRecent();
    void onHomeBrowse();

    // Tools menu (placeholders)
    void onAddComment();
    void onHighlight();
    void onUnderline();
    void onDraw();
    void onErase();
    void onTextRecognition();
    void onExtractText();
    void onRename();
    void onDuplicate();
    void onMoveToTrash();
    void onShowInFolder();

    // Signature menu (placeholders)
    void onSignDocument();
    void onAddSignatureField();
    void onVerifySignatures();
    void onRemoveSignature();
    void onCertificateManager();
    void onDigitalID();

    // Help menu
    void onGettingStarted();
    void onDocumentation();
    void onKeyboardShortcuts();
    void onReportBug();
    void onRequestFeature();
    void onAbout();
    void onCheckUpdates();
    void onTestPdfRenderer();

    // Tab management
    void onTabChanged(int index);
    void onHomePageChanged(HomeStartPage page);
    void onSettingsApplied(const AppSettings& settings);

private:
    void setupUI();
    void createMenuBar();
    void createToolBar();
    void createStatusBar();
    void createTabWidget();
    void connectSignals();
    void setupKeyboardShortcuts();
    void saveSession();
    void loadSession();

    // Menu creation helpers
    void createFileMenu(QMenuBar *menuBar);
    void createEditMenu(QMenuBar *menuBar);
    void createViewMenu(QMenuBar *menuBar);
    void createToolsMenu(QMenuBar *menuBar);
    void createSignatureMenu(QMenuBar *menuBar);
    void createHelpMenu(QMenuBar *menuBar);

    // Menu actions
    QAction *m_newTabAct = nullptr;
    QAction *m_openFileAct = nullptr;
    QAction *m_saveAct = nullptr;
    QAction *m_saveAsAct = nullptr;
    QAction *m_printAct = nullptr;
    QAction *m_exitAct = nullptr;

    QAction *m_undoAct = nullptr;
    QAction *m_redoAct = nullptr;
    QAction *m_cutAct = nullptr;
    QAction *m_copyAct = nullptr;
    QAction *m_pasteAct = nullptr;
    QAction *m_selectAllAct = nullptr;
    QAction *m_findReplaceAct = nullptr;
    QAction *m_preferencesAct = nullptr;

    QAction *m_zoomInAct = nullptr;
    QAction *m_zoomOutAct = nullptr;
    QAction *m_fitWidthAct = nullptr;
    QAction *m_fitPageAct = nullptr;
    QAction *m_navigationPaneAct = nullptr;
    QAction *m_fullScreenAct = nullptr;
    QAction *m_darkModeAct = nullptr;
    QAction *m_lightModeAct = nullptr;
    QAction *m_reloadAct = nullptr;
    QAction *m_homeRecentAct = nullptr;
    QAction *m_homeBrowseAct = nullptr;
    QActionGroup *m_homePageGroup = nullptr;

    QAction *m_keyboardShortcutsAct = nullptr;

    QTabWidget *m_tabWidget = nullptr;
    TabManager *m_tabManager = nullptr;
    QWidget *m_centralPlaceholder = nullptr;

    // Stored for use by restoreSession() which runs after show().
    AppSettings m_cachedSettings;
    bool m_noSession = false;
};

#endif // MAINWINDOW_H
