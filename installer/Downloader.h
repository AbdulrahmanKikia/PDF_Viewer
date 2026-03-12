#pragma once

#include <QObject>

class QNetworkAccessManager;
class QNetworkReply;
class QUrl;

class Downloader : public QObject
{
    Q_OBJECT
public:
    explicit Downloader(QObject *parent = nullptr);
    ~Downloader() override;

    void startDownload(const QUrl &url, const QString &targetFilePath);

signals:
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void finished(bool ok, const QString &filePath, const QString &errorString);

private slots:
    void onReadyRead();
    void onFinished();
    void onErrorOccurred();

private:
    QNetworkAccessManager *m_manager = nullptr;
    QNetworkReply *m_reply = nullptr;
    QString m_targetFilePath;
};

