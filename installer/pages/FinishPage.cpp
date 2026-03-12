#include "FinishPage.h"

#include "../InstallerWizard.h"

#include <QCheckBox>
#include <QDir>
#include <QLabel>
#include <QProcess>
#include <QVBoxLayout>

FinishPage::FinishPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Completed"));

    m_summaryLabel = new QLabel(this);
    m_summaryLabel->setWordWrap(true);

    m_launchCheckBox = new QCheckBox(tr("Launch PDF Viewer now"), this);
    m_launchCheckBox->setChecked(true);

    auto *layout = new QVBoxLayout;
    layout->addWidget(m_summaryLabel);
    layout->addWidget(m_launchCheckBox);
    setLayout(layout);
}

void FinishPage::initializePage()
{
    QString installPath;
    if (auto *wiz = qobject_cast<InstallerWizard *>(wizard())) {
        installPath = wiz->installPath();
    }

    m_summaryLabel->setText(tr("Installation complete.\n\n"
                               "Installed to: %1").arg(installPath));
}

bool FinishPage::isComplete() const
{
    return true;
}

void FinishPage::cleanupPage()
{
    if (!m_launchCheckBox || !m_launchCheckBox->isChecked())
        return;

    auto *wiz = qobject_cast<InstallerWizard *>(wizard());
    if (!wiz)
        return;

    const QString installDir = wiz->installPath();
    if (installDir.isEmpty())
        return;

    QDir dir(installDir);
    const QString exePath = dir.filePath(QStringLiteral("PDFViewer.exe"));
    QProcess::startDetached(exePath, {});
}

