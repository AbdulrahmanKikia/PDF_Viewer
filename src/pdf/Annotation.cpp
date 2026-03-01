#include "Annotation.h"

#include <QJsonValue>

static QString annotationTypeToString(AnnotationType t)
{
    switch (t) {
    case AnnotationType::Highlight: return QStringLiteral("highlight");
    case AnnotationType::Underline: return QStringLiteral("underline");
    case AnnotationType::Freehand:  return QStringLiteral("freehand");
    case AnnotationType::Comment:   return QStringLiteral("comment");
    }
    return QStringLiteral("highlight");
}

static AnnotationType annotationTypeFromString(const QString &s)
{
    if (s == QLatin1String("underline"))
        return AnnotationType::Underline;
    if (s == QLatin1String("freehand"))
        return AnnotationType::Freehand;
    if (s == QLatin1String("comment"))
        return AnnotationType::Comment;
    return AnnotationType::Highlight;
}

QJsonObject annotationToJson(const Annotation &ann)
{
    QJsonObject obj;
    obj[QStringLiteral("pageIndex")] = ann.pageIndex;
    obj[QStringLiteral("type")]      = annotationTypeToString(ann.type);

    // Rect
    if (!ann.rect.isNull()) {
        QJsonObject r;
        r[QStringLiteral("x")]      = ann.rect.x();
        r[QStringLiteral("y")]      = ann.rect.y();
        r[QStringLiteral("width")]  = ann.rect.width();
        r[QStringLiteral("height")] = ann.rect.height();
        obj[QStringLiteral("rect")] = r;
    }

    // Path (for freehand)
    if (!ann.path.isEmpty()) {
        QJsonArray pts;
        for (const QPointF &p : ann.path) {
            QJsonObject po;
            po[QStringLiteral("x")] = p.x();
            po[QStringLiteral("y")] = p.y();
            pts.append(po);
        }
        obj[QStringLiteral("path")] = pts;
    }

    // Color
    obj[QStringLiteral("color")] = ann.color.name(QColor::HexArgb);

    // Text
    if (!ann.text.isEmpty())
        obj[QStringLiteral("text")] = ann.text;

    return obj;
}

Annotation annotationFromJson(const QJsonObject &obj, bool *ok)
{
    Annotation ann;
    bool valid = true;

    ann.pageIndex = obj.value(QStringLiteral("pageIndex")).toInt(0);
    ann.type      = annotationTypeFromString(
                        obj.value(QStringLiteral("type")).toString(
                            QStringLiteral("highlight")));

    // Rect
    const QJsonValue rectVal = obj.value(QStringLiteral("rect"));
    if (rectVal.isObject()) {
        const QJsonObject r = rectVal.toObject();
        const double x      = r.value(QStringLiteral("x")).toDouble(0.0);
        const double y      = r.value(QStringLiteral("y")).toDouble(0.0);
        const double w      = r.value(QStringLiteral("width")).toDouble(0.0);
        const double h      = r.value(QStringLiteral("height")).toDouble(0.0);
        ann.rect = QRectF(x, y, w, h);
    }

    // Path
    const QJsonValue pathVal = obj.value(QStringLiteral("path"));
    if (pathVal.isArray()) {
        const QJsonArray pts = pathVal.toArray();
        for (const QJsonValue &v : pts) {
            if (!v.isObject())
                continue;
            const QJsonObject po = v.toObject();
            const double x       = po.value(QStringLiteral("x")).toDouble(0.0);
            const double y       = po.value(QStringLiteral("y")).toDouble(0.0);
            ann.path.append(QPointF(x, y));
        }
    }

    // Color
    const QString colorStr = obj.value(QStringLiteral("color")).toString();
    if (!colorStr.isEmpty()) {
        QColor c(colorStr);
        if (c.isValid())
            ann.color = c;
        else
            valid = false;
    }

    // Text
    ann.text = obj.value(QStringLiteral("text")).toString();

    if (ok)
        *ok = valid;
    return ann;
}

QJsonArray annotationListToJson(const AnnotationList &list)
{
    QJsonArray array;
    for (const Annotation &ann : list)
        array.append(annotationToJson(ann));
    return array;
}

AnnotationList annotationListFromJson(const QJsonArray &array)
{
    AnnotationList list;
    list.reserve(array.size());
    for (const QJsonValue &v : array) {
        if (!v.isObject())
            continue;
        bool ok = false;
        const Annotation ann = annotationFromJson(v.toObject(), &ok);
        if (ok)
            list.append(ann);
    }
    return list;
}

