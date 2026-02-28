#ifndef PDFTABWIDGET_H
#define PDFTABWIDGET_H

#include <QWidget>

class QPdfDocument;
class QPdfView;
class QPdfPageNavigator;
class QPdfSearchModel;
class QPdfBookmarkModel;
class QTreeView;
class QSplitter;

class PdfTabWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PdfTabWidget(QWidget *parent = nullptr);

    bool loadFile(const QString &filePath);
    void reloadFile();

    QString filePath() const { return m_filePath; }
    bool hasDocument() const;

    QPdfDocument      *document()      const { return m_document; }
    QPdfView          *view()          const { return m_view; }
    QPdfPageNavigator *navigator()     const { return m_navigator; }
    QPdfSearchModel   *searchModel()   const { return m_searchModel; }
    QPdfBookmarkModel *bookmarkModel() const { return m_bookmarkModel; }
    QTreeView         *tocView()       const { return m_tocView; }

    int    currentPage() const;
    double zoomFactor()  const;

    void setTocVisible(bool visible);
    bool isTocVisible() const;

signals:
    void documentLoaded(const QString &filePath);
    void pageChanged(int page);
    void zoomChanged(double factor);

private:
    void setupUi();

    QPdfDocument      *m_document      = nullptr;
    QPdfView          *m_view          = nullptr;
    QPdfPageNavigator *m_navigator     = nullptr;
    QPdfSearchModel   *m_searchModel   = nullptr;
    QPdfBookmarkModel *m_bookmarkModel = nullptr;

    QSplitter *m_splitter = nullptr;
    QTreeView *m_tocView  = nullptr;

    QString m_filePath;
};

#endif // PDFTABWIDGET_H
