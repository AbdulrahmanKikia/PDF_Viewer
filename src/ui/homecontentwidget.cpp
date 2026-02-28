#include "homecontentwidget.h"
#include "recentfileswidget.h"
#include "../config/fileclipboard.h"
#include "../config/settingsmanager.h"
#include "../config/debuglog.h"
#include "filepropertiesdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTreeView>
#include <QListView>
#include <QLineEdit>
#include <QLabel>
#include <QStackedWidget>
#include <QPushButton>
#include <QButtonGroup>
#include <QTimer>
#include <QMenu>
#include <QAction>
#include <QFileSystemModel>
#include <QSortFilterProxyModel>
#include <QStandardPaths>
#include <QFileInfo>
#include <QHeaderView>
#include <QIcon>
#include <QSettings>
#include <QWidget>
#include <QRegularExpression>
#include <QMessageBox>
#include <QInputDialog>
#include <QDir>
#include <QDesktopServices>
#include <QUrl>
#include <QProcess>
#include "../config/recentfilesmanager.h"
#include <QFile>
#include <QDateTime>
#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#endif

// #region agent log (optional)
static inline void hcwLog(const QString &hyp, const QString &msg, const QString &data = {})
{
    DebugLog::write(QStringLiteral("homecontentwidget.cpp"), hyp, msg, data);
}
// #endregion

HomeContentWidget::HomeContentWidget(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("HomeContentWidget"));
    loadHomePageSetting();
    setupUI();
    // Defer any I/O-heavy initialisation (file system model root path,
    // recent-file thumbnail generation) until after the event loop starts
    // and the window is already visible.  This keeps the constructor fast.
    QTimer::singleShot(0, this, &HomeContentWidget::initDeferred);
}

HomeContentWidget::~HomeContentWidget()
{
}

