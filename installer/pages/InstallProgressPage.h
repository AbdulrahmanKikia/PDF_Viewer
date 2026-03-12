#pragma once

#include <QWizardPage>

class QLabel;
class QProgressBar;

class InstallProgressPage : public QWizardPage
{
    Q_OBJECT
public:
    explicit InstallProgressPage(QWidget *parent = nullptr);

    void initializePage() override;
    bool isComplete() const override;

private:
    QLabel *m_statusLabel = nullptr;
    QProgressBar *m_progressBar = nullptr;
    bool m_finished = false;
};

