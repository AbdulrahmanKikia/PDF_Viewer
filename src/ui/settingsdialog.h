// settingsdialog.h
// Settings dialog: sidebar category list + stacked pages + search bar.
//
// Layout:
//   ┌─ Search bar ──────────────────────────────────┐
//   ├─ Category list ─┬─ Page stack ─────────────────┤
//   │  Appearance     │  (Appearance / General / ...) │
//   │  General        │                               │
//   │  Viewing        │                               │
//   │  Security       │                               │
//   │  Advanced       │                               │
//   ├─────────────────┴───────────────────────────────┤
//   │  [Apply] [OK] [Cancel] [Restore Defaults]        │
//   └──────────────────────────────────────────────────┘

#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include "settingsmanager.h"
#include <QDialog>

class QListWidget;
class QStackedWidget;
class QLineEdit;
class QDialogButtonBox;
class QPushButton;
class QAbstractButton;

// Forward declarations for page widgets (defined in this same .h for simplicity).
class AppearancePage;
class GeneralPage;
class ViewingPage;
class SecurityPage;
class AdvancedPage;

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget* parent = nullptr);

private slots:
    void onCategoryChanged(int row);
    void onSearchTextChanged(const QString& text);
    void onButtonClicked(QAbstractButton* button);
    void onApply();
    void onRestoreDefaults();
    void onExportSettings();
    void onImportSettings();

private:
    void setupUI();
    void loadSettingsIntoUI(const AppSettings& s);
    AppSettings collectSettingsFromUI() const;
    bool applyAndValidate();

    QListWidget*      m_categoryList  = nullptr;
    QStackedWidget*   m_pageStack     = nullptr;
    QLineEdit*        m_searchEdit    = nullptr;
    QDialogButtonBox* m_buttonBox     = nullptr;
    QPushButton*      m_restoreBtn    = nullptr;
    QPushButton*      m_exportBtn     = nullptr;
    QPushButton*      m_importBtn     = nullptr;

    AppearancePage* m_appearancePage = nullptr;
    GeneralPage*    m_generalPage    = nullptr;
    ViewingPage*    m_viewingPage    = nullptr;
    SecurityPage*   m_securityPage   = nullptr;
    AdvancedPage*   m_advancedPage   = nullptr;

    // Copy of settings as they were when dialog was opened (used for Cancel).
    AppSettings m_originalSettings;
};

// ---------------------------------------------------------------------------
// Page base – each category page is a QWidget with load/collect helpers.
// ---------------------------------------------------------------------------
class QCheckBox;
class QComboBox;
class QSpinBox;
class QSlider;
class QLabel;
class QGroupBox;

// ---- Appearance ----
class AppearancePage : public QWidget
{
    Q_OBJECT
public:
    explicit AppearancePage(QWidget* parent = nullptr);
    void load(const AppearanceSettings& s);
    AppearanceSettings collect() const;

private:
    QComboBox* m_themeCombo       = nullptr;
    QComboBox* m_fontFamilyCombo  = nullptr;
    QSpinBox*  m_fontSizeSpin     = nullptr;
    QCheckBox* m_animationsCheck  = nullptr;
    QLabel*    m_previewLabel     = nullptr;

    void updatePreview();
};

// ---- General ----
class GeneralPage : public QWidget
{
    Q_OBJECT
public:
    explicit GeneralPage(QWidget* parent = nullptr);
    void load(const GeneralSettings& s);
    GeneralSettings collect() const;

private:
    QCheckBox* m_reopenTabsCheck  = nullptr;
    QComboBox* m_homeStartCombo   = nullptr;
    QComboBox* m_tabBehaviorCombo = nullptr;
};

// ---- Viewing ----
class ViewingPage : public QWidget
{
    Q_OBJECT
public:
    explicit ViewingPage(QWidget* parent = nullptr);
    void load(const ViewingSettings& s);
    ViewingSettings collect() const;

private:
    QComboBox* m_zoomCombo         = nullptr;
    QCheckBox* m_thumbnailsCheck   = nullptr;
    QCheckBox* m_cacheCheck        = nullptr;
    QSlider*   m_cacheSlider       = nullptr;
    QLabel*    m_cacheSizeLabel    = nullptr;
    QComboBox* m_qualityCombo      = nullptr;
};

// ---- Security ----
class SecurityPage : public QWidget
{
    Q_OBJECT
public:
    explicit SecurityPage(QWidget* parent = nullptr);
    void load(const SecuritySettings& s);
    SecuritySettings collect() const;

private:
    QLineEdit* m_certPathEdit = nullptr;
};

// ---- Advanced ----
class AdvancedPage : public QWidget
{
    Q_OBJECT
public:
    explicit AdvancedPage(QWidget* parent = nullptr);
    void load(const AdvancedSettings& s);
    AdvancedSettings collect() const;

private:
    QCheckBox* m_loggingCheck      = nullptr;
    QComboBox* m_logLevelCombo     = nullptr;
    QSlider*   m_threadSlider      = nullptr;
    QLabel*    m_threadLabel       = nullptr;
    QCheckBox* m_experimentalCheck = nullptr;
};

#endif // SETTINGSDIALOG_H
