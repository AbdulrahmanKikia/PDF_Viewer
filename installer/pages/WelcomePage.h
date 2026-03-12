#pragma once

#include <QWizardPage>

class QLabel;

class WelcomePage : public QWizardPage
{
    Q_OBJECT
public:
    explicit WelcomePage(QWidget *parent = nullptr);
};

