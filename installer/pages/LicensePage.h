#pragma once

#include <QWizardPage>

class QCheckBox;
class QTextEdit;

class LicensePage : public QWizardPage
{
    Q_OBJECT
public:
    explicit LicensePage(QWidget *parent = nullptr);

    bool isComplete() const override;

private:
    QCheckBox *m_acceptCheckBox = nullptr;
};

