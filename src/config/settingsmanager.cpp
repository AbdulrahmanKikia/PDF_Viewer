// settingsmanager.cpp
#include "settingsmanager.h"

#include <QCoreApplication>
#include <QSettings>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QDebug>

// ---------------------------------------------------------------------------
// Static members
// ---------------------------------------------------------------------------
SettingsManager* SettingsManager::s_instance = nullptr;

const int SettingsManager::CURRENT_SCHEMA_VERSION = 1;

// Keys that are permanently locked and cannot be changed by users or import.
const QSet<QString> SettingsManager::LOCKED_KEYS = {
    QStringLiteral("security/requirePasswordForSigning"),
    QStringLiteral("security/signingAlgorithm"),
    QStringLiteral("security/certificateValidation"),
};

// ---------------------------------------------------------------------------
// Singleton
// ---------------------------------------------------------------------------
SettingsManager* SettingsManager::instance()
{
    if (!s_instance)
        s_instance = new SettingsManager(qApp);
    return s_instance;
}

SettingsManager::SettingsManager(QObject* parent)
    : QObject(parent)
{}

// ---------------------------------------------------------------------------
// Load
// ---------------------------------------------------------------------------
AppSettings SettingsManager::load()
{
    QSettings qs;

    // Check schema version and migrate if needed.
    int storedVersion = qs.value(QStringLiteral("settings/schemaVersion"), 0).toInt();
    if (storedVersion == 0) {
        // First run – write defaults.
        AppSettings defaults;
        writeToQSettings(defaults);
        qs.setValue(QStringLiteral("settings/schemaVersion"), CURRENT_SCHEMA_VERSION);
        // Do NOT call qs.sync() here: it is a synchronous registry flush on
        // Windows and blocks the main thread before the window is shown.
        // QSettings flushes automatically when the object goes out of scope.
        m_current = defaults;
        return m_current;
    }
    if (storedVersion < CURRENT_SCHEMA_VERSION) {
        migrate(storedVersion);
    }

    readFromQSettings();
    enforceLocked();
    return m_current;
}

// ---------------------------------------------------------------------------
// Save
// ---------------------------------------------------------------------------
bool SettingsManager::save(const AppSettings& settings)
{
    AppSettings s = settings;
    // Always enforce locked values regardless of what was passed in.
    s.security.requirePasswordForSigning = true;
    s.security.signingAlgorithm          = QStringLiteral("SHA-256");
    s.security.certificateValidation     = true;

    writeToQSettings(s);
    m_current = s;

    QSettings qs;
    qs.sync();

    emit settingsApplied(m_current);
    return true;
}

// ---------------------------------------------------------------------------
// value / setValue
// ---------------------------------------------------------------------------
QVariant SettingsManager::value(const QString& key, const QVariant& defaultValue) const
{
    QSettings qs;
    return qs.value(key, defaultValue);
}

bool SettingsManager::setValue(const QString& key, const QVariant& val)
{
    if (isLocked(key)) {
        m_lastError = QStringLiteral("'%1' is a locked security setting and cannot be changed.").arg(key);
        return false;
    }
    if (!validate(key, val)) {
        return false;  // m_lastError already set
    }

    QSettings qs;
    qs.setValue(key, val);
    qs.sync();

    // Keep m_current in sync for the modified key.
    readFromQSettings();

    emit settingChanged(key, val);
    emit settingsApplied(m_current);
    return true;
}

// ---------------------------------------------------------------------------
// resetToDefaults
// ---------------------------------------------------------------------------
void SettingsManager::resetToDefaults()
{
    AppSettings defaults;
    writeToQSettings(defaults);
    QSettings qs;
    qs.sync();
    m_current = defaults;
    emit settingsApplied(m_current);
}

