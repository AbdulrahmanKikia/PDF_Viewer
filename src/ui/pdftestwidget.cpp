// pdftestwidget.cpp
#include "pdftestwidget.h"
#include "../pdf/PdfRendererFactory.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QScrollArea>
#include <QLineEdit>
#include <QFileDialog>
#include <QMessageBox>
#include <QDateTime>
#include <QPixmap>
#include <QSizePolicy>

PdfTestWidget::PdfTestWidget(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Dev: PDF Renderer Test — %1")
                   .arg(PdfRendererFactory::activeBackendName()));
    setMinimumSize(800, 600);
    resize(1050, 740);
    setupUI();

    // Create service with the best available backend.
    m_service = std::make_unique<PdfRenderService>(
        PdfRendererFactory::create(), this);

    // Qt::QueuedConnection so worker-thread signals are delivered on the UI thread.
    connect(m_service.get(), &PdfRenderService::pageRendered,
            this, &PdfTestWidget::onPageRendered, Qt::QueuedConnection);
    connect(m_service.get(), &PdfRenderService::renderError,
            this, &PdfTestWidget::onRenderError,  Qt::QueuedConnection);

    updateInfoBar();
}

void PdfTestWidget::setupUI()
{
    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(6);

    // ---- Controls group ----
    QGroupBox*   grp  = new QGroupBox(tr("Controls"), this);
    QFormLayout* form = new QFormLayout(grp);

    // File path row
    QHBoxLayout* pathRow = new QHBoxLayout;
    m_pathEdit = new QLineEdit(this);
    m_pathEdit->setPlaceholderText(tr("Path to PDF file…"));
    QPushButton* browseBtn = new QPushButton(tr("Browse…"), this);
    connect(browseBtn, &QPushButton::clicked, this, &PdfTestWidget::onBrowse);
    pathRow->addWidget(m_pathEdit, 1);
    pathRow->addWidget(browseBtn);
    form->addRow(tr("PDF file:"), pathRow);

    // Page index
    m_pageSpin = new QSpinBox(this);
    m_pageSpin->setRange(0, 99999);
    m_pageSpin->setValue(0);
    m_pageSpin->setToolTip(tr("0-based page index"));
    form->addRow(tr("Page index:"), m_pageSpin);

    // Zoom
    m_zoomSpin = new QDoubleSpinBox(this);
    m_zoomSpin->setRange(0.05, 16.0);
    m_zoomSpin->setSingleStep(0.25);
    m_zoomSpin->setValue(1.5);
    m_zoomSpin->setDecimals(2);
    m_zoomSpin->setToolTip(tr("Zoom factor (1.0 = 72 dpi)"));
    form->addRow(tr("Zoom:"), m_zoomSpin);

    // Rotation
    m_rotCombo = new QComboBox(this);
    m_rotCombo->addItem(tr("0° (no rotation)"), 0);
    m_rotCombo->addItem(tr("90° clockwise"),    90);
    m_rotCombo->addItem(tr("180°"),            180);
    m_rotCombo->addItem(tr("270° clockwise"),  270);
    form->addRow(tr("Rotation:"), m_rotCombo);

    // Quality
    m_qualCombo = new QComboBox(this);
    m_qualCombo->addItem(tr("Fast"),     static_cast<int>(RenderQuality::Fast));
    m_qualCombo->addItem(tr("Balanced"), static_cast<int>(RenderQuality::Balanced));
    m_qualCombo->addItem(tr("Best"),     static_cast<int>(RenderQuality::Best));
    m_qualCombo->setCurrentIndex(1);
    form->addRow(tr("Quality:"), m_qualCombo);

    // Target pixel size (0 = unconstrained)
    QHBoxLayout* sizeRow = new QHBoxLayout;
    m_widthSpin  = new QSpinBox(this);
    m_heightSpin = new QSpinBox(this);
    for (QSpinBox* s : {m_widthSpin, m_heightSpin}) {
        s->setRange(0, 8000);
        s->setValue(0);
        s->setSpecialValueText(tr("auto"));
        s->setToolTip(tr("0 = unconstrained"));
    }
    m_widthSpin->setValue(1200);
    sizeRow->addWidget(m_widthSpin);
    sizeRow->addWidget(new QLabel(QStringLiteral("×"), this));
    sizeRow->addWidget(m_heightSpin);
    form->addRow(tr("Max pixels W×H:"), sizeRow);

    // Render button
    m_renderBtn = new QPushButton(tr("Render Page"), this);
    m_renderBtn->setDefault(true);
    connect(m_renderBtn, &QPushButton::clicked, this, &PdfTestWidget::onRender);
    form->addRow(m_renderBtn);

    root->addWidget(grp);

    // ---- Info bar ----
    m_infoLabel = new QLabel(tr("Ready."), this);
    m_infoLabel->setWordWrap(true);
    m_infoLabel->setObjectName(QStringLiteral("PdfTestInfo"));
    root->addWidget(m_infoLabel);

    // ---- Scroll area for the rendered image ----
    m_pageLabel = new QLabel(this);
    m_pageLabel->setAlignment(Qt::AlignCenter);
    m_pageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    m_pageLabel->setMinimumSize(100, 100);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidget(m_pageLabel);
    m_scrollArea->setWidgetResizable(false);
    m_scrollArea->setAlignment(Qt::AlignCenter);
    root->addWidget(m_scrollArea, 1);
}

