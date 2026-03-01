#ifndef PDFVIEWER_ANNOTATION_H
#define PDFVIEWER_ANNOTATION_H

#include <QColor>
#include <QJsonArray>
#include <QJsonObject>
#include <QPointF>
#include <QRectF>
#include <QVector>

// Lightweight enums and structs for in-memory annotations.

enum class AnnotationType {
    Highlight,
    Underline,
    Freehand,
    Comment
};

// Active tool for UI interaction; shared between window, tab, and overlay.
enum class AnnotationTool {
    None,
    Comment,
    Highlight,
    Underline,
    Draw,
    Erase
};

struct Annotation {
    int            pageIndex = 0;      // Logical page index within the document
    AnnotationType type      = AnnotationType::Highlight;
    QRectF         rect;              // For highlight / underline / comment anchor (page coords)
    QVector<QPointF> path;            // For freehand drawing (page coords)
    QColor         color     = QColor(255, 255, 0, 128); // Default semi-transparent yellow
    QString        text;              // Comment text (for Comment type)
};

// Helpers for (de)serialising single annotations and lists.

QJsonObject annotationToJson(const Annotation &ann);
Annotation  annotationFromJson(const QJsonObject &obj, bool *ok = nullptr);

using AnnotationList = QVector<Annotation>;

QJsonArray annotationListToJson(const AnnotationList &list);
AnnotationList annotationListFromJson(const QJsonArray &array);

#endif // PDFVIEWER_ANNOTATION_H

