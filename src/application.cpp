#include "application.h"
#include "config/theme.h"
#include <QString>
#include <QStyle>
#include <QApplication>

PDFViewerApp *PDFViewerApp::s_instance = nullptr;

PDFViewerApp::PDFViewerApp(int &argc, char **argv)
    : QApplication(argc, argv)
    , m_currentTheme(QStringLiteral("dark"))
{
    s_instance = this;

    // Use the Fusion style on all platforms so that QPalette colours are
    // respected for every widget background.  The Windows native style ignores
    // QPalette for most widget fills and always paints them using the system
    // theme (white), which breaks our dark/light palette-driven theming.
    QApplication::setStyle(QStringLiteral("Fusion"));

    setApplicationName(QStringLiteral("PDFViewer"));
    setApplicationVersion(QStringLiteral("1.2.0"));
    setOrganizationName(QStringLiteral("PDFViewer"));

    // Theme is applied by MainWindow after CLI flags (--no-qss, --qss-debug)
    // are parsed, so we do NOT call applyTheme() here.
    // m_currentTheme is initialised to "dark" as the expected default.

    connect(ThemeManager::instance(), &ThemeManager::themeChanged,
            this, [this](ThemeManager::Theme theme) {
        QString name = (theme == ThemeManager::Light) ? QStringLiteral("light")
                     : (theme == ThemeManager::Dark)  ? QStringLiteral("dark")
                                                     : QStringLiteral("system");
        m_currentTheme = name;
        emit themeChanged(m_currentTheme);
    });
}

PDFViewerApp::~PDFViewerApp()
{
    s_instance = nullptr;
}

PDFViewerApp *PDFViewerApp::instance()
{
    return s_instance;
}

void PDFViewerApp::setTheme(const QString &themeName)
{
    ThemeManager::Theme theme = ThemeManager::Dark;
    if (themeName == QLatin1String("light"))
        theme = ThemeManager::Light;
    else if (themeName == QLatin1String("system"))
        theme = ThemeManager::System;
    else
        theme = ThemeManager::Dark;

    ThemeManager::instance()->applyTheme(theme);
    // m_currentTheme and themeChanged() are updated via ThemeManager::themeChanged connection
}

QString PDFViewerApp::currentTheme() const
{
    return m_currentTheme;
}
