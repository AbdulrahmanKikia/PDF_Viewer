#include "AnnotationStore.h"

#include <QtMath>

void AnnotationStore::clear()
{
    m_byPage.clear();
}

void AnnotationStore::addAnnotation(const Annotation &ann)
{
    m_byPage[ann.pageIndex].append(ann);
}

bool AnnotationStore::removeNearest(int pageIndex, const QPointF &pagePoint, qreal maxDistance)
{
    auto it = m_byPage.find(pageIndex);
    if (it == m_byPage.end() || it->isEmpty())
        return false;

    AnnotationList &list = *it;
    int bestIdx = -1;
    qreal bestDist2 = maxDistance * maxDistance;

    for (int i = 0; i < list.size(); ++i) {
        const Annotation &ann = list.at(i);
        QPointF center;
        if (!ann.rect.isNull()) {
            center = ann.rect.center();
        } else if (!ann.path.isEmpty()) {
            center = ann.path.constFirst();
        } else {
            continue;
        }
        const qreal dx = center.x() - pagePoint.x();
        const qreal dy = center.y() - pagePoint.y();
        const qreal dist2 = dx * dx + dy * dy;
        if (dist2 <= bestDist2) {
            bestDist2 = dist2;
            bestIdx = i;
        }
    }

    if (bestIdx >= 0) {
        list.removeAt(bestIdx);
        if (list.isEmpty())
            m_byPage.erase(it);
        return true;
    }
    return false;
}

AnnotationList AnnotationStore::annotationsForPage(int pageIndex) const
{
    return m_byPage.value(pageIndex);
}

QJsonArray AnnotationStore::toJson() const
{
    QJsonArray array;
    for (auto it = m_byPage.constBegin(); it != m_byPage.constEnd(); ++it) {
        const int page = it.key();
        const AnnotationList &list = it.value();
        for (const Annotation &ann : list) {
            Annotation copy = ann;
            copy.pageIndex = page;
            array.append(annotationToJson(copy));
        }
    }
    return array;
}

void AnnotationStore::fromJson(const QJsonArray &array)
{
    m_byPage.clear();
    const AnnotationList list = annotationListFromJson(array);
    for (const Annotation &ann : list)
        m_byPage[ann.pageIndex].append(ann);
}

PageOrder makeIdentityPageOrder(int pageCount)
{
    PageOrder order;
    order.reserve(pageCount);
    for (int i = 0; i < pageCount; ++i)
        order.append(i);
    return order;
}

void deletePageFromOrder(PageOrder &order, int logicalIndex)
{
    if (logicalIndex < 0 || logicalIndex >= order.size())
        return;
    order.removeAt(logicalIndex);
}

void duplicatePageInOrder(PageOrder &order, int logicalIndex)
{
    if (logicalIndex < 0 || logicalIndex >= order.size())
        return;
    order.insert(logicalIndex + 1, order.at(logicalIndex));
}

void movePageLeftInOrder(PageOrder &order, int logicalIndex)
{
    if (logicalIndex <= 0 || logicalIndex >= order.size())
        return;
    order.move(logicalIndex, logicalIndex - 1);
}

void movePageRightInOrder(PageOrder &order, int logicalIndex)
{
    if (logicalIndex < 0 || logicalIndex >= order.size() - 1)
        return;
    order.move(logicalIndex, logicalIndex + 1);
}

