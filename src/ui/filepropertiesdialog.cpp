#include "filepropertiesdialog.h"
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QApplication>

FilePropertiesDialog::FilePropertiesDialog(const QString &filePath, QWidget *parent)
    : QDialog(parent)
    , m_filePath(filePath)
{
    setWindowTitle(tr("Properties"));
    setModal(true);
    resize(500, 300);
    setupUI();
    loadFileInfo();
}

FilePropertiesDialog::~FilePropertiesDialog()
{
}

void FilePropertiesDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);

    QFormLayout *formLayout = new QFormLayout();
    formLayout->setSpacing(8);

    // Name
    m_nameLabel = new QLabel(this);
    formLayout->addRow(tr("Name:"), m_nameLabel);

    // Path
    m_pathEdit = new QLineEdit(this);
    m_pathEdit->setReadOnly(true);
    formLayout->addRow(tr("Location:"), m_pathEdit);

    // Type
    m_typeLabel = new QLabel(this);
    formLayout->addRow(tr("Type:"), m_typeLabel);

    // Size
    m_sizeLabel = new QLabel(this);
    formLayout->addRow(tr("Size:"), m_sizeLabel);

    // Created
    m_createdLabel = new QLabel(this);
    formLayout->addRow(tr("Created:"), m_createdLabel);

    // Modified
    m_modifiedLabel = new QLabel(this);
    formLayout->addRow(tr("Modified:"), m_modifiedLabel);

    // Read-only
    m_readOnlyCheck = new QCheckBox(tr("Read-only"), this);
    m_readOnlyCheck->setEnabled(false);
    formLayout->addRow(QString(), m_readOnlyCheck);

    mainLayout->addLayout(formLayout);
    mainLayout->addStretch();

    // OK button
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    m_okBtn = new QPushButton(tr("OK"), this);
    m_okBtn->setDefault(true);
    connect(m_okBtn, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(m_okBtn);
    mainLayout->addLayout(buttonLayout);
}

void FilePropertiesDialog::loadFileInfo()
{
    QFileInfo info(m_filePath);
    
    if (!info.exists()) {
        return;
    }

    // Name
    m_nameLabel->setText(info.fileName());

    // Path
    m_pathEdit->setText(info.absolutePath());

    // Type
    if (info.isDir()) {
        m_typeLabel->setText(tr("Folder"));
    } else {
        QString suffix = info.suffix();
        if (suffix.isEmpty()) {
            m_typeLabel->setText(tr("File"));
        } else {
            m_typeLabel->setText(QStringLiteral("%1 File").arg(suffix.toUpper()));
        }
    }

    // Size
    if (info.isDir()) {
        m_sizeLabel->setText(tr("—"));
    } else {
        qint64 size = info.size();
        QString sizeStr;
        if (size < 1024) {
            sizeStr = QStringLiteral("%1 bytes").arg(size);
        } else if (size < 1024 * 1024) {
            sizeStr = QStringLiteral("%1 KB").arg(size / 1024.0, 0, 'f', 1);
        } else if (size < 1024 * 1024 * 1024) {
            sizeStr = QStringLiteral("%1 MB").arg(size / (1024.0 * 1024.0), 0, 'f', 1);
        } else {
            sizeStr = QStringLiteral("%1 GB").arg(size / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
        }
        m_sizeLabel->setText(sizeStr);
    }

    // Created (birth time if available, otherwise use metadata)
    QDateTime created = info.birthTime();
    if (!created.isValid()) {
        created = info.metadataChangeTime();
    }
    if (created.isValid()) {
        m_createdLabel->setText(created.toString(QStringLiteral("yyyy-MM-dd hh:mm:ss")));
    } else {
        m_createdLabel->setText(tr("—"));
    }

    // Modified
    QDateTime modified = info.lastModified();
    if (modified.isValid()) {
        m_modifiedLabel->setText(modified.toString(QStringLiteral("yyyy-MM-dd hh:mm:ss")));
    } else {
        m_modifiedLabel->setText(tr("—"));
    }

    // Read-only
    m_readOnlyCheck->setChecked(!info.isWritable());
}
