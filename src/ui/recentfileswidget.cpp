#include "recentfileswidget.h"
#include "../config/recentfilesmanager.h"
#include "../config/thumbnailcache.h"
#include "../config/debuglog.h"
#include <QGridLayout>
#include <QScrollArea>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QDateTime>
#include <QFileInfo>
#include <QPixmap>
#include <QFont>
#include <QFontMetrics>
#include <QMenu>
#include <QAction>
#include <QContextMenuEvent>
#include "../config/recentfilesmanager.h"
#include <QDesktopServices>
#include <QUrl>
#include <QProcess>
#include <QFile>
#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#endif

// #region agent log (optional)
static bool s_rfcPainted = false;
static inline void rfcLog(const QString &hyp, const QString &msg, const QString &data = {})
{
    DebugLog::write(QStringLiteral("recentfileswidget.cpp"), hyp, msg, data);
}
// #endregion

// RecentFileCard implementation
RecentFileCard::RecentFileCard(const RecentFileEntry &entry, ThumbnailCache *cache, QWidget *parent)
    : QWidget(parent)
    , m_filePath(entry.filePath)
    , m_displayName(entry.displayName)
    , m_lastOpened(entry.lastOpened)
    , m_cache(cache)
    , m_thumbnailLoaded(false)
{
    setObjectName(QStringLiteral("RecentFileCard"));
    setFixedSize(180, 220);
    setCursor(Qt::PointingHandCursor);
    setContextMenuPolicy(Qt::CustomContextMenu);

    // Request thumbnail asynchronously
    if (m_cache) {
        connect(m_cache, &ThumbnailCache::thumbnailReady,
                this, &RecentFileCard::onThumbnailReady);
        m_cache->requestThumbnail(m_filePath, QSize(160, 160));
    }

    setupContextMenu();
}

RecentFileCard::~RecentFileCard()
{
}

void RecentFileCard::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit clicked(m_filePath);
    }
    QWidget::mousePressEvent(event);
}

void RecentFileCard::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    // #region agent log
    if (!s_rfcPainted) {
        s_rfcPainted = true;
        const QPalette &p = palette();
        rfcLog("H-D", "RecentFileCard first paint",
            QString("{\"base\":\"%1\",\"window\":\"%2\",\"text\":\"%3\",\"autoFill\":%4}")
                .arg(p.color(QPalette::Base).name(),
                     p.color(QPalette::Window).name(),
                     p.color(QPalette::Text).name(),
                     autoFillBackground() ? "true" : "false"));
    }
    // #endregion

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const QPalette &pal = palette();
    const QColor cardBg     = pal.color(QPalette::Base);
    const QColor cardBorder = pal.color(QPalette::Mid);
    const QColor thumbBg    = pal.color(QPalette::AlternateBase);
    const QColor thumbBorder= pal.color(QPalette::Midlight);
    const QColor thumbText  = pal.color(QPalette::PlaceholderText);
    const QColor nameText   = pal.color(QPalette::Text);
    const QColor dateText   = pal.color(QPalette::PlaceholderText);

    // Card background
    QRect cardRect = rect().adjusted(4, 4, -4, -4);
    painter.setPen(QPen(cardBorder, 1));
    painter.setBrush(cardBg);
    painter.drawRoundedRect(cardRect, 6, 6);

    // Thumbnail area
    QRect thumbnailRect(12, 12, 156, 156);
    if (!m_thumbnail.isNull()) {
        painter.drawPixmap(thumbnailRect, m_thumbnail.scaled(156, 156, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        painter.setPen(QPen(thumbBorder, 1));
        painter.setBrush(thumbBg);
        painter.drawRoundedRect(thumbnailRect, 4, 4);
        painter.setPen(thumbText);
        painter.drawText(thumbnailRect, Qt::AlignCenter, tr("Loading..."));
    }

    // Filename
    QRect nameRect(12, 176, 156, 20);
    painter.setPen(nameText);
    QFont nameFont(QStringLiteral("Arial"), 10);
    painter.setFont(nameFont);
    QString elidedName = painter.fontMetrics().elidedText(m_displayName, Qt::ElideMiddle, nameRect.width());
    painter.drawText(nameRect, Qt::AlignLeft | Qt::AlignVCenter, elidedName);

    // Last opened timestamp
    QRect dateRect(12, 196, 156, 16);
    painter.setPen(dateText);
    QFont dateFont(QStringLiteral("Arial"), 8);
    painter.setFont(dateFont);
    QString dateStr = m_lastOpened.toString(QStringLiteral("MMM d, yyyy"));
    painter.drawText(dateRect, Qt::AlignLeft | Qt::AlignVCenter, dateStr);
}

void RecentFileCard::setupContextMenu()
{
    m_contextMenu = new QMenu(this);

    QAction *openAct = m_contextMenu->addAction(tr("Open"), this, [this]() {
        emit clicked(m_filePath);
    });

    m_contextMenu->addSeparator();

    QAction *showInFolderAct = m_contextMenu->addAction(tr("Show in Folder"), this, [this]() {
        emit showInFolderRequested(m_filePath);
    });

    QAction *removeAct = m_contextMenu->addAction(tr("Remove from Recent"), this, [this]() {
        emit removeFromRecentRequested(m_filePath);
    });
}

void RecentFileCard::contextMenuEvent(QContextMenuEvent *event)
{
    if (m_contextMenu) {
        m_contextMenu->exec(event->globalPos());
    }
}

void RecentFileCard::onThumbnailReady(const QString &filePath, const QPixmap &thumbnail)
{
    if (filePath == m_filePath) {
        m_thumbnail = thumbnail;
        m_thumbnailLoaded = true;
        update();
    }
}

// RecentFilesWidget implementation
RecentFilesWidget::RecentFilesWidget(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("RecentFilesWidget"));
    m_thumbnailCache = new ThumbnailCache(this);
    setupUI();
    // Do NOT call refreshRecentFiles() here — it reads QSettings (registry) via
    // RecentFilesManager and creates thumbnail cards synchronously, which blocks
    // the main thread before the window is shown.
    // HomeContentWidget::initDeferred() triggers the first refresh after show().

    // Connect to RecentFilesManager signals
    connect(RecentFilesManager::instance(), &RecentFilesManager::recentFilesChanged,
            this, &RecentFilesWidget::onRecentFilesChanged);
}

