#ifndef PDFVIEWER_ANNOTATIONSTORE_H
#define PDFVIEWER_ANNOTATIONSTORE_H

#include "Annotation.h"

#include <QHash>
#include <QJsonArray>
#include <QVector>

// In-memory store: annotations keyed by logical page index.

class AnnotationStore
{
public:
    AnnotationStore() = default;

    void clear();

    void addAnnotation(const Annotation &ann);

    // Removes the nearest annotation on pageIndex to pagePoint (page coords),
    // within maxDistance (also in page coords). Returns true if something was removed.
    bool removeNearest(int pageIndex, const QPointF &pagePoint, qreal maxDistance);

    AnnotationList annotationsForPage(int pageIndex) const;

    QJsonArray toJson() const;
    void fromJson(const QJsonArray &array);

private:
    QHash<int, AnnotationList> m_byPage;
};

// Simple alias for per-document page ordering: each entry is an original page index.
using PageOrder = QVector<int>;

PageOrder makeIdentityPageOrder(int pageCount);
void deletePageFromOrder(PageOrder &order, int logicalIndex);
void duplicatePageInOrder(PageOrder &order, int logicalIndex);
void movePageLeftInOrder(PageOrder &order, int logicalIndex);
void movePageRightInOrder(PageOrder &order, int logicalIndex);

#endif // PDFVIEWER_ANNOTATIONSTORE_H