void PdfTestWidget::onBrowse()
{
    QString path = QFileDialog::getOpenFileName(
        this, tr("Open PDF File"), QString(), tr("PDF files (*.pdf);;All files (*)"));
    if (!path.isEmpty())
        m_pathEdit->setText(path);
}

void PdfTestWidget::onRender()
{
    const QString path = m_pathEdit->text().trimmed();
    if (path.isEmpty()) {
        QMessageBox::warning(this, tr("No file selected"),
                             tr("Please choose a PDF file first."));
        return;
    }

    // Re-open if path changed.
    if (path != m_service->filePath()) {
        if (m_service->isOpen())
            m_service->close();
        if (!m_service->open(path)) {
            QMessageBox::critical(this, tr("Open failed"),
                tr("Could not open PDF:\n%1\n\nError: %2")
                    .arg(path, m_service->lastError()));
            return;
        }
        m_pageSpin->setMaximum(qMax(0, m_service->pageCount() - 1));
    }

    RenderKey key;
    key.filePath    = path;
    key.pageIndex   = m_pageSpin->value();
    key.zoom        = float(m_zoomSpin->value());
    key.rotationDeg = m_rotCombo->currentData().toInt();
    key.targetW     = m_widthSpin->value();
    key.targetH     = m_heightSpin->value();
    key.quality     = static_cast<RenderQuality>(m_qualCombo->currentData().toInt());

    m_lastKey       = key;
    m_renderStartMs = QDateTime::currentMSecsSinceEpoch();
    m_renderBtn->setEnabled(false);
    m_infoLabel->setText(tr("Rendering page %1 (rot=%2°)…")
                         .arg(key.pageIndex).arg(key.rotationDeg));

    m_service->requestRender(key);
}

void PdfTestWidget::onPageRendered(const RenderKey& key, const QImage& image)
{
    if (key != m_lastKey)
        return; // Stale result from a previous request — ignore.

    m_renderBtn->setEnabled(true);
    const qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - m_renderStartMs;

    QPixmap pm = QPixmap::fromImage(image);
    m_pageLabel->setPixmap(pm);
    m_pageLabel->resize(pm.size());

    PageRenderCache& cache = PdfRenderService::globalCache();
    updateInfoBar(
        tr("Page %1 | rot %2° | %3×%4 px | %5 ms  ·  Cache: %6 entries / %7 MB")
            .arg(key.pageIndex)
            .arg(key.rotationDeg)
            .arg(image.width())
            .arg(image.height())
            .arg(elapsed)
            .arg(cache.count())
            .arg(cache.usedBytes() / (1024 * 1024)));
}

void PdfTestWidget::onRenderError(const RenderKey& key, const QString& error)
{
    if (key != m_lastKey) return;
    m_renderBtn->setEnabled(true);
    m_infoLabel->setText(tr("Render error on page %1: %2")
                         .arg(key.pageIndex).arg(error));
}

void PdfTestWidget::updateInfoBar(const QString& extra)
{
    QString info = tr("Backend: %1").arg(m_service->backendName());
    if (m_service->isOpen()) {
        info += tr("  |  Pages: %1  |  %2")
                    .arg(m_service->pageCount())
                    .arg(m_service->filePath());
    }
    if (!extra.isEmpty())
        info += QLatin1Char('\n') + extra;
    m_infoLabel->setText(info);
}
