#pragma once

#include <QWizard>

class InstallerWizard : public QWizard
{
    Q_OBJECT
public:
    enum PageId {
        Page_Welcome = 0,
        Page_License,
        Page_InstallLocation,
        Page_Download,
        Page_InstallProgress,
        Page_Finish
    };

    explicit InstallerWizard(QWidget *parent = nullptr);

    QString installPath() const;
    QString downloadedZipPath() const;

    void setDownloadedZipPath(const QString &path);
};