RecentFilesWidget::~RecentFilesWidget()
{
}

void RecentFilesWidget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(8);

    // Title
    m_titleLabel = new QLabel(tr("Recent Files"), this);
    m_titleLabel->setObjectName(QStringLiteral("RecentFilesTitle"));
    QFont titleFont(QStringLiteral("Arial"), 14, QFont::Bold);
    m_titleLabel->setFont(titleFont);
    mainLayout->addWidget(m_titleLabel);

    // Scroll area with grid layout
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setObjectName(QStringLiteral("RecentFilesScrollArea"));
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    // Ensure the viewport fills with QPalette::Base so it matches the dark theme.
    m_scrollArea->viewport()->setAutoFillBackground(true);

    m_contentWidget = new QWidget();
    m_contentWidget->setObjectName(QStringLiteral("RecentFilesContent"));
    m_gridLayout = new QGridLayout(m_contentWidget);
    m_gridLayout->setContentsMargins(0, 0, 0, 0);
    m_gridLayout->setSpacing(12);
    m_gridLayout->setAlignment(Qt::AlignTop);

    // "No results" placeholder
    m_noResultsLabel = new QLabel(tr("No results found"), m_contentWidget);
    m_noResultsLabel->setAlignment(Qt::AlignCenter);
    m_noResultsLabel->setStyleSheet(QStringLiteral("color: gray; padding: 40px; font-size: 14px;"));
    m_noResultsLabel->setVisible(false);

    m_scrollArea->setWidget(m_contentWidget);
    mainLayout->addWidget(m_scrollArea, 1);
}

void RecentFilesWidget::setFilterText(const QString &text)
{
    m_filterText = text.trimmed();
    refreshRecentFiles();
}

void RecentFilesWidget::refreshRecentFiles()
{
    // Clear existing cards
    QLayoutItem *item;
    while ((item = m_gridLayout->takeAt(0)) != nullptr) {
        if (item->widget() != m_noResultsLabel) {
            delete item->widget();
        }
        delete item;
    }

    // Get recent files
    QList<RecentFileEntry> recentFiles = RecentFilesManager::instance()->getRecentFiles(20);

    // Apply filter if present
    if (!m_filterText.isEmpty()) {
        QList<RecentFileEntry> filtered;
        for (const RecentFileEntry &entry : recentFiles) {
            if (matchesFilter(entry, m_filterText)) {
                filtered.append(entry);
            }
        }
        recentFiles = filtered;
    }

    // Show "No results" if filtered and empty
    if (!m_filterText.isEmpty() && recentFiles.isEmpty()) {
        m_noResultsLabel->setVisible(true);
        m_gridLayout->addWidget(m_noResultsLabel, 0, 0, 1, CARDS_PER_ROW);
        return;
    }
    
    m_noResultsLabel->setVisible(false);

    if (recentFiles.isEmpty() && m_filterText.isEmpty()) {
        QLabel *emptyLabel = new QLabel(tr("No recent files"), m_contentWidget);
        emptyLabel->setAlignment(Qt::AlignCenter);
        emptyLabel->setStyleSheet(QStringLiteral("color: gray; padding: 40px;"));
        m_gridLayout->addWidget(emptyLabel, 0, 0, 1, CARDS_PER_ROW);
        return;
    }

    // Create cards
    int row = 0;
    int col = 0;
    for (const RecentFileEntry &entry : recentFiles) {
        RecentFileCard *card = new RecentFileCard(entry, m_thumbnailCache, m_contentWidget);
        connect(card, &RecentFileCard::clicked, this, &RecentFilesWidget::fileClicked);
        connect(card, &RecentFileCard::showInFolderRequested, this, &RecentFilesWidget::showInFolderRequested);
        connect(card, &RecentFileCard::removeFromRecentRequested, this, &RecentFilesWidget::removeFromRecentRequested);
        m_gridLayout->addWidget(card, row, col);

        col++;
        if (col >= CARDS_PER_ROW) {
            col = 0;
            row++;
        }
    }
}

bool RecentFilesWidget::matchesFilter(const RecentFileEntry &entry, const QString &filterText) const
{
    if (filterText.isEmpty()) {
        return true;
    }
    
    // Case-insensitive substring match on filename
    return entry.displayName.contains(filterText, Qt::CaseInsensitive);
}

void RecentFilesWidget::onRecentFilesChanged()
{
    refreshRecentFiles();
}
