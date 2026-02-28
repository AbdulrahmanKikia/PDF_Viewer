#include "debuglog.h"

#include <QDateTime>
#include <QFile>
#include <QStandardPaths>

namespace DebugLog
{

static QString logFilePath()
{
    const QString baseDir =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (baseDir.isEmpty()) {
        return QString();
    }
    return baseDir + QStringLiteral("/debug.log");
}

void write(const QString &location,
           const QString &hypothesisId,
           const QString &message,
           const QString &dataJson)
{
#ifdef PDFVIEWER_ENABLE_DEBUGLOG
    const QString path = logFilePath();
    if (path.isEmpty()) {
        return;
    }

    QFile f(path);
    if (!f.open(QIODevice::Append | QIODevice::Text)) {
        return;
    }

    const QString payload = QString::fromLatin1(
        "{\"hypothesisId\":\"%1\","
        "\"location\":\"%2\","
        "\"message\":\"%3\","
        "\"data\":%4,"
        "\"timestamp\":%5}\n")
            .arg(hypothesisId,
                 location,
                 message,
                 dataJson.isEmpty() ? QStringLiteral("{}") : dataJson,
                 QString::number(QDateTime::currentMSecsSinceEpoch()));
    f.write(payload.toUtf8());
    f.flush();
#else
    Q_UNUSED(location);
    Q_UNUSED(hypothesisId);
    Q_UNUSED(message);
    Q_UNUSED(dataJson);
#endif
}

} // namespace DebugLog

