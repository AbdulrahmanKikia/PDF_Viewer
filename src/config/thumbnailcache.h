#ifndef THUMBNAILCACHE_H
#define THUMBNAILCACHE_H

#include <QObject>
#include <QString>
#include <QPixmap>
#include <QSize>
#include <QStandardPaths>
#include <QDir>

class ThumbnailCache : public QObject
{
    Q_OBJECT

public:
    explicit ThumbnailCache(QObject *parent = nullptr);
    ~ThumbnailCache();

    // Get thumbnail (synchronous - returns placeholder for now)
    QPixmap getThumbnail(const QString &filePath, const QSize &size = QSize(200, 200));

    // Async thumbnail generation (stub - returns placeholder)
    void requestThumbnail(const QString &filePath, const QSize &size = QSize(200, 200));

    // Cache management
    void setCacheDirectory(const QString &path);
    QString cacheDirectory() const;
    void clearCache();
    qint64 cacheSize() const;

signals:
    void thumbnailReady(const QString &filePath, const QPixmap &thumbnail);

private:
    QString getCacheFilePath(const QString &filePath, const QSize &size) const;
    QPixmap generatePlaceholderThumbnail(const QString &filePath, const QSize &size) const;
    bool loadFromDisk(const QString &cachePath, QPixmap &pixmap) const;
    void saveToDisk(const QString &cachePath, const QPixmap &pixmap) const;

    QString m_cacheDir;
    // TODO: Add LRU memory cache in next phase
    // QHash<QString, QPixmap> m_memoryCache;
    // QList<QString> m_lruOrder;
    // static constexpr int MAX_MEMORY_CACHE_SIZE = 100;
};

#endif // THUMBNAILCACHE_H