// ---------------------------------------------------------------------------
// exportToJson
// ---------------------------------------------------------------------------
bool SettingsManager::exportToJson(const QString& filePath) const
{
    QJsonObject root;
    root[QStringLiteral("schemaVersion")] = CURRENT_SCHEMA_VERSION;

    const AppSettings& s = m_current;

    QJsonObject appearance;
    appearance[QStringLiteral("theme")]          = s.appearance.theme;
    appearance[QStringLiteral("accentColor")]    = s.appearance.accentColor;
    appearance[QStringLiteral("fontFamily")]     = s.appearance.fontFamily;
    appearance[QStringLiteral("fontSize")]       = s.appearance.fontSize;
    appearance[QStringLiteral("showAnimations")] = s.appearance.showAnimations;
    root[QStringLiteral("appearance")] = appearance;

    QJsonObject general;
    general[QStringLiteral("reopenTabsOnStartup")] = s.general.reopenTabsOnStartup;
    general[QStringLiteral("homeStartPage")]        = s.general.homeStartPage;
    general[QStringLiteral("tabOpenBehavior")]      = s.general.tabOpenBehavior;
    root[QStringLiteral("general")] = general;

    QJsonObject viewing;
    viewing[QStringLiteral("defaultZoom")]    = s.viewing.defaultZoom;
    viewing[QStringLiteral("showThumbnails")] = s.viewing.showThumbnails;
    viewing[QStringLiteral("cacheEnabled")]   = s.viewing.cacheEnabled;
    viewing[QStringLiteral("cacheSizeMB")]    = s.viewing.cacheSizeMB;
    viewing[QStringLiteral("quality")]        = s.viewing.quality;
    root[QStringLiteral("viewing")] = viewing;

    // Security: export only the user-editable certificatePath, not locked keys.
    QJsonObject security;
    security[QStringLiteral("certificatePath")] = s.security.certificatePath;
    root[QStringLiteral("security")] = security;

    QJsonObject advanced;
    advanced[QStringLiteral("enableLogging")]        = s.advanced.enableLogging;
    advanced[QStringLiteral("logLevel")]             = s.advanced.logLevel;
    advanced[QStringLiteral("maxWorkerThreads")]     = s.advanced.maxWorkerThreads;
    advanced[QStringLiteral("experimentalFeatures")] = s.advanced.experimentalFeatures;
    root[QStringLiteral("advanced")] = advanced;

    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_lastError = QStringLiteral("Cannot open file for writing: %1").arg(filePath);
        return false;
    }
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
}

