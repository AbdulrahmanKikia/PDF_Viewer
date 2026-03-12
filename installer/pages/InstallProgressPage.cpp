#include "InstallProgressPage.h"

#include "../InstallActions.h"
#include "../InstallerWizard.h"

#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>

InstallProgressPage::InstallProgressPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Installing"));

    m_statusLabel = new QLabel(tr("Installing files..."), this);
    m_statusLabel->setWordWrap(true);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 0);

    auto *layout = new QVBoxLayout;
    layout->addWidget(m_statusLabel);
    layout->addWidget(m_progressBar);
    setLayout(layout);
}

void InstallProgressPage::initializePage()
{
    m_finished = false;
    m_progressBar->setRange(0, 0);

    auto *wiz = qobject_cast<InstallerWizard *>(wizard());
    if (!wiz) {
        m_statusLabel->setText(tr("Internal error: wizard not found."));
        m_finished = true;
        emit completeChanged();
        return;
    }

    const QString zipPath = wiz->downloadedZipPath();
    const QString installDir = wiz->installPath();

    QString error;
    if (!InstallActions::performInstall(zipPath, installDir, &error)) {
        if (error.isEmpty())
            error = tr("Installation failed.");
        m_statusLabel->setText(error);
    } else {
        m_statusLabel->setText(tr("Installation completed successfully."));
        InstallActions::createShortcuts(installDir);
    }

    m_progressBar->setRange(0, 1);
    m_progressBar->setValue(1);

    m_finished = true;
    emit completeChanged();
}

bool InstallProgressPage::isComplete() const
{
    return m_finished;
}

