#include "thumbnailcache.h"
#include <QTimer>
#include <QFileInfo>
#include <QDir>
#include <QPainter>
#include <QFont>
#include <QFontMetrics>
#include <QStandardPaths>
#include <QCryptographicHash>
#include <QDebug>
#include <QColor>

ThumbnailCache::ThumbnailCache(QObject *parent)
    : QObject(parent)
{
    // Default cache directory: AppDataLocation/thumbnails
    QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    m_cacheDir = baseDir + QStringLiteral("/thumbnails");
    QDir dir;
    dir.mkpath(m_cacheDir);
}

ThumbnailCache::~ThumbnailCache()
{
}

QPixmap ThumbnailCache::getThumbnail(const QString &filePath, const QSize &size)
{
    if (filePath.isEmpty() || !QFileInfo::exists(filePath)) {
        return generatePlaceholderThumbnail(filePath, size);
    }

    QString cachePath = getCacheFilePath(filePath, size);
    QPixmap pixmap;

    // Try loading from disk cache
    if (loadFromDisk(cachePath, pixmap)) {
        return pixmap;
    }

    // TODO: Phase 6 - Real PDF thumbnail generation (MuPDF/PDFium)
    // For now, generate placeholder
    pixmap = generatePlaceholderThumbnail(filePath, size);

    // Save placeholder to disk cache
    saveToDisk(cachePath, pixmap);

    return pixmap;
}

void ThumbnailCache::requestThumbnail(const QString &filePath, const QSize &size)
{
    // Defer thumbnail generation so callers are never blocked in their
    // constructors.  The queued call runs on the next event-loop iteration,
    // after the widget tree is built and the window is visible.
    QTimer::singleShot(0, this, [this, filePath, size]() {
        QPixmap thumbnail = getThumbnail(filePath, size);
        emit thumbnailReady(filePath, thumbnail);
    });
}

void ThumbnailCache::setCacheDirectory(const QString &path)
{
    m_cacheDir = path;
    QDir dir;
    dir.mkpath(m_cacheDir);
}

QString ThumbnailCache::cacheDirectory() const
{
    return m_cacheDir;
}

void ThumbnailCache::clearCache()
{
    QDir dir(m_cacheDir);
    dir.removeRecursively();
    dir.mkpath(m_cacheDir);
}

qint64 ThumbnailCache::cacheSize() const
{
    qint64 totalSize = 0;
    QDir dir(m_cacheDir);
    QFileInfoList files = dir.entryInfoList(QDir::Files);
    for (const QFileInfo &info : files) {
        totalSize += info.size();
    }
    return totalSize;
}

QString ThumbnailCache::getCacheFilePath(const QString &filePath, const QSize &size) const
{
    // Generate cache key from file path and size
    QString key = QStringLiteral("%1_%2x%3")
                  .arg(filePath)
                  .arg(size.width())
                  .arg(size.height());
    QByteArray hash = QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Md5);
    QString hashStr = hash.toHex();
    return m_cacheDir + QStringLiteral("/") + hashStr + QStringLiteral(".png");
}

QPixmap ThumbnailCache::generatePlaceholderThumbnail(const QString &filePath, const QSize &size) const
{
    QPixmap pixmap(size);
    pixmap.fill(QColor(240, 240, 240)); // Light gray background

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw PDF icon placeholder (simple rectangle with "PDF" text)
    QRect iconRect(10, 10, size.width() - 20, size.height() - 40);
    painter.setPen(QPen(QColor(200, 50, 50), 2));
    painter.setBrush(QColor(220, 220, 220));
    painter.drawRoundedRect(iconRect, 4, 4);

    // Draw "PDF" text
    painter.setPen(QColor(100, 100, 100));
    QFont font(QStringLiteral("Arial"), 16, QFont::Bold);
    painter.setFont(font);
    painter.drawText(iconRect, Qt::AlignCenter, QStringLiteral("PDF"));

    // Draw filename at bottom
    QFileInfo info(filePath);
    QString fileName = info.fileName();
    if (fileName.length() > 20) {
        fileName = fileName.left(17) + QStringLiteral("...");
    }

    QRect textRect(10, size.height() - 30, size.width() - 20, 20);
    painter.setPen(QColor(50, 50, 50));
    QFont nameFont(QStringLiteral("Arial"), 9);
    painter.setFont(nameFont);
    painter.drawText(textRect, Qt::AlignCenter | Qt::TextElideMode::ElideMiddle, fileName);

    return pixmap;
}

bool ThumbnailCache::loadFromDisk(const QString &cachePath, QPixmap &pixmap) const
{
    if (QFileInfo::exists(cachePath)) {
        pixmap = QPixmap(cachePath);
        return !pixmap.isNull();
    }
    return false;
}

void ThumbnailCache::saveToDisk(const QString &cachePath, const QPixmap &pixmap) const
{
    if (!pixmap.isNull()) {
        pixmap.save(cachePath, "PNG");
    }
}