void HomeContentWidget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(8);

    // Header row: Recent/Browse toggle + title + view mode + search bar
    QWidget *headerWidget = new QWidget(this);
    headerWidget->setObjectName(QStringLiteral("HomeHeader"));

    QHBoxLayout *headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(0, 0, 0, 8);

    // Recent/Browse toggle buttons
    m_homePageGroup = new QButtonGroup(this);
    m_recentPageBtn = new QPushButton(tr("Recent"), this);
    m_recentPageBtn->setObjectName(QStringLiteral("HomePageToggle"));
    m_recentPageBtn->setCheckable(true);
    m_recentPageBtn->setChecked(m_currentHomePage == HomeStartPage::Recent);
    m_recentPageBtn->setToolTip(tr("Show recent files"));
    m_homePageGroup->addButton(m_recentPageBtn, static_cast<int>(HomeStartPage::Recent));

    m_browsePageBtn = new QPushButton(tr("Browse"), this);
    m_browsePageBtn->setObjectName(QStringLiteral("HomePageToggle"));
    m_browsePageBtn->setCheckable(true);
    m_browsePageBtn->setChecked(m_currentHomePage == HomeStartPage::Browse);
    m_browsePageBtn->setToolTip(tr("Browse file system"));
    m_homePageGroup->addButton(m_browsePageBtn, static_cast<int>(HomeStartPage::Browse));

    connect(m_homePageGroup, &QButtonGroup::idClicked,
            this, &HomeContentWidget::onHomePageToggleClicked);

    headerLayout->addWidget(m_recentPageBtn);
    headerLayout->addWidget(m_browsePageBtn);
    headerLayout->addSpacing(12);

    m_titleLabel = new QLabel(tr("Files"), this);
    m_titleLabel->setObjectName(QStringLiteral("HomeTitle"));
    headerLayout->addWidget(m_titleLabel);

    headerLayout->addStretch();

    // View mode buttons (only shown in Browse mode)
    m_viewModeGroup = new QButtonGroup(this);
    m_listModeBtn = new QPushButton(tr("List"), this);
    m_listModeBtn->setObjectName(QStringLiteral("ViewModeButton"));
    m_listModeBtn->setCheckable(true);
    m_listModeBtn->setChecked(true);
    m_listModeBtn->setToolTip(tr("List view"));
    m_viewModeGroup->addButton(m_listModeBtn, static_cast<int>(ViewMode::List));

    m_gridModeBtn = new QPushButton(tr("Grid"), this);
    m_gridModeBtn->setObjectName(QStringLiteral("ViewModeButton"));
    m_gridModeBtn->setCheckable(true);
    m_gridModeBtn->setToolTip(tr("Grid view"));
    m_viewModeGroup->addButton(m_gridModeBtn, static_cast<int>(ViewMode::Grid));

    m_compactModeBtn = new QPushButton(tr("Compact"), this);
    m_compactModeBtn->setObjectName(QStringLiteral("ViewModeButton"));
    m_compactModeBtn->setCheckable(true);
    m_compactModeBtn->setToolTip(tr("Compact view"));
    m_viewModeGroup->addButton(m_compactModeBtn, static_cast<int>(ViewMode::Compact));

    connect(m_viewModeGroup, &QButtonGroup::idClicked,
            this, &HomeContentWidget::onViewModeChanged);

    headerLayout->addWidget(m_listModeBtn);
    headerLayout->addWidget(m_gridModeBtn);
    headerLayout->addWidget(m_compactModeBtn);

    headerLayout->addSpacing(8);

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setObjectName(QStringLiteral("HomeSearchBar"));
    m_searchEdit->setPlaceholderText(tr("Search PDFs, folders…"));
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setMinimumWidth(200);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &HomeContentWidget::onSearchTextChanged);
    headerLayout->addWidget(m_searchEdit);

    // Debounce timer for search (200ms delay)
    m_searchDebounceTimer = new QTimer(this);
    m_searchDebounceTimer->setSingleShot(true);
    m_searchDebounceTimer->setInterval(200);
    connect(m_searchDebounceTimer, &QTimer::timeout, this, &HomeContentWidget::applySearchFilter);

    mainLayout->addWidget(headerWidget);

    // Main stack: Recent (index 0) vs Browse (index 1)
    m_mainStack = new QStackedWidget(this);
    m_mainStack->setObjectName(QStringLiteral("HomeMainStack"));

    // Recent Files widget (index 0)
    m_recentFilesWidget = new RecentFilesWidget(this);
    connect(m_recentFilesWidget, &RecentFilesWidget::fileClicked,
            this, &HomeContentWidget::openFileRequested);
    m_mainStack->addWidget(m_recentFilesWidget);

    // Browse view widget container (index 1)
    QWidget *browseContainer = new QWidget(this);
    QVBoxLayout *browseLayout = new QVBoxLayout(browseContainer);
    browseLayout->setContentsMargins(0, 0, 0, 0);

    // Browse stack: List/Grid/Compact modes
    m_browseStack = new QStackedWidget(this);
    m_browseStack->setObjectName(QStringLiteral("BrowseStack"));

    // Create QFileSystemModel but do NOT call setRootPath() yet — that call
    // starts QFileSystemWatcher enumeration and can block for seconds on
    // Windows (especially with network drives).  setRootPath is called in
    // initDeferred() after the event loop is running.
    m_fileSystemModel = new QFileSystemModel(this);
    m_fileSystemModel->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
    m_fileSystemModel->setNameFilterDisables(false);

    m_proxyModel = new QSortFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_fileSystemModel);
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    // Do NOT enable setRecursiveFilteringEnabled(true): it forces a full
    // filesystem traversal on every filter change, blocking the main thread.

    m_treeView = new QTreeView(this);
    m_treeView->setObjectName(QStringLiteral("HomeFileTree"));
    m_treeView->setModel(m_proxyModel);
    // Root index set in initDeferred after setRootPath().
    m_treeView->setHeaderHidden(false);
    m_treeView->header()->setStretchLastSection(true);
    m_treeView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_treeView->setAnimated(true);
    m_treeView->setIndentation(20);
    m_treeView->setSortingEnabled(true);
    m_treeView->sortByColumn(0, Qt::AscendingOrder);
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_treeView->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked);

    connect(m_treeView, &QTreeView::doubleClicked, this, &HomeContentWidget::onItemDoubleClicked);
    connect(m_treeView, &QTreeView::customContextMenuRequested, this, &HomeContentWidget::showBrowseContextMenu);

    m_fileSystemModel->setReadOnly(false);

    m_clipboard = FileClipboard::instance();
    connect(m_clipboard, &FileClipboard::clipboardChanged, this, [this]() {
        if (m_browseContextMenu) {
            // Updated when menu is shown
        }
    });

    setupBrowseContextMenu();

    m_listView = new QListView(this);
    m_listView->setModel(m_proxyModel);
    // Root index set in initDeferred.
    m_listView->setVisible(false);

    m_browseStack->addWidget(m_treeView);
    m_browseStack->addWidget(m_listView);

    browseLayout->addWidget(m_browseStack);
    m_mainStack->addWidget(browseContainer);

    mainLayout->addWidget(m_mainStack, 1);

    // Defer active-page switch and view mode update to initDeferred() so the
    // widget is added to the layout and the window can appear before any I/O.
    // Set the stack index directly (no I/O) so the visible page is correct
    // when the window first paints.
    if (m_mainStack) {
        m_mainStack->setCurrentIndex(static_cast<int>(m_currentHomePage));
    }

    // #region agent log — palette snapshot after widget tree is built
    QTimer::singleShot(200, this, [this]() {
        hcwLog("H-A", "HomeContentWidget palette",
            QString("{\"window\":\"%1\",\"base\":\"%2\",\"text\":\"%3\",\"autoFill\":%4}")
                .arg(palette().color(QPalette::Window).name(),
                     palette().color(QPalette::Base).name(),
                     palette().color(QPalette::Text).name(),
                     autoFillBackground() ? "true" : "false"));
        if (m_mainStack) hcwLog("H-C", "HomeMainStack palette",
            QString("{\"window\":\"%1\",\"base\":\"%2\",\"autoFill\":%3}")
                .arg(m_mainStack->palette().color(QPalette::Window).name(),
                     m_mainStack->palette().color(QPalette::Base).name(),
                     m_mainStack->autoFillBackground() ? "true" : "false"));
        if (m_recentFilesWidget) hcwLog("H-A", "RecentFilesWidget palette",
            QString("{\"window\":\"%1\",\"base\":\"%2\",\"autoFill\":%3}")
                .arg(m_recentFilesWidget->palette().color(QPalette::Window).name(),
                     m_recentFilesWidget->palette().color(QPalette::Base).name(),
                     m_recentFilesWidget->autoFillBackground() ? "true" : "false"));
        // Walk immediate children of this widget
        const auto kids = findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
        for (QWidget *k : kids) {
            hcwLog("H-A", "child-widget",
                QString("{\"class\":\"%1\",\"name\":\"%2\",\"window\":\"%3\",\"base\":\"%4\",\"autoFill\":%5,\"size\":\"%6x%7\"}")
                    .arg(k->metaObject()->className(), k->objectName(),
                         k->palette().color(QPalette::Window).name(),
                         k->palette().color(QPalette::Base).name(),
                         k->autoFillBackground() ? "true" : "false",
                         QString::number(k->width()), QString::number(k->height())));
        }
    });
    // #endregion
}

