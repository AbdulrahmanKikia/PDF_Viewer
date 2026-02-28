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

private:
    void createUi();
    void createMenuBar();

    PdfTabWidget *currentTab() const;
    PdfTabWidget *openInNewTab(const QString &filePath = QString());
    void updateTabTitle(int index);
    void syncUiToCurrentTab();
    void updateWindowTitle();
    void updatePageControls();
    void updateStatusLabel();
    void installTabEventFilter(PdfTabWidget *tab);

    void saveWindowGeometry();
    void restoreWindowGeometry();
    void saveSession();
    void restoreSession();

    QTabWidget *m_tabWidget = nullptr;

    QLineEdit *m_searchEdit     = nullptr;
    QSpinBox  *m_pageSpin       = nullptr;
    QLabel    *m_pageCountLabel = nullptr;
    QLabel    *m_statusLabel    = nullptr;

    QMenu *m_recentFilesMenu = nullptr;

    QAction *m_darkModeAction  = nullptr;
    QAction *m_lightModeAction = nullptr;
    QAction *m_navPaneAction   = nullptr;

    bool m_isFullScreen = false;
};

#endif // SIMPLEPDFWINDOW_H
