// pdftestwidget.h
// Developer-only PDF render-test dialog.
// Opened from Tools → "Dev: Render Test…"
//
// Lets the developer:
//   - Pick a PDF file
//   - Choose page index, zoom, rotation (0/90/180/270), quality
//   - Trigger an async render and see the result in a scroll area
//   - See backend name, page count, image size, and elapsed render time

#ifndef PDFTESTWIDGET_H
#define PDFTESTWIDGET_H

#include <QDialog>
#include "../pdf/PdfRenderService.h"

class QLabel;
class QSpinBox;
class QDoubleSpinBox;
class QComboBox;
class QPushButton;
class QScrollArea;
class QLineEdit;

class PdfTestWidget : public QDialog
{
    Q_OBJECT

public:
    explicit PdfTestWidget(QWidget* parent = nullptr);

private slots:
    void onBrowse();
    void onRender();
    void onPageRendered(const RenderKey& key, const QImage& image);
    void onRenderError(const RenderKey& key, const QString& error);

private:
    void setupUI();
    void updateInfoBar(const QString& extra = {});

    // Controls
    QLineEdit*      m_pathEdit    = nullptr;
    QSpinBox*       m_pageSpin    = nullptr;
    QDoubleSpinBox* m_zoomSpin    = nullptr;
    QComboBox*      m_rotCombo    = nullptr;   // 0 / 90 / 180 / 270
    QComboBox*      m_qualCombo   = nullptr;
    QSpinBox*       m_widthSpin   = nullptr;
    QSpinBox*       m_heightSpin  = nullptr;
    QPushButton*    m_renderBtn   = nullptr;

    // Output
    QLabel*         m_infoLabel   = nullptr;
    QLabel*         m_pageLabel   = nullptr;   // Shows rendered pixmap
    QScrollArea*    m_scrollArea  = nullptr;

    std::unique_ptr<PdfRenderService> m_service;
    RenderKey m_lastKey;
    qint64    m_renderStartMs = 0;
};

#endif // PDFTESTWIDGET_H
