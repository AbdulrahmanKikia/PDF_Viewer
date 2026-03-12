#pragma once

#include <QWizardPage>

class QLabel;
class QProgressBar;
class QPushButton;

class Downloader;

class DownloadPage : public QWizardPage
{
    Q_OBJECT
public:
    explicit DownloadPage(QWidget *parent = nullptr);

    void initializePage() override;
    bool isComplete() const override;

private slots:
    void startDownload();
    void onProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onFinished(bool ok, const QString &filePath, const QString &errorString);
    void onRetryClicked();

private:
    void setUiBusy(bool busy);

    Downloader *m_downloader = nullptr;
    QLabel *m_statusLabel = nullptr;
    QProgressBar *m_progressBar = nullptr;
    QPushButton *m_retryButton = nullptr;
    bool m_finishedOk = false;
};

