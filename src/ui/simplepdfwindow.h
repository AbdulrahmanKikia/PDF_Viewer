#ifndef SIMPLEPDFWINDOW_H
#define SIMPLEPDFWINDOW_H

#include <QMainWindow>

class QAction;
class QDragEnterEvent;
class QDropEvent;
class QEvent;
class QMenu;
class QLineEdit;
class QSpinBox;
class QLabel;
class QTabWidget;

class PdfTabWidget;
class HomePageWidget;

class SimplePdfWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit SimplePdfWindow(QWidget *parent = nullptr);

    void openFilePath(const QString &filePath);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void openFile();
    void newTab();
    void closeCurrentTab();
    void closeTab(int index);
    void nextTab();
    void previousTab();
    void onTabChanged(int index);
    void onTabCloseRequested(int index);

    void goNextPage();
    void goPreviousPage();
    void goFirstPage();
    void goLastPage();
    void gotoPageFromSpin();

    void zoomIn();
    void zoomOut();
    void fitWidth();
    void fitPage();
    void resetZoom();

    void onSearchTextChanged(const QString &text);
    void findNext();
    void findPrevious();
    void focusSearchBar();
    void clearSearch();

    void onBookmarkActivated(const QModelIndex &index);

    void toggleFullScreen();
    void toggleDarkMode();
    void toggleLightMode();
    void reloadDocument();
    void showInFolder();
    void showAbout();
    void showKeyboardShortcuts();

    void populateRecentFilesMenu();
    void showTabContextMenu(const QPoint &pos);

    void showSettings();
    void printDocument();

private:
    void createUi();
    void createMenuBar();

    void applySettingsToApp();

    PdfTabWidget    *currentTab()  const;
    HomePageWidget  *openHomeTab();
    HomePageWidget  *homeTab() const;
    bool             isHomeTab(int index) const;

    PdfTabWidget *openInNewTab(const QString &filePath = QString());
    /** Opens a PDF respecting General tab open behavior (new / replace / ask). */
    void openFileWithTabBehavior(const QString &filePath);
    void updateTabTitle(int index);
    void syncUiToCurrentTab();
    void updateWindowTitle();
    void updatePageControls();
    void updateStatusLabel();
    void installTabEventFilter(PdfTabWidget *tab);

    /** Schedules a jump to \a page after the next event loop run (after Qt's layout update from zoom). */
    void scheduleZoomAnchorJump(PdfTabWidget *tab, int page);
    /** Zooms to \a newZoom keeping the current viewport center fixed (for zoom in/out and Ctrl+wheel). */
    void zoomAnchoredToViewportCenter(PdfTabWidget *tab, double newZoom);

    void saveWindowGeometry();
    void restoreWindowGeometry();
    void saveSession();
    void restoreSession();

    QTabWidget *m_tabWidget = nullptr;

    QLineEdit *m_searchEdit     = nullptr;
    QSpinBox  *m_pageSpin       = nullptr;
    QLabel    *m_pageCountLabel = nullptr;
    QLabel    *m_statusLabel    = nullptr;

    QMenu   *m_recentFilesMenu = nullptr;
    QAction *m_darkModeAction  = nullptr;
    QAction *m_lightModeAction = nullptr;
    QAction *m_navPaneAction   = nullptr;
    QAction *m_printAct        = nullptr;

    HomePageWidget *m_homeTab    = nullptr;
    bool            m_isFullScreen = false;
};

#endif // SIMPLEPDFWINDOW_H
