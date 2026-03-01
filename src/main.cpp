#include "application.h"
#include "config/debuglog.h"

#ifdef PDFVIEWER_USE_QTPDF
#  include "ui/simplepdfwindow.h"
#else
#  include "ui/mainwindow.h"
#endif

#include <QApplication>
#include <QCommandLineParser>
#include <QIcon>
#include <QPixmap>
#include <QScreen>
#include <QTimer>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QStyle>

// #region agent log (optional)
static inline void mainLog(const QString &hyp, const QString &msg, const QString &data = {})
{
    DebugLog::write(QStringLiteral("main.cpp"), hyp, msg, data);
}
// #endregion

int main(int argc, char *argv[])
{
    qSetMessagePattern(QStringLiteral("[%{type}] %{message}"));

    PDFViewerApp app(argc, argv);
    QPixmap logo(QStringLiteral(":/icons/app_logo.png"));
    QIcon appIcon;
    for (int sz : {16, 24, 32, 48, 64, 128, 256})
        appIcon.addPixmap(logo.scaled(sz, sz, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    app.setWindowIcon(appIcon);

    qInfo() << "=== PDFViewer starting ===";
    qInfo() << "Qt       :" << qVersion();
    qInfo() << "Platform :" << app.platformName();

#ifdef PDFVIEWER_USE_QTPDF
    SimplePdfWindow w;
#else
    const bool noQss = false;
    const bool noSession = false;
    MainWindow w(noQss, noSession, nullptr);
#endif
    w.show();

#ifdef PDFVIEWER_USE_QTPDF
    // Open a PDF passed as command-line argument: PDFViewer.exe path/to/file.pdf
    const QStringList args = app.arguments();
    if (args.size() > 1) {
        const QString candidate = args.at(1);
        if (QFileInfo::exists(candidate) &&
            candidate.endsWith(QLatin1String(".pdf"), Qt::CaseInsensitive)) {
            w.openFilePath(candidate);
        }
    }
#endif

    return app.exec();
}
