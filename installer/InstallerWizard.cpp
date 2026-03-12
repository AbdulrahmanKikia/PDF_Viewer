#include "InstallerWizard.h"

#include "InstallerConfig.h"

#include <QLabel>

static const char *INSTALL_PATH_FIELD = "installPath";
static const char *DOWNLOADED_ZIP_FIELD = "downloadedZipPath";

InstallerWizard::InstallerWizard(QWidget *parent)
    : QWizard(parent)
{
    setWindowTitle(QStringLiteral("%1 Setup").arg(InstallerConfig::productName()));
    setOption(QWizard::NoBackButtonOnStartPage, true);
    setWizardStyle(QWizard::ModernStyle);

    // Pages will be registered from main.cpp to avoid pulling in all headers here.
}

QString InstallerWizard::installPath() const
{
    return field(QLatin1String(INSTALL_PATH_FIELD)).toString();
}

QString InstallerWizard::downloadedZipPath() const
{
    return field(QLatin1String(DOWNLOADED_ZIP_FIELD)).toString();
}

void InstallerWizard::setDownloadedZipPath(const QString &path)
{
    setField(QLatin1String(DOWNLOADED_ZIP_FIELD), path);
}

