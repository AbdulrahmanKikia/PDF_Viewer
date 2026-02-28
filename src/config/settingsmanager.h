// settingsmanager.h
// Central settings system for PDFViewer.
//
// KEY NAMING SCHEME (QSettings group/key):
//   "settings/schemaVersion"
//   "appearance/theme"           - "dark" | "light" | "system"
//   "appearance/accentColor"     - "#RRGGBB"
//   "appearance/fontFamily"      - e.g. "Segoe UI"
//   "appearance/fontSize"        - int 10-20
//   "appearance/showAnimations"  - bool
//   "general/reopenTabsOnStartup"- bool
//   "general/homeStartPage"      - "recent" | "browse"
//   "general/tabOpenBehavior"    - "new" | "replace" | "ask"
//   "viewing/defaultZoom"        - "fit-width" | "fit-page" | "100" (percent string)
//   "viewing/showThumbnails"     - bool
//   "viewing/cacheEnabled"       - bool
//   "viewing/cacheSizeMB"        - int 50-500
//   "viewing/quality"            - "fast" | "balanced" | "best"
//   "security/requirePasswordForSigning" - LOCKED: always true
//   "security/signingAlgorithm"          - LOCKED: always "SHA-256"
//   "security/certificateValidation"     - LOCKED: always true
//   "security/certificatePath"           - QString (user-editable)
//   "advanced/enableLogging"     - bool
//   "advanced/logLevel"          - "info" | "debug" | "warning" | "error"
//   "advanced/maxWorkerThreads"  - int 1-8
//   "advanced/experimentalFeatures" - bool
//
// LOCKED POLICY:
//   Keys in the LOCKED_KEYS set cannot be changed via setValue() or importFromJson().
//   They are always written with their enforced values on save.

#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include <QObject>
#include <QString>
#include <QVariant>
#include <QSet>

// ---------------------------------------------------------------------------
// AppSettings – typed in-memory representation of all application settings
// ---------------------------------------------------------------------------
struct AppearanceSettings {
    QString theme          = QStringLiteral("dark");      // "dark"|"light"|"system"
    QString accentColor    = QStringLiteral("#00bfff");
    QString fontFamily     = QStringLiteral("Segoe UI");
    int     fontSize       = 12;                          // 10-20 pt
    bool    showAnimations = true;
};

struct GeneralSettings {
    bool    reopenTabsOnStartup = true;
    QString homeStartPage       = QStringLiteral("recent"); // "recent"|"browse"
    QString tabOpenBehavior     = QStringLiteral("new");    // "new"|"replace"|"ask"
};

struct ViewingSettings {
    QString defaultZoom    = QStringLiteral("fit-width"); // "fit-width"|"fit-page"|"<percent>"
    bool    showThumbnails = true;
    bool    cacheEnabled   = true;
    int     cacheSizeMB    = 200;                         // 50-500
    QString quality        = QStringLiteral("balanced");  // "fast"|"balanced"|"best"
};

struct SecuritySettings {
    // LOCKED values – never changeable through UI or import
    bool    requirePasswordForSigning = true;   // LOCKED
    QString signingAlgorithm          = QStringLiteral("SHA-256"); // LOCKED
    bool    certificateValidation     = true;   // LOCKED
    // User-editable
    QString certificatePath;
};

struct AdvancedSettings {
    bool    enableLogging         = false;
    QString logLevel              = QStringLiteral("info"); // "info"|"debug"|"warning"|"error"
    int     maxWorkerThreads      = 4;                      // 1-8
    bool    experimentalFeatures  = false;
};

struct AppSettings {
    int                schemaVersion = 1;
    AppearanceSettings appearance;
    GeneralSettings    general;
    ViewingSettings    viewing;
    SecuritySettings   security;
    AdvancedSettings   advanced;
};

// ---------------------------------------------------------------------------
// SettingsManager – QSettings wrapper with validation, locking, and migration
// ---------------------------------------------------------------------------
class SettingsManager : public QObject
{
    Q_OBJECT

public:
    static SettingsManager* instance();

    // Load persisted settings into memory; applies defaults for missing keys.
    // Runs schema migration if schemaVersion is outdated.
    AppSettings load();

    // Persist the current in-memory settings struct to QSettings.
    bool save(const AppSettings& settings);

    // Type-safe single-key get with a default (reads from QSettings directly).
    QVariant value(const QString& key, const QVariant& defaultValue = QVariant()) const;

    // Single-key set. Returns false and sets lastError() if key is locked or value invalid.
    bool setValue(const QString& key, const QVariant& value);

    // Reset all settings to defaults (does not touch locked keys – they stay enforced).
    void resetToDefaults();

    // Export all non-locked settings to a JSON file.
    bool exportToJson(const QString& filePath) const;

    // Import settings from a JSON file. Locked keys are silently ignored.
    // Returns false and sets lastError() if file is invalid or migration fails.
    bool importFromJson(const QString& filePath, QStringList& ignoredKeys);

    // True if key is in the locked set (security keys).
    bool isLocked(const QString& key) const;

    // Current schema version constant (useful for export/import callers).
    static int currentSchemaVersion() { return CURRENT_SCHEMA_VERSION; }

    // Human-readable description of the last validation error.
    QString lastError() const { return m_lastError; }

    // Current in-memory settings (fast access without re-reading QSettings).
    const AppSettings& current() const { return m_current; }

signals:
    // Emitted after save() or setValue() succeeds with the full updated struct.
    void settingsApplied(const AppSettings& settings);

    // Emitted for a single key change (for lightweight listeners).
    void settingChanged(const QString& key, const QVariant& value);

private:
    explicit SettingsManager(QObject* parent = nullptr);
    static SettingsManager* s_instance;

    AppSettings m_current;
    mutable QString m_lastError;

    // Validation helpers. Return false and populate m_lastError on failure.
    bool validate(const QString& key, const QVariant& value);

    // Write all enforced locked values unconditionally.
    void enforceLocked();

    // Schema migration: called when loaded schemaVersion < current version.
    void migrate(int fromVersion);

    // Populate m_current from QSettings.
    void readFromQSettings();

    // Write m_current to QSettings.
    void writeToQSettings(const AppSettings& s);

    static const QSet<QString> LOCKED_KEYS;
    static const int CURRENT_SCHEMA_VERSION;
};

#endif // SETTINGSMANAGER_H
