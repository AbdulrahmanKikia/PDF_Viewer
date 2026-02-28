#ifndef SIMPLEPDFWINDOW_H
#define SIMPLEPDFWINDOW_H

#include <QMainWindow>

class QEvent;
class QPdfDocument;
class QPdfView;
class QPdfPageNavigator;
class QPdfSearchModel;
class QPdfBookmarkModel;
class QLineEdit;
class QSpinBox;
class QLabel;
class QTreeView;
class QSplitter;

class SimplePdfWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit SimplePdfWindow(QWidget *parent = nullptr);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void openFile();

    void goNextPage();
    void goPreviousPage();
    void gotoPageFromSpin();

    void zoomIn();
    void zoomOut();
    void fitWidth();
    void fitPage();
    void resetZoom();

    void onSearchTextChanged(const QString &text);
    void findNext();
    void findPrevious();

    void onBookmarkActivated(const QModelIndex &index);

    void onDocumentStatusChanged();
    void onCurrentPageChanged();

private:
    void createUi();
    void createActions();
    void updateWindowTitle(const QString &filePath);
    void updatePageControls();
    void updateStatusLabel();

    QPdfDocument      *m_document      = nullptr;
    QPdfView          *m_view          = nullptr;
    QPdfPageNavigator *m_navigator     = nullptr;
    QPdfSearchModel   *m_searchModel   = nullptr;
    QPdfBookmarkModel *m_bookmarkModel = nullptr;

    QSplitter *m_splitter   = nullptr;
    QTreeView *m_tocView    = nullptr;

    QLineEdit *m_searchEdit = nullptr;
    QSpinBox  *m_pageSpin   = nullptr;
    QLabel    *m_pageCountLabel = nullptr;
    QLabel    *m_statusLabel    = nullptr;

    QString m_currentFilePath;
};

#endif // SIMPLEPDFWINDOW_H

