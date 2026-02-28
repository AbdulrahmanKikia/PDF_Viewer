#ifndef HOMECONTENTWIDGET_H
#define HOMECONTENTWIDGET_H

#include <QWidget>
#include <QModelIndex>

class QFileSystemModel;
class QSortFilterProxyModel;
class QTreeView;
class QListView;
class QLineEdit;
class QVBoxLayout;
class QHBoxLayout;
class QLabel;
class QStackedWidget;
class QPushButton;
class QButtonGroup;
class QTimer;
class QMenu;
class RecentFilesWidget;
class FileClipboard;

enum class ViewMode {
    List,
    Grid,
    Compact
};

enum class HomeStartPage {
    Recent,
    Browse
};

class HomeContentWidget : public QWidget
{
    Q_OBJECT

public:
    explicit HomeContentWidget(QWidget *parent = nullptr);
    ~HomeContentWidget();

    void setRootPath(const QString &path);
    void setFilterText(const QString &text);
    void setViewMode(ViewMode mode);
    void setActiveHomePage(HomeStartPage page);
    HomeStartPage activeHomePage() const { return m_currentHomePage; }
    void focusSearchBar();
    QString currentSearchText() const;
    void selectAll();

signals:
    void openFileRequested(const QString &path);
    void homePageChanged(HomeStartPage page);

public slots:
    void onCopy();
    void onCut();
    void onPaste();
    void onDelete();
    void onRename();

private slots:
    void initDeferred();
    void onItemDoubleClicked(const QModelIndex &index);
    void onViewModeChanged(int id);
    void onHomePageToggleClicked(int id);
    void onSearchTextChanged(const QString &text);
    void applySearchFilter();
    void showBrowseContextMenu(const QPoint &pos);
    void onDuplicate();
    void onProperties();
    void onShowInFolder();

private:
    void setupUI();
    void updateViewMode();
    void saveHomePageSetting();
    void loadHomePageSetting();
    bool isPdfFile(const QString &path) const;
    void setupBrowseContextMenu();
    QString getCurrentFolderPath() const;
    QString generateUniqueFileName(const QString &basePath, const QString &fileName) const;
    bool moveToTrash(const QString &path) const;
    void showInFolder(const QString &path);

    QFileSystemModel *m_fileSystemModel = nullptr;
    QSortFilterProxyModel *m_proxyModel = nullptr;
    QTreeView *m_treeView = nullptr;
    QListView *m_listView = nullptr;  // Placeholder for grid/list views
    RecentFilesWidget *m_recentFilesWidget = nullptr;
    QLineEdit *m_searchEdit = nullptr;
    QLabel *m_titleLabel = nullptr;
    QStackedWidget *m_mainStack = nullptr;  // Recent vs Browse
    QStackedWidget *m_browseStack = nullptr;  // List/Grid/Compact modes
    QPushButton *m_recentPageBtn = nullptr;
    QPushButton *m_browsePageBtn = nullptr;
    QButtonGroup *m_homePageGroup = nullptr;
    QPushButton *m_listModeBtn = nullptr;
    QPushButton *m_gridModeBtn = nullptr;
    QPushButton *m_compactModeBtn = nullptr;
    QButtonGroup *m_viewModeGroup = nullptr;
    QTimer *m_searchDebounceTimer = nullptr;
    QMenu *m_browseContextMenu = nullptr;
    FileClipboard *m_clipboard = nullptr;
    QModelIndex m_contextMenuIndex;
    ViewMode m_currentViewMode = ViewMode::List;
    HomeStartPage m_currentHomePage = HomeStartPage::Recent;
    QString m_currentSearchText;
};

#endif // HOMECONTENTWIDGET_H
