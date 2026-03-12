#include "InstallerWizard.h"

#include "pages/DownloadPage.h"
#include "pages/FinishPage.h"
#include "pages/InstallLocationPage.h"
#include "pages/LicensePage.h"
#include "pages/WelcomePage.h"
#include "pages/InstallProgressPage.h"

#include <QApplication>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    InstallerWizard wizard;

    wizard.setPage(InstallerWizard::Page_Welcome, new WelcomePage);
    wizard.setPage(InstallerWizard::Page_License, new LicensePage);
    wizard.setPage(InstallerWizard::Page_InstallLocation, new InstallLocationPage);
    wizard.setPage(InstallerWizard::Page_Download, new DownloadPage);
    wizard.setPage(InstallerWizard::Page_InstallProgress, new InstallProgressPage);
    wizard.setPage(InstallerWizard::Page_Finish, new FinishPage);

    wizard.setStartId(InstallerWizard::Page_Welcome);

    return wizard.exec();
}