void HomeContentWidget::initDeferred()
{
    // Called via QTimer::singleShot(0) from the constructor, so the window is
    // already shown before we touch the file system.
    const QString homePath =
        QStandardPaths::writableLocation(QStandardPaths::HomeLocation);

    // setRootPath starts the QFileSystemWatcher background thread. Doing this
    // here (not in setupUI) means it happens after the first paint.
    m_fileSystemModel->setRootPath(homePath);

    QModelIndex sourceRoot = m_fileSystemModel->index(homePath);
    QModelIndex proxyRoot  = m_proxyModel->mapFromSource(sourceRoot);
    if (m_treeView) m_treeView->setRootIndex(proxyRoot);
    if (m_listView) m_listView->setRootIndex(proxyRoot);

    // Now trigger the full active-page setup (populates recent files, etc.)
    setActiveHomePage(m_currentHomePage);
    updateViewMode();
}

void HomeContentWidget::setRootPath(const QString &path)
{
    if (path.isEmpty()) {
        return;
    }
    // Switch to Browse view when navigating to a path
    setActiveHomePage(HomeStartPage::Browse);
    
    QModelIndex sourceIndex = m_fileSystemModel->index(path);
    if (sourceIndex.isValid()) {
        QModelIndex proxyIndex = m_proxyModel->mapFromSource(sourceIndex);
        m_treeView->setRootIndex(proxyIndex);
        if (m_listView) {
            m_listView->setRootIndex(proxyIndex);
        }
    }
}

