#include "LicensePage.h"

#include <QCheckBox>
#include <QTextEdit>
#include <QVBoxLayout>

LicensePage::LicensePage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("License Agreement"));

    auto *licenseView = new QTextEdit(this);
    licenseView->setReadOnly(true);
    licenseView->setPlainText(tr("Insert your license text here.\n\n"
                                 "By checking the box below, you agree to the terms of this license."));

    m_acceptCheckBox = new QCheckBox(tr("I accept the terms of the license agreement."), this);

    auto *layout = new QVBoxLayout;
    layout->addWidget(licenseView);
    layout->addWidget(m_acceptCheckBox);
    setLayout(layout);

    connect(m_acceptCheckBox, &QCheckBox::toggled, this, &LicensePage::completeChanged);
}

bool LicensePage::isComplete() const
{
    return m_acceptCheckBox && m_acceptCheckBox->isChecked();
}

