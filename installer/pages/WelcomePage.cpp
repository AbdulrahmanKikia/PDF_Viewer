#include "WelcomePage.h"

#include "../InstallerConfig.h"

#include <QLabel>
#include <QVBoxLayout>

WelcomePage::WelcomePage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Welcome to %1 Setup").arg(InstallerConfig::productName()));

    auto *label = new QLabel(tr("This wizard will install %1 on your computer.\n\n"
                                "Click Next to continue.")
                                 .arg(InstallerConfig::productName()));
    label->setWordWrap(true);

    auto *layout = new QVBoxLayout;
    layout->addWidget(label);
    setLayout(layout);
}