void HomeContentWidget::setFilterText(const QString &text)
{
    if (m_searchEdit) {
        m_searchEdit->setText(text);
    }
    m_currentSearchText = text;
    applySearchFilter();
}

void HomeContentWidget::onItemDoubleClicked(const QModelIndex &index)
{
    QModelIndex sourceIndex = m_proxyModel->mapToSource(index);
    QString path = m_fileSystemModel->filePath(sourceIndex);

    if (m_fileSystemModel->isDir(sourceIndex)) {
        // Double-click on folder: navigate (handled by tree view expand)
        return;
    }

    if (isPdfFile(path)) {
        emit openFileRequested(path);
    }
}

void HomeContentWidget::setViewMode(ViewMode mode)
{
    m_currentViewMode = mode;
    updateViewMode();
}

void HomeContentWidget::onViewModeChanged(int id)
{
    m_currentViewMode = static_cast<ViewMode>(id);
    updateViewMode();
}

void HomeContentWidget::setActiveHomePage(HomeStartPage page)
{
    m_currentHomePage = page;
    
    if (m_mainStack) {
        m_mainStack->setCurrentIndex(static_cast<int>(page));
        
        // Update button states
        if (m_recentPageBtn) {
            m_recentPageBtn->setChecked(page == HomeStartPage::Recent);
        }
        if (m_browsePageBtn) {
            m_browsePageBtn->setChecked(page == HomeStartPage::Browse);
        }
        
        // Update title
        if (m_titleLabel) {
            m_titleLabel->setText(page == HomeStartPage::Recent ? tr("Recent Files") : tr("Files"));
        }
        
        // Show/hide view mode buttons and search bar based on page
        bool showBrowseControls = (page == HomeStartPage::Browse);
        if (m_listModeBtn) m_listModeBtn->setVisible(showBrowseControls);
        if (m_gridModeBtn) m_gridModeBtn->setVisible(showBrowseControls);
        if (m_compactModeBtn) m_compactModeBtn->setVisible(showBrowseControls);
        if (m_searchEdit) m_searchEdit->setVisible(showBrowseControls);
        
        // Re-apply current search filter when switching pages
        applySearchFilter();
    }
    
    saveHomePageSetting();
    emit homePageChanged(page);
}

void HomeContentWidget::onHomePageToggleClicked(int id)
{
    HomeStartPage page = static_cast<HomeStartPage>(id);
    setActiveHomePage(page);
    
    // Emit signal so MainWindow can update menu actions
    emit homePageChanged(page);
}

void HomeContentWidget::updateViewMode()
{
    // Only applies to Browse view
    if (m_currentHomePage != HomeStartPage::Browse || !m_browseStack) {
        return;
    }
    
    // For now, all modes use tree view
    // TODO: Phase 2C+ - Implement actual grid/list/compact views
    switch (m_currentViewMode) {
    case ViewMode::List:
    case ViewMode::Grid:
    case ViewMode::Compact:
        m_browseStack->setCurrentWidget(m_treeView);
        break;
    }
}

void HomeContentWidget::saveHomePageSetting()
{
    // Persist via SettingsManager so the key ("general/homeStartPage") stays
    // consistent with the settings dialog.  Fall back to raw QSettings if the
    // singleton is not yet created (e.g. very early startup).
    QString value = (m_currentHomePage == HomeStartPage::Recent)
                        ? QStringLiteral("recent")
                        : QStringLiteral("browse");
    SettingsManager::instance()->setValue(QStringLiteral("general/homeStartPage"), value);
}

void HomeContentWidget::loadHomePageSetting()
{
    // Read through SettingsManager so we always use the canonical key.
    QString value = SettingsManager::instance()
                        ->value(QStringLiteral("general/homeStartPage"), QStringLiteral("recent"))
                        .toString();
    m_currentHomePage = (value == QStringLiteral("browse"))
                            ? HomeStartPage::Browse
                            : HomeStartPage::Recent;
}

void HomeContentWidget::onSearchTextChanged(const QString &text)
{
    m_currentSearchText = text;
    // Restart debounce timer
    if (m_searchDebounceTimer) {
        m_searchDebounceTimer->stop();
        m_searchDebounceTimer->start();
    }
}

