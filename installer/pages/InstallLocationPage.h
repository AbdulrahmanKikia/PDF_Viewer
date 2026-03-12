#pragma once

#include <QWizardPage>

class QLineEdit;
class QPushButton;

class InstallLocationPage : public QWizardPage
{
    Q_OBJECT
public:
    explicit InstallLocationPage(QWidget *parent = nullptr);

    void initializePage() override;
    bool validatePage() override;

private slots:
    void browseForLocation();

private:
    QLineEdit *m_pathEdit = nullptr;
};

