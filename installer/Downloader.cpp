#include "Downloader.h"

#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

Downloader::Downloader(QObject *parent)
    : QObject(parent)
    , m_manager(new QNetworkAccessManager(this))
{
}

Downloader::~Downloader() = default;

void Downloader::startDownload(const QUrl &url, const QString &targetFilePath)
{
    if (m_reply) {
        m_reply->deleteLater();
        m_reply = nullptr;
    }

    m_targetFilePath = targetFilePath;

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("PDFViewerInstaller/1.0"));

    m_reply = m_manager->get(request);
    connect(m_reply, &QNetworkReply::downloadProgress,
            this, &Downloader::downloadProgress);
    connect(m_reply, &QNetworkReply::readyRead,
            this, &Downloader::onReadyRead);
    connect(m_reply, &QNetworkReply::finished,
            this, &Downloader::onFinished);
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    connect(m_reply, &QNetworkReply::errorOccurred,
            this, &Downloader::onErrorOccurred);
#endif

    // Truncate existing file, if any.
    QFile file(m_targetFilePath);
    if (file.exists())
        file.remove();
}

void Downloader::onReadyRead()
{
    if (!m_reply)
        return;

    QFile file(m_targetFilePath);
    if (!file.open(QIODevice::Append)) {
        emit finished(false, QString(), tr("Cannot write to %1").arg(m_targetFilePath));
        m_reply->abort();
        return;
    }

    const QByteArray data = m_reply->readAll();
    file.write(data);
}

void Downloader::onFinished()
{
    if (!m_reply)
        return;

    const auto error = m_reply->error();
    m_reply->deleteLater();
    m_reply = nullptr;

    if (error != QNetworkReply::NoError) {
        emit finished(false, QString(), tr("Network error during download."));
        return;
    }

    emit finished(true, m_targetFilePath, QString());
}

void Downloader::onErrorOccurred()
{
    if (!m_reply)
        return;

    const QString message = m_reply->errorString();
    emit finished(false, QString(), message);
}

