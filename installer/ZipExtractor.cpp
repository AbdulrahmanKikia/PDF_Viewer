#include "ZipExtractor.h"

#include <QDir>
#include <QProcess>

namespace ZipExtractor {

bool extractAll(const QString &zipPath, const QString &targetDir, QString *errorMessage)
{
    // For Windows-only installer we can rely on PowerShell's Expand-Archive,
    // which is available on modern Windows without additional packages.

    QDir().mkpath(targetDir);

    QProcess process;
    QStringList args;
    args << QStringLiteral("-NoProfile")
         << QStringLiteral("-NonInteractive")
         << QStringLiteral("-Command")
         << QStringLiteral("Expand-Archive -LiteralPath '%1' -DestinationPath '%2' -Force")
                .arg(zipPath, targetDir);

    process.start(QStringLiteral("powershell.exe"), args);
    if (!process.waitForFinished(10 * 60 * 1000)) { // 10 minutes
        if (errorMessage)
            *errorMessage = QObject::tr("Timed out while extracting archive.");
        process.kill();
        return false;
    }

    const int exitCode = process.exitCode();
    if (exitCode != 0) {
        if (errorMessage) {
            const QString stdErr = QString::fromLocal8Bit(process.readAllStandardError());
            *errorMessage = QObject::tr("Extraction failed (code %1): %2")
                                .arg(exitCode)
                                .arg(stdErr.trimmed());
        }
        return false;
    }

    return true;
}

} // namespace ZipExtractor