void HomeContentWidget::applySearchFilter()
{
    QString searchText = m_currentSearchText.trimmed();
    
    if (m_currentHomePage == HomeStartPage::Browse) {
        // Filter Browse view using QSortFilterProxyModel.
        // Skip when search is empty — clearing the regex on a QFileSystemModel
        // proxy with a large directory can still be slow; just leave it unset.
        if (m_proxyModel) {
            if (searchText.isEmpty()) {
                // No-op: leave proxy unfiltered rather than calling setFilter("")
                // which triggers a full tree scan on some Qt/Windows combinations.
                return;
            } else {
                // Convert simple wildcards to regex
                QString pattern = QRegularExpression::escape(searchText);
                pattern.replace("\\*", ".*");
                pattern.replace("\\?", ".");
                QRegularExpression regex(pattern, QRegularExpression::CaseInsensitiveOption);
                m_proxyModel->setFilterRegularExpression(regex);
                // Filter on filename column (column 0)
                m_proxyModel->setFilterKeyColumn(0);
            }
        }
    } else if (m_currentHomePage == HomeStartPage::Recent) {
        // Filter Recent view cards
        if (m_recentFilesWidget) {
            m_recentFilesWidget->setFilterText(searchText);
        }
    }
}

void HomeContentWidget::focusSearchBar()
{
    if (m_searchEdit) {
        m_searchEdit->setFocus();
        m_searchEdit->selectAll();
    }
}

QString HomeContentWidget::currentSearchText() const
{
    return m_searchEdit ? m_searchEdit->text() : QString();
}

void HomeContentWidget::selectAll()
{
    if (m_currentHomePage == HomeStartPage::Browse && m_treeView) {
        m_treeView->selectAll();
    }
}

void HomeContentWidget::setupBrowseContextMenu()
{
    m_browseContextMenu = new QMenu(this);

    QAction *openAct = m_browseContextMenu->addAction(tr("Open"), this, [this]() {
        if (m_contextMenuIndex.isValid()) {
            onItemDoubleClicked(m_contextMenuIndex);
        }
    });
    openAct->setShortcut(QKeySequence::Open);

    QAction *openNewTabAct = m_browseContextMenu->addAction(tr("Open in New Tab"), this, [this]() {
        if (m_contextMenuIndex.isValid()) {
            QModelIndex sourceIndex = m_proxyModel->mapToSource(m_contextMenuIndex);
            QString path = m_fileSystemModel->filePath(sourceIndex);
            if (isPdfFile(path)) {
                emit openFileRequested(path);
            }
        }
    });

    m_browseContextMenu->addSeparator();

    QAction *copyAct = m_browseContextMenu->addAction(tr("Copy"), this, &HomeContentWidget::onCopy);
    copyAct->setShortcut(QKeySequence::Copy);

    QAction *cutAct = m_browseContextMenu->addAction(tr("Cut"), this, &HomeContentWidget::onCut);
    cutAct->setShortcut(QKeySequence::Cut);

    QAction *pasteAct = m_browseContextMenu->addAction(tr("Paste"), this, &HomeContentWidget::onPaste);
    pasteAct->setShortcut(QKeySequence::Paste);

    m_browseContextMenu->addSeparator();

    QAction *renameAct = m_browseContextMenu->addAction(tr("Rename"), this, &HomeContentWidget::onRename);
    renameAct->setShortcut(QKeySequence(Qt::Key_F2));

    QAction *duplicateAct = m_browseContextMenu->addAction(tr("Duplicate"), this, &HomeContentWidget::onDuplicate);

    m_browseContextMenu->addSeparator();

    QAction *deleteAct = m_browseContextMenu->addAction(tr("Delete"), this, &HomeContentWidget::onDelete);
    deleteAct->setShortcut(QKeySequence::Delete);

    m_browseContextMenu->addSeparator();

    QAction *propertiesAct = m_browseContextMenu->addAction(tr("Properties"), this, &HomeContentWidget::onProperties);

    QAction *showInFolderAct = m_browseContextMenu->addAction(tr("Show in Folder"), this, &HomeContentWidget::onShowInFolder);
}

