#include "theme.h"
#include <QApplication>
#include <QFile>
#include <QFont>
#include <QPalette>
#include <QStyle>
#include <QDebug>
#include <QDateTime>
#include "debuglog.h"

#ifdef Q_OS_WIN
#include <QSettings>
#endif

// #region agent log (optional)
static inline void tLog(const QString &hyp, const QString &msg, const QString &data = {})
{
    DebugLog::write(QStringLiteral("theme.cpp"), hyp, msg, data);
}
// #endregion

ThemeManager *ThemeManager::s_instance = nullptr;

ThemeManager::ThemeManager(QObject *parent)
    : QObject(parent)
    , m_currentTheme(Dark)
{
    initColorMaps();
}

ThemeManager *ThemeManager::instance()
{
    if (!s_instance)
        s_instance = new ThemeManager(qApp);
    return s_instance;
}

void ThemeManager::initColorMaps()
{
    m_darkColors[QStringLiteral("base")]    = QStringLiteral("#1e1e1e");
    m_darkColors[QStringLiteral("text")]    = QStringLiteral("#ffffff");
    m_darkColors[QStringLiteral("accent")]  = QStringLiteral("#00bfff");
    m_darkColors[QStringLiteral("surface")] = QStringLiteral("#2e2e2e");
    m_darkColors[QStringLiteral("border")]  = QStringLiteral("#3e3e3e");

    m_lightColors[QStringLiteral("base")]    = QStringLiteral("#ffffff");
    m_lightColors[QStringLiteral("text")]    = QStringLiteral("#1a1a1a");
    m_lightColors[QStringLiteral("accent")]  = QStringLiteral("#0084ff");
    m_lightColors[QStringLiteral("surface")] = QStringLiteral("#f0f0f0");
    m_lightColors[QStringLiteral("border")]  = QStringLiteral("#e0e0e0");
}

bool ThemeManager::shouldUseDarkTheme() const
{
#ifdef Q_OS_WIN
    QSettings s(
        QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows"
                       "\\CurrentVersion\\Themes\\Personalize"),
        QSettings::NativeFormat);
    return s.value(QStringLiteral("AppsUseLightTheme"), 1).toInt() == 0;
#else
    if (qEnvironmentVariableIsSet("COLORFGBG")) {
        QString fgbg = qEnvironmentVariable("COLORFGBG");
        if (fgbg.endsWith(QLatin1String("0")))
            return true;
    }
    return true; // default dark on non-Windows
#endif
}

QString ThemeManager::stylesheetPathForTheme(Theme theme) const
{
    bool dark = (theme == Dark) || (theme == System && shouldUseDarkTheme());
    return dark ? QStringLiteral(":/styles/dark.qss")
                : QStringLiteral(":/styles/light.qss");
}

// ---------------------------------------------------------------------------
// qssModeFromEnv  — reads PDFVIEWER_QSS_MODE
// ---------------------------------------------------------------------------
ThemeManager::QssMode ThemeManager::qssModeFromEnv()
{
    const QString val = qEnvironmentVariable("PDFVIEWER_QSS_MODE").toLower().trimmed();
    if (val == QLatin1String("common"))        return QssMode::CommonOnly;
    if (val == QLatin1String("common+dark"))   return QssMode::CommonDark;
    if (val == QLatin1String("common+light"))  return QssMode::CommonLight;
    return QssMode::Full;
}

// ---------------------------------------------------------------------------
// loadQssFile  — loads one embedded QSS resource, logs if debug is on
// ---------------------------------------------------------------------------
QString ThemeManager::loadQssFile(const QString &resourcePath)
{
    QFile f(resourcePath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (m_qssDebug)
            qWarning() << "[QSS] MISSING resource:" << resourcePath;
        return {};
    }
    const QString content = QString::fromUtf8(f.readAll());
    if (m_qssDebug) {
        qInfo() << "[QSS] loaded" << resourcePath
                << content.size() << "chars";
    }
    return content;
}

