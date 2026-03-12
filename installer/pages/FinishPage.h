#pragma once

#include <QWizardPage>

class QCheckBox;
class QLabel;

class FinishPage : public QWizardPage
{
    Q_OBJECT
public:
    explicit FinishPage(QWidget *parent = nullptr);

    void initializePage() override;
    bool isComplete() const override;
    void cleanupPage() override;

private:
    QCheckBox *m_launchCheckBox = nullptr;
    QLabel *m_summaryLabel = nullptr;
};