// ---------------------------------------------------------------------------
// importFromJson
// ---------------------------------------------------------------------------
bool SettingsManager::importFromJson(const QString& filePath, QStringList& ignoredKeys)
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_lastError = QStringLiteral("Cannot open file for reading: %1").arg(filePath);
        return false;
    }
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &parseError);
    if (doc.isNull()) {
        m_lastError = QStringLiteral("JSON parse error: %1").arg(parseError.errorString());
        return false;
    }
    if (!doc.isObject()) {
        m_lastError = QStringLiteral("Invalid settings file format.");
        return false;
    }

    QJsonObject root = doc.object();
    AppSettings s = m_current; // Start from current to preserve anything not in file.

    // Helper lambda to check and log locked key attempts.
    auto warnLocked = [&](const QString& key) {
        ignoredKeys.append(key);
        qWarning() << "SettingsManager: import ignored locked key" << key;
    };

    if (root.contains(QStringLiteral("appearance"))) {
        QJsonObject ap = root[QStringLiteral("appearance")].toObject();
        if (ap.contains(QStringLiteral("theme")))          s.appearance.theme = ap[QStringLiteral("theme")].toString();
        if (ap.contains(QStringLiteral("accentColor")))    s.appearance.accentColor = ap[QStringLiteral("accentColor")].toString();
        if (ap.contains(QStringLiteral("fontFamily")))     s.appearance.fontFamily = ap[QStringLiteral("fontFamily")].toString();
        if (ap.contains(QStringLiteral("fontSize")))       s.appearance.fontSize = ap[QStringLiteral("fontSize")].toInt(12);
        if (ap.contains(QStringLiteral("showAnimations"))) s.appearance.showAnimations = ap[QStringLiteral("showAnimations")].toBool();
    }

    if (root.contains(QStringLiteral("general"))) {
        QJsonObject ge = root[QStringLiteral("general")].toObject();
        if (ge.contains(QStringLiteral("reopenTabsOnStartup"))) s.general.reopenTabsOnStartup = ge[QStringLiteral("reopenTabsOnStartup")].toBool();
        if (ge.contains(QStringLiteral("homeStartPage")))       s.general.homeStartPage = ge[QStringLiteral("homeStartPage")].toString();
        if (ge.contains(QStringLiteral("tabOpenBehavior")))     s.general.tabOpenBehavior = ge[QStringLiteral("tabOpenBehavior")].toString();
    }

    if (root.contains(QStringLiteral("viewing"))) {
        QJsonObject vi = root[QStringLiteral("viewing")].toObject();
        if (vi.contains(QStringLiteral("defaultZoom")))    s.viewing.defaultZoom = vi[QStringLiteral("defaultZoom")].toString();
        if (vi.contains(QStringLiteral("showThumbnails"))) s.viewing.showThumbnails = vi[QStringLiteral("showThumbnails")].toBool();
        if (vi.contains(QStringLiteral("cacheEnabled")))   s.viewing.cacheEnabled = vi[QStringLiteral("cacheEnabled")].toBool();
        if (vi.contains(QStringLiteral("cacheSizeMB")))    s.viewing.cacheSizeMB = vi[QStringLiteral("cacheSizeMB")].toInt(200);
        if (vi.contains(QStringLiteral("quality")))        s.viewing.quality = vi[QStringLiteral("quality")].toString();
    }

    if (root.contains(QStringLiteral("security"))) {
        QJsonObject se = root[QStringLiteral("security")].toObject();
        // Warn and ignore any attempt to set locked security keys.
        if (se.contains(QStringLiteral("requirePasswordForSigning"))) warnLocked(QStringLiteral("security/requirePasswordForSigning"));
        if (se.contains(QStringLiteral("signingAlgorithm")))          warnLocked(QStringLiteral("security/signingAlgorithm"));
        if (se.contains(QStringLiteral("certificateValidation")))     warnLocked(QStringLiteral("security/certificateValidation"));
        // Allow the non-locked key.
        if (se.contains(QStringLiteral("certificatePath"))) s.security.certificatePath = se[QStringLiteral("certificatePath")].toString();
    }

    if (root.contains(QStringLiteral("advanced"))) {
        QJsonObject ad = root[QStringLiteral("advanced")].toObject();
        if (ad.contains(QStringLiteral("enableLogging")))        s.advanced.enableLogging = ad[QStringLiteral("enableLogging")].toBool();
        if (ad.contains(QStringLiteral("logLevel")))             s.advanced.logLevel = ad[QStringLiteral("logLevel")].toString();
        if (ad.contains(QStringLiteral("maxWorkerThreads")))     s.advanced.maxWorkerThreads = ad[QStringLiteral("maxWorkerThreads")].toInt(4);
        if (ad.contains(QStringLiteral("experimentalFeatures"))) s.advanced.experimentalFeatures = ad[QStringLiteral("experimentalFeatures")].toBool();
    }

    // Validate the combined result before saving.
    if (s.appearance.fontSize < 10 || s.appearance.fontSize > 20) s.appearance.fontSize = 12;
    if (s.viewing.cacheSizeMB < 50 || s.viewing.cacheSizeMB > 500) s.viewing.cacheSizeMB = 200;
    if (s.advanced.maxWorkerThreads < 1 || s.advanced.maxWorkerThreads > 8) s.advanced.maxWorkerThreads = 4;
    if (s.general.homeStartPage != QStringLiteral("recent") && s.general.homeStartPage != QStringLiteral("browse"))
        s.general.homeStartPage = QStringLiteral("recent");

    return save(s);
}