void HomeContentWidget::showBrowseContextMenu(const QPoint &pos)
{
    if (m_currentHomePage != HomeStartPage::Browse) {
        return;
    }

    QModelIndex index = m_treeView->indexAt(pos);
    m_contextMenuIndex = index;

    if (!m_browseContextMenu) {
        return;
    }

    // Update action states
    bool hasSelection = index.isValid();
    bool isFile = false;
    bool isDir = false;
    QString path;

    if (hasSelection) {
        QModelIndex sourceIndex = m_proxyModel->mapToSource(index);
        path = m_fileSystemModel->filePath(sourceIndex);
        QFileInfo info(path);
        isFile = info.isFile();
        isDir = info.isDir();
    }

    // Enable/disable actions based on selection
    QList<QAction*> actions = m_browseContextMenu->actions();
    for (QAction *action : actions) {
        QString text = action->text();
        if (text == tr("Open") || text == tr("Open in New Tab")) {
            action->setEnabled(hasSelection && isFile && isPdfFile(path));
        } else if (text == tr("Copy") || text == tr("Cut") || text == tr("Rename") || 
                   text == tr("Duplicate") || text == tr("Delete") || text == tr("Properties") ||
                   text == tr("Show in Folder")) {
            action->setEnabled(hasSelection);
        } else if (text == tr("Paste")) {
            action->setEnabled(m_clipboard && m_clipboard->hasData());
        }
    }

    m_browseContextMenu->exec(m_treeView->mapToGlobal(pos));
}

void HomeContentWidget::onCopy()
{
    if (m_currentHomePage != HomeStartPage::Browse || !m_treeView) {
        return;
    }
    
    QModelIndex currentIndex = m_treeView->currentIndex();
    if (!currentIndex.isValid()) {
        return;
    }
    
    QModelIndex sourceIndex = m_proxyModel->mapToSource(currentIndex);
    QString path = m_fileSystemModel->filePath(sourceIndex);
    if (m_clipboard) {
        m_clipboard->copy(path);
    }
}

void HomeContentWidget::onCut()
{
    if (m_currentHomePage != HomeStartPage::Browse || !m_treeView) {
        return;
    }
    
    QModelIndex currentIndex = m_treeView->currentIndex();
    if (!currentIndex.isValid()) {
        return;
    }
    
    QModelIndex sourceIndex = m_proxyModel->mapToSource(currentIndex);
    QString path = m_fileSystemModel->filePath(sourceIndex);
    if (m_clipboard) {
        m_clipboard->cut(path);
    }
}

void HomeContentWidget::onPaste()
{
    if (m_currentHomePage != HomeStartPage::Browse || !m_treeView) {
        return;
    }
    if (!m_clipboard || !m_clipboard->hasData()) {
        return;
    }

    const QString destFolder = getCurrentFolderPath();
    if (destFolder.isEmpty()) {
        return;
    }

    // Basic guardrails: refuse operations that target very high-risk roots.
    QFileInfo destInfo(destFolder);
    const QString nativeDest = QDir::toNativeSeparators(destInfo.absoluteFilePath()).toLower();
    if (nativeDest == QDir::rootPath().toLower()) {
        QMessageBox::warning(this, tr("Operation blocked"),
                             tr("Pasting directly into the filesystem root is not allowed."));
        return;
    }

    QList<QString> paths = m_clipboard->getPaths();
    const bool isCut = m_clipboard->isCutOperation();

    for (const QString &sourcePath : paths) {
        QFileInfo sourceInfo(sourcePath);
        if (!sourceInfo.exists()) {
            continue;
        }

        if (sourceInfo.isDir()) {
            // Explicitly disallow directory copy/move for now.
            QMessageBox::information(this, tr("Not Supported"),
                                     tr("Folder paste is not supported yet. "
                                        "Please move or copy folders using your system file manager."));
            continue;
        }

        const QString finalDestName = generateUniqueFileName(destFolder, sourceInfo.fileName());
        const QString finalDestPath = destFolder + QDir::separator() + finalDestName;

        if (isCut) {
            // Move operation
            QDir dir;
            if (!dir.rename(sourcePath, finalDestPath)) {
                // Cross-device move: copy + delete
                if (QFile::copy(sourcePath, finalDestPath)) {
                    if (!QFile::remove(sourcePath)) {
                        QMessageBox::warning(this, tr("Error"),
                                             tr("Failed to remove original file after moving %1.")
                                                 .arg(sourceInfo.fileName()));
                    }
                } else {
                    QMessageBox::warning(this, tr("Error"),
                                         tr("Failed to move %1").arg(sourceInfo.fileName()));
                }
            }
        } else {
            // Copy operation (files only)
            if (!QFile::copy(sourcePath, finalDestPath)) {
                QMessageBox::warning(this, tr("Error"),
                                     tr("Failed to copy %1").arg(sourceInfo.fileName()));
            }
        }
    }

    if (isCut) {
        m_clipboard->clear();
    }

    const QString currentRoot = getCurrentFolderPath();
    if (!currentRoot.isEmpty()) {
        setRootPath(currentRoot);
    }
}

