#ifndef PDFVIEWER_ANNOTATIONOVERLAYWIDGET_H
#define PDFVIEWER_ANNOTATIONOVERLAYWIDGET_H

#include <QWidget>

#include "pdf/Annotation.h"

class PdfTabWidget;

// Lightweight overlay that paints annotations for the current page.
// Interaction (creating/removing annotations) will be wired in a later step.

class AnnotationOverlayWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AnnotationOverlayWidget(PdfTabWidget *tab, QWidget *parent = nullptr);

    void setAnnotationTool(AnnotationTool tool);
    AnnotationTool annotationTool() const { return m_tool; }

protected:
    void paintEvent(QPaintEvent *event) override;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    PdfTabWidget  *m_tab  = nullptr;
    AnnotationTool m_tool = AnnotationTool::None;

    bool    m_dragging      = false;
    QPointF m_dragStartPage;
    QPointF m_dragLastPage;
    QVector<QPointF> m_freehandPoints;

    // Helpers to compute scale and offset mapping page coords to overlay coords.
    bool computeTransform(int pageIndex, qreal &scale, QPointF &offset) const;
};

#endif // PDFVIEWER_ANNOTATIONOVERLAYWIDGET_H

