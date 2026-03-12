#include "InstallLocationPage.h"

#include "../InstallerConfig.h"

#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QStandardPaths>
#include <QVBoxLayout>

static const char *INSTALL_PATH_FIELD = "installPath";

InstallLocationPage::InstallLocationPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Choose Install Location"));

    m_pathEdit = new QLineEdit(this);

    auto *browseButton = new QPushButton(tr("Browse..."), this);
    connect(browseButton, &QPushButton::clicked, this, &InstallLocationPage::browseForLocation);

    auto *pathLayout = new QHBoxLayout;
    pathLayout->addWidget(m_pathEdit);
    pathLayout->addWidget(browseButton);

    auto *layout = new QVBoxLayout;
    layout->addLayout(pathLayout);
    setLayout(layout);

    registerField(QLatin1String(INSTALL_PATH_FIELD), m_pathEdit);
}

void InstallLocationPage::initializePage()
{
    QString base =
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (base.isEmpty())
        base = QDir::homePath();

    QDir dir(base);
    const QString suggested =
        dir.filePath(InstallerConfig::defaultInstallDirName());

    if (m_pathEdit->text().isEmpty())
        m_pathEdit->setText(QDir::toNativeSeparators(suggested));
}

bool InstallLocationPage::validatePage()
{
    const QString path = m_pathEdit->text().trimmed();
    if (path.isEmpty())
        return false;

    QDir dir;
    // Try to create the directory if it doesn't exist.
    return dir.mkpath(path);
}

void InstallLocationPage::browseForLocation()
{
    const QString dir =
        QFileDialog::getExistingDirectory(this, tr("Select Install Folder"),
                                          m_pathEdit->text());
    if (!dir.isEmpty())
        m_pathEdit->setText(QDir::toNativeSeparators(dir));
}

