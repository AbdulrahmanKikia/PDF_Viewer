#ifndef THEME_H
#define THEME_H

#include <QMap>
#include <QObject>
#include <QString>

class ThemeManager : public QObject
{
    Q_OBJECT
public:
    enum Theme { Light, Dark, System };

    // QSS loading mode — controlled by PDFVIEWER_QSS_MODE env var.
    // Allows bisecting which file causes visual breakage.
    enum class QssMode {
        Full,         // common + theme  (default)
        CommonOnly,   // common.qss only
        CommonDark,   // common + dark
        CommonLight,  // common + light
    };

    static ThemeManager *instance();

    void applyTheme(Theme theme);
    void loadStylesheet(const QString &path);
    QString getThemeColor(const QString &colorName) const;
    Theme currentTheme() const;
    bool shouldUseDarkTheme() const;

    // Enable verbose QSS loading diagnostics (--qss-debug flag).
    void setQssDebug(bool on) { m_qssDebug = on; }
    bool qssDebug() const     { return m_qssDebug; }

    // Read PDFVIEWER_QSS_MODE from the environment.
    static QssMode qssModeFromEnv();

signals:
    void themeChanged(Theme theme);

private:
    explicit ThemeManager(QObject *parent = nullptr);
    static ThemeManager *s_instance;

    Theme m_currentTheme = Dark;
    bool  m_qssDebug     = false;
    QMap<QString, QString> m_darkColors;
    QMap<QString, QString> m_lightColors;

    void initColorMaps();
    QString stylesheetPathForTheme(Theme theme) const;

    // Load one QSS file; returns the content (empty on failure).
    QString loadQssFile(const QString &resourcePath);

    // Set QPalette for dark/light so widgets have correct base colours
    // even where QSS rules don't reach.
    void applyPalette(Theme theme);
};

#endif // THEME_H
