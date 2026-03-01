#include "AnnotationOverlayWidget.h"

#include "../pdftabwidget.h"
#include "../../pdf/AnnotationStore.h"

#include <QPdfDocument>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QInputDialog>
#include <QLineEdit>

AnnotationOverlayWidget::AnnotationOverlayWidget(PdfTabWidget *tab, QWidget *parent)
    : QWidget(parent)
    , m_tab(tab)
{
    setAttribute(Qt::WA_TranslucentBackground, true);
    // Start in browse mode so QPdfView handles all interaction by default.
    setAnnotationTool(AnnotationTool::None);
    if (parent)
        parent->installEventFilter(this);
}

void AnnotationOverlayWidget::setAnnotationTool(AnnotationTool tool)
{
    m_tool = tool;
    const bool intercept = (tool != AnnotationTool::None);
    setAttribute(Qt::WA_TransparentForMouseEvents, !intercept);
}

bool AnnotationOverlayWidget::computeTransform(int pageIndex, qreal &scale, QPointF &offset) const
{
    if (!m_tab || !m_tab->hasDocument())
        return false;

    QPdfDocument *doc = m_tab->document();
    if (!doc)
        return false;

    if (pageIndex < 0 || pageIndex >= doc->pageCount())
        return false;

    const QSizeF pageSizePt = doc->pagePointSize(pageIndex);
    if (pageSizePt.isEmpty())
        return false;

    const QRectF vpRect = rect();

    const qreal scaleX = vpRect.width()  / pageSizePt.width();
    const qreal scaleY = vpRect.height() / pageSizePt.height();
    scale = qMin(scaleX, scaleY);

    const QSizeF renderSize(pageSizePt.width() * scale,
                            pageSizePt.height() * scale);

    const qreal offsetX = (vpRect.width()  - renderSize.width())  / 2.0;
    const qreal offsetY = (vpRect.height() - renderSize.height()) / 2.0;

    offset = QPointF(offsetX, offsetY);
    return true;
}

void AnnotationOverlayWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    if (!m_tab || !m_tab->hasDocument())
        return;

    const int logicalPage = m_tab->currentPage();
    const int physicalPage = m_tab->logicalToPhysicalPage(logicalPage);

    qreal scale = 1.0;
    QPointF offset;
    if (!computeTransform(physicalPage, scale, offset))
        return;

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const AnnotationList list = m_tab->annotations().annotationsForPage(logicalPage);
    for (const Annotation &ann : list) {
        p.save();

        const QColor color = ann.color.isValid()
                                 ? ann.color
                                 : QColor(255, 255, 0, 128);
        p.setPen(Qt::NoPen);
        p.setBrush(color);

        if (!ann.rect.isNull()) {
            const QRectF r = ann.rect;
            const QRectF rPx(offset.x() + r.x() * scale,
                             offset.y() + r.y() * scale,
                             r.width() * scale,
                             r.height() * scale);

            if (ann.type == AnnotationType::Highlight || ann.type == AnnotationType::Comment) {
                p.fillRect(rPx, color);
            } else if (ann.type == AnnotationType::Underline) {
                QPen pen(color.darker());
                pen.setWidthF(2.0);
                p.setPen(pen);
                const qreal y = rPx.bottom();
                p.drawLine(QPointF(rPx.left(), y), QPointF(rPx.right(), y));
            }

            if (ann.type == AnnotationType::Comment && !ann.text.isEmpty()) {
                QPen border(Qt::black);
                border.setWidth(1);
                p.setPen(border);
                p.setBrush(QColor(255, 255, 224, 230)); // light note color
                const QRectF bubble(rPx.topRight() + QPointF(4, 0),
                                    QSizeF(160, 60));
                p.drawRoundedRect(bubble, 4, 4);
                p.setPen(Qt::black);
                p.drawText(bubble.adjusted(4, 4, -4, -4),
                           Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
                           ann.text);
            }
        } else if (!ann.path.isEmpty()) {
            QPen pen(color);
            pen.setWidthF(2.0);
            pen.setCapStyle(Qt::RoundCap);
            pen.setJoinStyle(Qt::RoundJoin);
            p.setPen(pen);
            p.setBrush(Qt::NoBrush);

            QPainterPath path;
            bool first = true;
            for (const QPointF &pt : ann.path) {
                const QPointF ptPx(offset.x() + pt.x() * scale,
                                   offset.y() + pt.y() * scale);
                if (first) {
                    path.moveTo(ptPx);
                    first = false;
                } else {
                    path.lineTo(ptPx);
                }
            }
            p.drawPath(path);
        }

        p.restore();
    }

    // In-progress highlight / freehand preview.
    if (m_dragging && (m_tool == AnnotationTool::Highlight || m_tool == AnnotationTool::Draw)) {
        p.save();
        const QColor previewColor(255, 255, 0, 80);
        if (m_tool == AnnotationTool::Highlight) {
            QRectF r(QPointF(m_dragStartPage.x(), m_dragStartPage.y()),
                     QPointF(m_dragLastPage.x(), m_dragLastPage.y()));
            r = r.normalized();
            QRectF rPx(offset.x() + r.x() * scale,
                       offset.y() + r.y() * scale,
                       r.width() * scale,
                       r.height() * scale);
            p.setPen(Qt::NoPen);
            p.setBrush(previewColor);
            p.fillRect(rPx, previewColor);
        } else if (m_tool == AnnotationTool::Draw && !m_freehandPoints.isEmpty()) {
            QPen pen(previewColor);
            pen.setWidthF(2.0);
            pen.setCapStyle(Qt::RoundCap);
            pen.setJoinStyle(Qt::RoundJoin);
            p.setPen(pen);
            p.setBrush(Qt::NoBrush);

            QPainterPath path;
            bool first = true;
            for (const QPointF &pt : m_freehandPoints) {
                const QPointF ptPx(offset.x() + pt.x() * scale,
                                   offset.y() + pt.y() * scale);
                if (first) {
                    path.moveTo(ptPx);
                    first = false;
                } else {
                    path.lineTo(ptPx);
                }
            }
            p.drawPath(path);
        }
        p.restore();
    }
}

