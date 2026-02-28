#include "application.h"
#include "config/theme.h"
#include "config/debuglog.h"

#ifdef PDFVIEWER_USE_QTPDF
#  include "ui/simplepdfwindow.h"
#else
#  include "ui/mainwindow.h"
#endif

#include <QApplication>
#include <QIcon>
#include <QScreen>
#include <QTimer>
#include <QDebug>
#include <QFile>
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
    app.setWindowIcon(QIcon(QStringLiteral(":/icons/app.svg")));

    qInfo() << "=== PDFViewer starting ===";
    qInfo() << "Qt       :" << qVersion();
    qInfo() << "Platform :" << app.platformName();

    // Apply default theme (dark) up front.
    ThemeManager::instance()->applyTheme(ThemeManager::Dark);

#ifdef PDFVIEWER_USE_QTPDF
    SimplePdfWindow w;
#else
    const bool noQss = false;
    const bool noSession = false;
    MainWindow w(noQss, noSession, nullptr);
#endif
    w.show();

    return app.exec();
}
