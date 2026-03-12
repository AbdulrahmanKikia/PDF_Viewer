#include "DownloadPage.h"

#include "../Downloader.h"
#include "../InstallerConfig.h"
#include "../InstallerWizard.h"

#include <QDir>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QStandardPaths>
#include <QUrl>
#include <QVBoxLayout>

DownloadPage::DownloadPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Download Files"));

    m_statusLabel = new QLabel(tr("Click Next to start downloading the application files."), this);
    m_statusLabel->setWordWrap(true);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 0);
    m_progressBar->setValue(0);

    m_retryButton = new QPushButton(tr("Retry"), this);
    m_retryButton->setEnabled(false);
    connect(m_retryButton, &QPushButton::clicked,
            this, &DownloadPage::onRetryClicked);

    auto *layout = new QVBoxLayout;
    layout->addWidget(m_statusLabel);
    layout->addWidget(m_progressBar);
    layout->addWidget(m_retryButton);
    setLayout(layout);

    m_downloader = new Downloader(this);
    connect(m_downloader, &Downloader::downloadProgress,
            this, &DownloadPage::onProgress);
    connect(m_downloader, &Downloader::finished,
            this, &DownloadPage::onFinished);
}

void DownloadPage::initializePage()
{
    m_finishedOk = false;
    setUiBusy(true);
    startDownload();
}

bool DownloadPage::isComplete() const
{
    return m_finishedOk;
}

void DownloadPage::startDownload()
{
    const QString tempDir =
        QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QDir dir(tempDir.isEmpty() ? QDir::currentPath() : tempDir);
    const QString targetFile =
        dir.filePath(InstallerConfig::downloadedZipFileName());

    QUrl url(InstallerConfig::downloadUrl());

    m_statusLabel->setText(tr("Downloading from %1").arg(url.toString()));
    m_progressBar->setRange(0, 0);

    m_downloader->startDownload(url, targetFile);

    if (auto *wiz = qobject_cast<InstallerWizard *>(wizard())) {
        wiz->setDownloadedZipPath(targetFile);
    }
}

void DownloadPage::onProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (bytesTotal <= 0) {
        m_progressBar->setRange(0, 0);
        return;
    }

    m_progressBar->setRange(0, static_cast<int>(bytesTotal));
    m_progressBar->setValue(static_cast<int>(bytesReceived));
}

void DownloadPage::onFinished(bool ok, const QString &filePath, const QString &errorString)
{
    if (ok) {
        m_statusLabel->setText(tr("Download completed.\n%1").arg(filePath));
        m_finishedOk = true;
        m_retryButton->setEnabled(false);
    } else {
        const QString msg = errorString.isEmpty()
                                ? tr("Download failed.")
                                : tr("Download failed: %1").arg(errorString);
        m_statusLabel->setText(msg);
        m_finishedOk = false;
        m_retryButton->setEnabled(true);
    }

    setUiBusy(false);
    emit completeChanged();
}

void DownloadPage::onRetryClicked()
{
    m_finishedOk = false;
    setUiBusy(true);
    startDownload();
}

void DownloadPage::setUiBusy(bool busy)
{
    m_progressBar->setEnabled(busy);
}

