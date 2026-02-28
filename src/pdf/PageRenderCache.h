// PageRenderCache.h
// Thread-safe LRU cache for rendered PDF page images.
//
// Key:    RenderKey  (filePath, pageIndex, zoom, targetSize, rotation, quality)
// Value:  QImage
// Policy: Least-Recently-Used eviction when total byte usage exceeds capacity.
//
// The cache is safe to insert/lookup from multiple threads simultaneously.
// Default capacity: 200 MB; changeable at runtime via setMaxBytes().

#ifndef PAGERENDERCACHE_H
#define PAGERENDERCACHE_H

#include "IPdfRenderer.h"

#include <QHash>
#include <QImage>
#include <QMutex>

#include <list>

class PageRenderCache
{
public:
    explicit PageRenderCache(qint64 maxBytes = 200LL * 1024 * 1024);

    // Look up a cached image.  Returns a null QImage on a cache miss.
    QImage get(const RenderKey& key);

    // Insert (or update) a cached image; evicts LRU entries as needed.
    void put(const RenderKey& key, const QImage& image);

    // Evict all entries whose filePath matches (call when a document is closed).
    void evictFile(const QString& filePath);

    // Evict all entries.
    void clear();

    // Change capacity at runtime (e.g. from Settings dialog).
    void setMaxBytes(qint64 maxBytes);

    // Current approximate memory usage in bytes.
    qint64 usedBytes() const;

    // Number of entries currently cached.
    int count() const;

private:
    struct Entry {
        RenderKey key;
        QImage    image;
        qint64    sizeBytes = 0;
    };

    using List     = std::list<Entry>;
    using ListIter = List::iterator;

    // Must be called with m_mutex held.
    void evictToFit();

    mutable QMutex               m_mutex;
    List                         m_lruList;   // front = most recently used
    QHash<RenderKey, ListIter>   m_index;
    qint64 m_maxBytes  = 200LL * 1024 * 1024;
    qint64 m_usedBytes = 0;
};

#endif // PAGERENDERCACHE_H