void HomeContentWidget::onRename()
{
    if (m_currentHomePage != HomeStartPage::Browse || !m_treeView) {
        return;
    }
    
    QModelIndex currentIndex = m_treeView->currentIndex();
    if (!currentIndex.isValid()) {
        return;
    }
    
    // Trigger inline editing
    m_treeView->edit(currentIndex);
}

void HomeContentWidget::onDuplicate()
{
    if (m_currentHomePage != HomeStartPage::Browse || !m_treeView) {
        return;
    }
    
    QModelIndex currentIndex = m_treeView->currentIndex();
    if (!currentIndex.isValid()) {
        return;
    }
    
    QModelIndex sourceIndex = m_proxyModel->mapToSource(currentIndex);
    const QString sourcePath = m_fileSystemModel->filePath(sourceIndex);
    QFileInfo sourceInfo(sourcePath);

    if (sourceInfo.isDir()) {
        QMessageBox::information(this, tr("Not Supported"),
                                 tr("Folder duplication is not supported. "
                                    "Please use your system file manager to duplicate folders."));
        return;
    }

    const QString destFolder = sourceInfo.absolutePath();
    const QString newFileName = generateUniqueFileName(destFolder, sourceInfo.fileName());
    const QString destPath = destFolder + QDir::separator() + newFileName;

    if (!QFile::copy(sourcePath, destPath)) {
        QMessageBox::warning(this, tr("Error"),
                             tr("Failed to duplicate %1").arg(sourceInfo.fileName()));
    } else {
        const QString currentRoot = getCurrentFolderPath();
        if (!currentRoot.isEmpty()) {
            setRootPath(currentRoot);
        }
    }
}

