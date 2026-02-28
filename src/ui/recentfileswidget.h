#ifndef RECENTFILESWIDGET_H
#define RECENTFILESWIDGET_H

#include <QWidget>
#include <QList>
#include <QDateTime>
#include <QPixmap>

class QGridLayout;
class QScrollArea;
class QLabel;
class QMenu;
class ThumbnailCache;

struct RecentFileEntry;

class RecentFileCard : public QWidget
{
    Q_OBJECT

public:
    explicit RecentFileCard(const RecentFileEntry &entry, ThumbnailCache *cache, QWidget *parent = nullptr);
    ~RecentFileCard();

signals:
    void clicked(const QString &filePath);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private slots:
    void onThumbnailReady(const QString &filePath, const QPixmap &thumbnail);

signals:
    void showInFolderRequested(const QString &filePath);
    void removeFromRecentRequested(const QString &filePath);

private:
    void setupContextMenu();

    QString m_filePath;
    QString m_displayName;
    QDateTime m_lastOpened;
    QPixmap m_thumbnail;
    ThumbnailCache *m_cache;
    bool m_thumbnailLoaded;
    QMenu *m_contextMenu = nullptr;
};

class RecentFilesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RecentFilesWidget(QWidget *parent = nullptr);
    ~RecentFilesWidget();

    void refreshRecentFiles();
    void setFilterText(const QString &text);

signals:
    void fileClicked(const QString &filePath);
    void showInFolderRequested(const QString &filePath);
    void removeFromRecentRequested(const QString &filePath);

private slots:
    void onRecentFilesChanged();

private:
    void setupUI();
    void populateCards();
    bool matchesFilter(const RecentFileEntry &entry, const QString &filterText) const;

    QScrollArea *m_scrollArea = nullptr;
    QWidget *m_contentWidget = nullptr;
    QGridLayout *m_gridLayout = nullptr;
    QLabel *m_titleLabel = nullptr;
    QLabel *m_noResultsLabel = nullptr;
    ThumbnailCache *m_thumbnailCache = nullptr;
    QString m_filterText;
    static constexpr int CARDS_PER_ROW = 4;
    static constexpr int CARD_WIDTH = 180;
    static constexpr int CARD_HEIGHT = 220;
};

#endif // RECENTFILESWIDGET_H