// ---------------------------------------------------------------------------
// applyTheme  — assembles and applies the stylesheet
// ---------------------------------------------------------------------------
void ThemeManager::applyTheme(Theme theme)
{
    Theme effective = theme;
    if (theme == System)
        effective = shouldUseDarkTheme() ? Dark : Light;
    m_currentTheme = effective;

    // #region agent log
    tLog("H-1", "applyTheme entry",
        QString("{\"theme\":%1,\"effective\":\"%2\",\"styleName\":\"%3\"}")
            .arg(theme)
            .arg(effective == Dark ? "dark" : "light")
            .arg(QApplication::style() ? QString::fromLatin1(QApplication::style()->metaObject()->className()) : QStringLiteral("null")));
    // #endregion

    const QssMode mode = qssModeFromEnv();

    if (m_qssDebug) {
        const char* modeStr =
            (mode == QssMode::CommonOnly)  ? "common" :
            (mode == QssMode::CommonDark)  ? "common+dark" :
            (mode == QssMode::CommonLight) ? "common+light" : "full";
        qInfo() << "[QSS] mode=" << modeStr
                << "theme=" << (effective == Dark ? "dark" : "light");
    }

    QString sheet;

    // Always load common (typography, spacing, shapes — no color).
    const QString commonSrc = loadQssFile(QStringLiteral(":/styles/common.qss"));
    sheet += commonSrc;

    // Load the theme file unless we are in CommonOnly mode.
    if (mode != QssMode::CommonOnly) {
        QString themePath;
        if (mode == QssMode::CommonDark)  themePath = QStringLiteral(":/styles/dark.qss");
        if (mode == QssMode::CommonLight) themePath = QStringLiteral(":/styles/light.qss");
        if (mode == QssMode::Full)        themePath = stylesheetPathForTheme(theme);

        if (!themePath.isEmpty())
            sheet += loadQssFile(themePath);
    }

    if (m_qssDebug)
        qInfo() << "[QSS] total stylesheet:" << sheet.size() << "chars applied";

    // Set the application font via QApplication::setFont() — NOT via QSS.
    // A font rule like "QMainWindow { font-family: ... }" activates Qt's QSS
    // painting engine for QMainWindow, which causes its central-widget area to
    // lose its native background and go transparent on Windows.  Setting the
    // font in C++ avoids activating the QSS engine for container widgets.
    QFont appFont(QStringLiteral("Segoe UI"), 10);
    appFont.setStyleHint(QFont::SansSerif);
    QApplication::setFont(appFont);

    // Set QPalette so every widget has correct base colours even where QSS
    // selectors don't reach (e.g. QMainWindow's internal body widget).
    applyPalette(effective);

    // #region agent log
    tLog("H-1", "applyTheme done",
        QString("{\"sheetLen\":%1,\"palWindow\":\"%2\",\"palBase\":\"%3\",\"styleName\":\"%4\"}")
            .arg(sheet.size())
            .arg(QApplication::palette().color(QPalette::Window).name())
            .arg(QApplication::palette().color(QPalette::Base).name())
            .arg(QApplication::style() ? QString::fromLatin1(QApplication::style()->metaObject()->className()) : QStringLiteral("null")));
    // #endregion

    qApp->setStyleSheet(sheet);
    emit themeChanged(m_currentTheme);
}

void ThemeManager::loadStylesheet(const QString &path)
{
    const QString extra = loadQssFile(path);
    qApp->setStyleSheet(qApp->styleSheet() + extra);
}

QString ThemeManager::getThemeColor(const QString &colorName) const
{
    const QMap<QString, QString> &map =
        (m_currentTheme == Light) ? m_lightColors : m_darkColors;
    auto it = map.constFind(colorName);
    return it != map.constEnd() ? it.value() : QString();
}

ThemeManager::Theme ThemeManager::currentTheme() const
{
    return m_currentTheme;
}

// ---------------------------------------------------------------------------
// applyPalette — sets QPalette so every widget has sensible base colours
// even where our QSS selectors don't reach (e.g. QMainWindow interior).
// We use the Fusion style palette builder so the results look polished.
// ---------------------------------------------------------------------------
void ThemeManager::applyPalette(Theme theme)
{
    QPalette pal;
    if (theme == Dark) {
        pal.setColor(QPalette::Window,          QColor(0x1e, 0x1e, 0x1e));
        pal.setColor(QPalette::WindowText,      QColor(0xdd, 0xdd, 0xdd));
        pal.setColor(QPalette::Base,            QColor(0x2e, 0x2e, 0x2e));
        pal.setColor(QPalette::AlternateBase,   QColor(0x24, 0x24, 0x24));
        pal.setColor(QPalette::Text,            QColor(0xdd, 0xdd, 0xdd));
        pal.setColor(QPalette::BrightText,      Qt::white);
        pal.setColor(QPalette::Button,          QColor(0x2e, 0x2e, 0x2e));
        pal.setColor(QPalette::ButtonText,      QColor(0xdd, 0xdd, 0xdd));
        pal.setColor(QPalette::Highlight,       QColor(0x00, 0x78, 0xd4));
        pal.setColor(QPalette::HighlightedText, Qt::white);
        pal.setColor(QPalette::ToolTipBase,     QColor(0x2e, 0x2e, 0x2e));
        pal.setColor(QPalette::ToolTipText,     QColor(0xdd, 0xdd, 0xdd));
        pal.setColor(QPalette::Link,            QColor(0x00, 0xbf, 0xff));
        pal.setColor(QPalette::Disabled, QPalette::Text,       QColor(0x66, 0x66, 0x66));
        pal.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(0x66, 0x66, 0x66));
        pal.setColor(QPalette::Disabled, QPalette::WindowText, QColor(0x66, 0x66, 0x66));
    } else {
        // Light palette — close to system default; overrides only what we need.
        pal = QApplication::style()->standardPalette();
        pal.setColor(QPalette::Highlight,       QColor(0x00, 0x78, 0xd4));
        pal.setColor(QPalette::HighlightedText, Qt::white);
    }
    QApplication::setPalette(pal);
}