void HomeContentWidget::onDelete()
{
    if (m_currentHomePage != HomeStartPage::Browse || !m_treeView) {
        return;
    }
    
    QModelIndex currentIndex = m_treeView->currentIndex();
    if (!currentIndex.isValid()) {
        return;
    }
    
    QModelIndex sourceIndex = m_proxyModel->mapToSource(currentIndex);
    QString path = m_fileSystemModel->filePath(sourceIndex);
    QFileInfo info(path);

    QString itemName = info.fileName();
    QString message = info.isDir() 
        ? tr("Are you sure you want to delete the folder '%1' and all its contents?")
        : tr("Are you sure you want to delete '%1'?");
    
    int ret = QMessageBox::question(this, tr("Confirm Delete"), 
        message.arg(itemName),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (ret != QMessageBox::Yes) {
        return;
    }

    bool success = false;
    if (info.isDir()) {
        QDir dir(path);
        success = dir.removeRecursively();
    } else {
        success = QFile::remove(path);
    }

    if (!success) {
        // Try move to trash
        if (moveToTrash(path)) {
            success = true;
        } else {
            QMessageBox::warning(this, tr("Error"), 
                tr("Failed to delete %1").arg(itemName));
        }
    }

    if (success) {
        // Remove from recent files if it was a PDF
        if (isPdfFile(path)) {
            RecentFilesManager::instance()->removeFile(path);
        }
        // Refresh by resetting root
        QString currentRoot = getCurrentFolderPath();
        if (!currentRoot.isEmpty()) {
            setRootPath(currentRoot);
        }
    }
}

void HomeContentWidget::onProperties()
{
    if (m_currentHomePage != HomeStartPage::Browse || !m_treeView) {
        return;
    }
    
    QModelIndex currentIndex = m_treeView->currentIndex();
    if (!currentIndex.isValid()) {
        return;
    }
    
    QModelIndex sourceIndex = m_proxyModel->mapToSource(currentIndex);
    QString path = m_fileSystemModel->filePath(sourceIndex);

    FilePropertiesDialog dialog(path, this);
    dialog.exec();
}

void HomeContentWidget::onShowInFolder()
{
    if (m_currentHomePage != HomeStartPage::Browse || !m_treeView) {
        return;
    }
    
    QModelIndex currentIndex = m_treeView->currentIndex();
    if (!currentIndex.isValid()) {
        return;
    }
    
    QModelIndex sourceIndex = m_proxyModel->mapToSource(currentIndex);
    QString path = m_fileSystemModel->filePath(sourceIndex);
    showInFolder(path);
}

void HomeContentWidget::showInFolder(const QString &path)
{
    QFileInfo info(path);

#ifdef Q_OS_WIN
    // Windows: Use explorer /select to select the file
    const QString nativePath = QDir::toNativeSeparators(info.absoluteFilePath());
    QStringList args;
    args << QStringLiteral("/select,") + nativePath;
    QProcess::startDetached(QStringLiteral("explorer.exe"), args);
#else
    // Linux/Mac: Open parent folder
    QString folderPath = info.isDir() ? path : info.absolutePath();
    QDesktopServices::openUrl(QUrl::fromLocalFile(folderPath));
#endif
}

QString HomeContentWidget::getCurrentFolderPath() const
{
    if (!m_treeView) {
        return QString();
    }
    QModelIndex rootIndex = m_treeView->rootIndex();
    QModelIndex sourceRootIndex = m_proxyModel->mapToSource(rootIndex);
    return m_fileSystemModel->filePath(sourceRootIndex);
}

QString HomeContentWidget::generateUniqueFileName(const QString &basePath, const QString &fileName) const
{
    QFileInfo info(basePath + QDir::separator() + fileName);
    QString baseName = info.baseName();
    QString suffix = info.suffix();
    QString dir = info.absolutePath();

    QString newName = fileName;
    int counter = 1;

    while (QFileInfo::exists(dir + QDir::separator() + newName)) {
        if (suffix.isEmpty()) {
            newName = QStringLiteral("%1 (%2)").arg(baseName).arg(counter);
        } else {
            newName = QStringLiteral("%1 (%2).%3").arg(baseName).arg(counter).arg(suffix);
        }
        counter++;
    }

    return newName;
}

bool HomeContentWidget::moveToTrash(const QString &path) const
{
#ifdef Q_OS_WIN
    // Windows: Use SHFileOperation or IFileOperation
    QFileInfo info(path);
    QString nativePath = QDir::toNativeSeparators(info.absoluteFilePath());
    
    wchar_t *wPath = new wchar_t[nativePath.length() + 2];
    nativePath.toWCharArray(wPath);
    wPath[nativePath.length()] = L'\0';
    wPath[nativePath.length() + 1] = L'\0'; // Double null-terminated

    SHFILEOPSTRUCTW fileOp;
    ZeroMemory(&fileOp, sizeof(fileOp));
    fileOp.hwnd = nullptr;
    fileOp.wFunc = FO_DELETE;
    fileOp.pFrom = wPath;
    fileOp.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;

    int result = SHFileOperationW(&fileOp);
    delete[] wPath;

    return (result == 0 && !fileOp.fAnyOperationsAborted);
#else
    // Non-Windows: attempt to send to the desktop trash using common helpers.
    QFileInfo info(path);
    const QString folderPath = info.absolutePath();

    // Try gio-based trash first (GNOME / many Linux desktops).
    QProcess proc;
    proc.start(QStringLiteral("gio"), QStringList() << QStringLiteral("trash") << info.absoluteFilePath());
    if (proc.waitForFinished(1500) && proc.exitStatus() == QProcess::NormalExit && proc.exitCode() == 0) {
        return true;
    }

    // Fallback: try xdg-trash (less common).
    QProcess proc2;
    proc2.start(QStringLiteral("xdg-trash"), QStringList() << info.absoluteFilePath());
    if (proc2.waitForFinished(1500) && proc2.exitStatus() == QProcess::NormalExit && proc2.exitCode() == 0) {
        return true;
    }

    Q_UNUSED(folderPath);
    return false;
#endif
}

bool HomeContentWidget::isPdfFile(const QString &path) const
{
    return QFileInfo(path).suffix().toLower() == QStringLiteral("pdf");
}
