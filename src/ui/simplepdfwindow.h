#ifndef SIMPLEPDFWINDOW_H
#define SIMPLEPDFWINDOW_H

#include <QMainWindow>

class QAction;
class QDragEnterEvent;
class QDropEvent;
class QEvent;
class QLabel;
class QMenu;
class QSpinBox;

class PdfTabWidget;

/**
 * Barebones single-document PDF viewer.
 * One window, one PDF. Open replaces the current document. No tabs, session, settings, or extra features.
 */
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

    void onBookmarkActivated(const QModelIndex &index);
    void setContinuousScroll(bool enabled);
    void showAbout();

private:
    void createUi();
    void createMenuBar();

    PdfTabWidget *viewer() const { return m_viewer; }

    void updateWindowTitle();
    void updatePageControls();
    void updateStatusLabel();

    void scheduleZoomAnchorJump(PdfTabWidget *tab, int page);
    void zoomAnchoredToViewportCenter(PdfTabWidget *tab, double newZoom);

    void saveWindowGeometry();
    void restoreWindowGeometry();
    void restoreViewerPreferences();

    PdfTabWidget *m_viewer = nullptr;

    QAction *m_continuousScrollAction = nullptr;
    QSpinBox *m_pageSpin             = nullptr;
    QLabel   *m_pageCountLabel = nullptr;
    QLabel   *m_statusLabel    = nullptr;
};

#endif // SIMPLEPDFWINDOW_H
