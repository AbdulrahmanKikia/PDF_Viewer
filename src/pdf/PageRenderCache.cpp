// PageRenderCache.cpp
#include "PageRenderCache.h"

static qint64 imageBytes(const QImage& img)
{
    return img.isNull() ? 0 : img.sizeInBytes();
}

PageRenderCache::PageRenderCache(qint64 maxBytes)
    : m_maxBytes(maxBytes)
{}

QImage PageRenderCache::get(const RenderKey& key)
{
    QMutexLocker lk(&m_mutex);
    auto it = m_index.find(key);
    if (it == m_index.end())
        return {};
    // Move to front (most-recently-used).
    m_lruList.splice(m_lruList.begin(), m_lruList, it.value());
    return it.value()->image;
}

void PageRenderCache::put(const RenderKey& key, const QImage& image)
{
    if (image.isNull()) return;

    QMutexLocker lk(&m_mutex);
    auto it = m_index.find(key);
    if (it != m_index.end()) {
        // Update existing entry.
        m_usedBytes -= it.value()->sizeBytes;
        it.value()->image     = image;
        it.value()->sizeBytes = imageBytes(image);
        m_usedBytes += it.value()->sizeBytes;
        m_lruList.splice(m_lruList.begin(), m_lruList, it.value());
    } else {
        Entry e;
        e.key       = key;
        e.image     = image;
        e.sizeBytes = imageBytes(image);
        m_lruList.push_front(std::move(e));
        m_index.insert(key, m_lruList.begin());
        m_usedBytes += m_lruList.front().sizeBytes;
    }
    evictToFit();
}

void PageRenderCache::evictFile(const QString& filePath)
{
    QMutexLocker lk(&m_mutex);
    for (auto it = m_lruList.begin(); it != m_lruList.end(); ) {
        if (it->key.filePath == filePath) {
            m_usedBytes -= it->sizeBytes;
            m_index.remove(it->key);
            it = m_lruList.erase(it);
        } else {
            ++it;
        }
    }
}

void PageRenderCache::clear()
{
    QMutexLocker lk(&m_mutex);
    m_lruList.clear();
    m_index.clear();
    m_usedBytes = 0;
}

void PageRenderCache::setMaxBytes(qint64 maxBytes)
{
    QMutexLocker lk(&m_mutex);
    m_maxBytes = maxBytes;
    evictToFit();
}

qint64 PageRenderCache::usedBytes() const
{
    QMutexLocker lk(&m_mutex);
    return m_usedBytes;
}

int PageRenderCache::count() const
{
    QMutexLocker lk(&m_mutex);
    return static_cast<int>(m_lruList.size());
}

void PageRenderCache::evictToFit()
{
    // Called with m_mutex held.
    while (m_usedBytes > m_maxBytes && !m_lruList.empty()) {
        Entry& lru = m_lruList.back();
        m_usedBytes -= lru.sizeBytes;
        m_index.remove(lru.key);
        m_lruList.pop_back();
    }
}