static QPointF mapToPage(const QPointF &viewPos, qreal scale, const QPointF &offset)
{
    return QPointF((viewPos.x() - offset.x()) / scale,
                   (viewPos.y() - offset.y()) / scale);
}

void AnnotationOverlayWidget::mousePressEvent(QMouseEvent *event)
{
    if (!m_tab || !m_tab->hasDocument() || m_tool == AnnotationTool::None) {
        QWidget::mousePressEvent(event);
        return;
    }

    const int logicalPage   = m_tab->currentPage();
    const int physicalPage  = m_tab->logicalToPhysicalPage(logicalPage);

    qreal scale = 1.0;
    QPointF offset;
    if (!computeTransform(physicalPage, scale, offset)) {
        QWidget::mousePressEvent(event);
        return;
    }

    const QPointF pagePt = mapToPage(event->position(), scale, offset);

    if (m_tool == AnnotationTool::Highlight || m_tool == AnnotationTool::Draw) {
        m_dragging      = true;
        m_dragStartPage = pagePt;
        m_dragLastPage  = pagePt;
        m_freehandPoints.clear();
        if (m_tool == AnnotationTool::Draw)
            m_freehandPoints.append(pagePt);
        update();
    } else if (m_tool == AnnotationTool::Comment) {
        bool ok = false;
        const QString text = QInputDialog::getText(this, tr("Add Comment"),
                                                   tr("Comment text:"), QLineEdit::Normal,
                                                   QString(), &ok);
        if (ok && !text.isEmpty()) {
            Annotation ann;
            ann.pageIndex = logicalPage;
            ann.type      = AnnotationType::Comment;
            ann.rect      = QRectF(pagePt, QSizeF(40.0, 20.0));
            ann.color     = QColor(255, 255, 0, 160);
            ann.text      = text;
            m_tab->annotations().addAnnotation(ann);
            update();
        }
    } else if (m_tool == AnnotationTool::Erase) {
        // Use a generous radius so clicks near visual shapes still hit.
        if (m_tab->annotations().removeNearest(logicalPage, pagePt, 40.0))
            update();
    }

    event->accept();
}

void AnnotationOverlayWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_dragging || !m_tab || !m_tab->hasDocument()) {
        QWidget::mouseMoveEvent(event);
        return;
    }

    const int logicalPage   = m_tab->currentPage();
    const int physicalPage  = m_tab->logicalToPhysicalPage(logicalPage);

    qreal scale = 1.0;
    QPointF offset;
    if (!computeTransform(physicalPage, scale, offset)) {
        QWidget::mouseMoveEvent(event);
        return;
    }

    const QPointF pagePt = mapToPage(event->position(), scale, offset);
    m_dragLastPage = pagePt;
    if (m_tool == AnnotationTool::Draw)
        m_freehandPoints.append(pagePt);
    update();
    event->accept();
}

void AnnotationOverlayWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (!m_dragging || !m_tab || !m_tab->hasDocument()) {
        m_dragging = false;
        m_freehandPoints.clear();
        QWidget::mouseReleaseEvent(event);
        return;
    }

    const int logicalPage   = m_tab->currentPage();

    if (m_tool == AnnotationTool::Highlight) {
        QRectF r(QPointF(m_dragStartPage.x(), m_dragStartPage.y()),
                 QPointF(m_dragLastPage.x(), m_dragLastPage.y()));
        r = r.normalized();
        if (!r.isNull()) {
            Annotation ann;
            ann.pageIndex = logicalPage;
            ann.type      = AnnotationType::Highlight;
            ann.rect      = r;
            ann.color     = QColor(255, 255, 0, 128);
            m_tab->annotations().addAnnotation(ann);
        }
    } else if (m_tool == AnnotationTool::Draw && !m_freehandPoints.isEmpty()) {
        Annotation ann;
        ann.pageIndex = logicalPage;
        ann.type      = AnnotationType::Freehand;
        ann.path      = m_freehandPoints;
        ann.color     = QColor(255, 0, 0, 160);
        m_tab->annotations().addAnnotation(ann);
    }

    m_dragging = false;
    m_freehandPoints.clear();
    update();
    event->accept();
}

bool AnnotationOverlayWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == parent() && event->type() == QEvent::Resize) {
        if (auto *w = qobject_cast<QWidget *>(parent()))
            setGeometry(w->rect());
    }
    return false;
}