// ---------------------------------------------------------------------------
// isLocked
// ---------------------------------------------------------------------------
bool SettingsManager::isLocked(const QString& key) const
{
    return LOCKED_KEYS.contains(key);
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------
bool SettingsManager::validate(const QString& key, const QVariant& value)
{
    if (key == QStringLiteral("appearance/fontSize")) {
        int v = value.toInt();
        if (v < 10 || v > 20) {
            m_lastError = QStringLiteral("Font size must be between 10 and 20 pt. Got: %1").arg(v);
            return false;
        }
    } else if (key == QStringLiteral("viewing/cacheSizeMB")) {
        int v = value.toInt();
        if (v < 50 || v > 500) {
            m_lastError = QStringLiteral("Cache size must be between 50 and 500 MB. Got: %1").arg(v);
            return false;
        }
    } else if (key == QStringLiteral("advanced/maxWorkerThreads")) {
        int v = value.toInt();
        if (v < 1 || v > 8) {
            m_lastError = QStringLiteral("Worker threads must be between 1 and 8. Got: %1").arg(v);
            return false;
        }
    } else if (key == QStringLiteral("general/homeStartPage")) {
        QString v = value.toString();
        if (v != QStringLiteral("recent") && v != QStringLiteral("browse")) {
            m_lastError = QStringLiteral("Home start page must be 'recent' or 'browse'. Got: '%1'").arg(v);
            return false;
        }
    } else if (key == QStringLiteral("appearance/theme")) {
        QString v = value.toString();
        if (v != QStringLiteral("dark") && v != QStringLiteral("light") && v != QStringLiteral("system")) {
            m_lastError = QStringLiteral("Theme must be 'dark', 'light', or 'system'. Got: '%1'").arg(v);
            return false;
        }
    }
    return true;
}

void SettingsManager::enforceLocked()
{
    m_current.security.requirePasswordForSigning = true;
    m_current.security.signingAlgorithm          = QStringLiteral("SHA-256");
    m_current.security.certificateValidation     = true;

    QSettings qs;
    qs.setValue(QStringLiteral("security/requirePasswordForSigning"), true);
    qs.setValue(QStringLiteral("security/signingAlgorithm"), QStringLiteral("SHA-256"));
    qs.setValue(QStringLiteral("security/certificateValidation"), true);
}

void SettingsManager::migrate(int fromVersion)
{
    // Architecture stub: add actual key migrations here as the schema evolves.
    // Example:
    //   if (fromVersion < 2) { /* rename old keys */ }
    QSettings qs;
    qs.setValue(QStringLiteral("settings/schemaVersion"), CURRENT_SCHEMA_VERSION);
    qs.sync();
    Q_UNUSED(fromVersion)
}

void SettingsManager::readFromQSettings()
{
    QSettings qs;

    m_current.schemaVersion = qs.value(QStringLiteral("settings/schemaVersion"), 1).toInt();

    m_current.appearance.theme          = qs.value(QStringLiteral("appearance/theme"),          QStringLiteral("dark")).toString();
    m_current.appearance.accentColor    = qs.value(QStringLiteral("appearance/accentColor"),    QStringLiteral("#00bfff")).toString();
    m_current.appearance.fontFamily     = qs.value(QStringLiteral("appearance/fontFamily"),     QStringLiteral("Segoe UI")).toString();
    m_current.appearance.fontSize       = qs.value(QStringLiteral("appearance/fontSize"),       12).toInt();
    m_current.appearance.showAnimations = qs.value(QStringLiteral("appearance/showAnimations"), true).toBool();

    m_current.general.reopenTabsOnStartup = qs.value(QStringLiteral("general/reopenTabsOnStartup"), true).toBool();
    m_current.general.homeStartPage       = qs.value(QStringLiteral("general/homeStartPage"),        QStringLiteral("recent")).toString();
    m_current.general.tabOpenBehavior     = qs.value(QStringLiteral("general/tabOpenBehavior"),      QStringLiteral("new")).toString();

    m_current.viewing.defaultZoom    = qs.value(QStringLiteral("viewing/defaultZoom"),    QStringLiteral("fit-width")).toString();
    m_current.viewing.showThumbnails = qs.value(QStringLiteral("viewing/showThumbnails"), true).toBool();
    m_current.viewing.cacheEnabled   = qs.value(QStringLiteral("viewing/cacheEnabled"),   true).toBool();
    m_current.viewing.cacheSizeMB    = qs.value(QStringLiteral("viewing/cacheSizeMB"),    200).toInt();
    m_current.viewing.quality        = qs.value(QStringLiteral("viewing/quality"),        QStringLiteral("balanced")).toString();

    // Locked – read from QSettings only to initialise, will be overwritten by enforceLocked().
    m_current.security.requirePasswordForSigning = true;
    m_current.security.signingAlgorithm          = QStringLiteral("SHA-256");
    m_current.security.certificateValidation     = true;
    m_current.security.certificatePath           = qs.value(QStringLiteral("security/certificatePath"), QString()).toString();

    m_current.advanced.enableLogging        = qs.value(QStringLiteral("advanced/enableLogging"),        false).toBool();
    m_current.advanced.logLevel             = qs.value(QStringLiteral("advanced/logLevel"),             QStringLiteral("info")).toString();
    m_current.advanced.maxWorkerThreads     = qs.value(QStringLiteral("advanced/maxWorkerThreads"),     4).toInt();
    m_current.advanced.experimentalFeatures = qs.value(QStringLiteral("advanced/experimentalFeatures"), false).toBool();
}

void SettingsManager::writeToQSettings(const AppSettings& s)
{
    QSettings qs;

    qs.setValue(QStringLiteral("settings/schemaVersion"),       s.schemaVersion);

    qs.setValue(QStringLiteral("appearance/theme"),             s.appearance.theme);
    qs.setValue(QStringLiteral("appearance/accentColor"),       s.appearance.accentColor);
    qs.setValue(QStringLiteral("appearance/fontFamily"),        s.appearance.fontFamily);
    qs.setValue(QStringLiteral("appearance/fontSize"),          s.appearance.fontSize);
    qs.setValue(QStringLiteral("appearance/showAnimations"),    s.appearance.showAnimations);

    qs.setValue(QStringLiteral("general/reopenTabsOnStartup"),  s.general.reopenTabsOnStartup);
    qs.setValue(QStringLiteral("general/homeStartPage"),        s.general.homeStartPage);
    qs.setValue(QStringLiteral("general/tabOpenBehavior"),      s.general.tabOpenBehavior);

    qs.setValue(QStringLiteral("viewing/defaultZoom"),          s.viewing.defaultZoom);
    qs.setValue(QStringLiteral("viewing/showThumbnails"),       s.viewing.showThumbnails);
    qs.setValue(QStringLiteral("viewing/cacheEnabled"),         s.viewing.cacheEnabled);
    qs.setValue(QStringLiteral("viewing/cacheSizeMB"),          s.viewing.cacheSizeMB);
    qs.setValue(QStringLiteral("viewing/quality"),              s.viewing.quality);

    // Locked values are always written with enforced constants.
    qs.setValue(QStringLiteral("security/requirePasswordForSigning"), true);
    qs.setValue(QStringLiteral("security/signingAlgorithm"),          QStringLiteral("SHA-256"));
    qs.setValue(QStringLiteral("security/certificateValidation"),     true);
    qs.setValue(QStringLiteral("security/certificatePath"),           s.security.certificatePath);

    qs.setValue(QStringLiteral("advanced/enableLogging"),        s.advanced.enableLogging);
    qs.setValue(QStringLiteral("advanced/logLevel"),             s.advanced.logLevel);
    qs.setValue(QStringLiteral("advanced/maxWorkerThreads"),     s.advanced.maxWorkerThreads);
    qs.setValue(QStringLiteral("advanced/experimentalFeatures"), s.advanced.experimentalFeatures);
}
