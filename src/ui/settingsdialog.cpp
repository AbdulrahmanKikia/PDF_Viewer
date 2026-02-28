// settingsdialog.cpp
#include "settingsdialog.h"
#include "../config/settingsmanager.h"
#include "../config/theme.h"

#include <QListWidget>
#include <QStackedWidget>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QAbstractButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QSlider>
#include <QFrame>
#include <QFileDialog>
#include <QMessageBox>
#include <QFontDatabase>
#include <QFont>
#include <QSplitter>

// ============================================================================
// AppearancePage
// ============================================================================
AppearancePage::AppearancePage(QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(16);

    QGroupBox* themeGroup = new QGroupBox(tr("Theme"), this);
    QFormLayout* themeForm = new QFormLayout(themeGroup);
    m_themeCombo = new QComboBox(this);
    m_themeCombo->addItem(tr("Dark"),   QStringLiteral("dark"));
    m_themeCombo->addItem(tr("Light"),  QStringLiteral("light"));
    m_themeCombo->addItem(tr("System"), QStringLiteral("system"));
    themeForm->addRow(tr("Theme:"), m_themeCombo);
    layout->addWidget(themeGroup);

    QGroupBox* fontGroup = new QGroupBox(tr("Font"), this);
    QFormLayout* fontForm = new QFormLayout(fontGroup);
    m_fontFamilyCombo = new QComboBox(this);
    // Populate with common readable fonts always present on Windows.
    const QStringList fonts = {
        QStringLiteral("Segoe UI"), QStringLiteral("Arial"),
        QStringLiteral("Calibri"),  QStringLiteral("Consolas"),
        QStringLiteral("Tahoma"),   QStringLiteral("Verdana")
    };
    for (const QString& f : fonts)
        m_fontFamilyCombo->addItem(f, f);

    m_fontSizeSpin = new QSpinBox(this);
    m_fontSizeSpin->setRange(10, 20);
    m_fontSizeSpin->setSuffix(tr(" pt"));
    m_fontSizeSpin->setToolTip(tr("Font size must be between 10 and 20 pt."));

    fontForm->addRow(tr("Font family:"), m_fontFamilyCombo);
    fontForm->addRow(tr("Font size:"),   m_fontSizeSpin);
    layout->addWidget(fontGroup);

    QGroupBox* uiGroup = new QGroupBox(tr("UI Elements"), this);
    QVBoxLayout* uiLayout = new QVBoxLayout(uiGroup);
    m_animationsCheck = new QCheckBox(tr("Show animations"), this);
    uiLayout->addWidget(m_animationsCheck);
    layout->addWidget(uiGroup);

    m_previewLabel = new QLabel(tr("Preview text: The quick brown fox"), this);
    m_previewLabel->setObjectName(QStringLiteral("SettingsPreviewLabel"));
    m_previewLabel->setFrameShape(QFrame::StyledPanel);
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setMinimumHeight(48);
    layout->addWidget(m_previewLabel);

    layout->addStretch();

    connect(m_fontFamilyCombo, &QComboBox::currentIndexChanged, this, &AppearancePage::updatePreview);
    connect(m_fontSizeSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &AppearancePage::updatePreview);
}

void AppearancePage::load(const AppearanceSettings& s)
{
    int themeIdx = m_themeCombo->findData(s.theme);
    m_themeCombo->setCurrentIndex(themeIdx >= 0 ? themeIdx : 0);

    int fontIdx = m_fontFamilyCombo->findData(s.fontFamily);
    m_fontFamilyCombo->setCurrentIndex(fontIdx >= 0 ? fontIdx : 0);

    m_fontSizeSpin->setValue(s.fontSize);
    m_animationsCheck->setChecked(s.showAnimations);
    updatePreview();
}

AppearanceSettings AppearancePage::collect() const
{
    AppearanceSettings s;
    s.theme          = m_themeCombo->currentData().toString();
    s.fontFamily     = m_fontFamilyCombo->currentData().toString();
    s.fontSize       = m_fontSizeSpin->value();
    s.showAnimations = m_animationsCheck->isChecked();
    // accentColor: not yet implemented in UI, keep default.
    s.accentColor    = QStringLiteral("#00bfff");
    return s;
}

void AppearancePage::updatePreview()
{
    if (!m_previewLabel) return;
    QFont font(m_fontFamilyCombo->currentText(), m_fontSizeSpin->value());
    m_previewLabel->setFont(font);
}

// ============================================================================
// GeneralPage
// ============================================================================
GeneralPage::GeneralPage(QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(16);

    QGroupBox* sessionGroup = new QGroupBox(tr("Session Management"), this);
    QVBoxLayout* sessionLayout = new QVBoxLayout(sessionGroup);
    m_reopenTabsCheck = new QCheckBox(tr("Reopen tabs from last session on startup"), this);
    sessionLayout->addWidget(m_reopenTabsCheck);
    layout->addWidget(sessionGroup);

    QGroupBox* homeGroup = new QGroupBox(tr("Home Page"), this);
    QFormLayout* homeForm = new QFormLayout(homeGroup);
    m_homeStartCombo = new QComboBox(this);
    m_homeStartCombo->addItem(tr("Recent files"), QStringLiteral("recent"));
    m_homeStartCombo->addItem(tr("File browser"), QStringLiteral("browse"));
    homeForm->addRow(tr("Default home view:"), m_homeStartCombo);
    layout->addWidget(homeGroup);

    QGroupBox* tabGroup = new QGroupBox(tr("Tab Behavior"), this);
    QFormLayout* tabForm = new QFormLayout(tabGroup);
    m_tabBehaviorCombo = new QComboBox(this);
    m_tabBehaviorCombo->addItem(tr("Open in new tab"),         QStringLiteral("new"));
    m_tabBehaviorCombo->addItem(tr("Replace current tab"),     QStringLiteral("replace"));
    m_tabBehaviorCombo->addItem(tr("Ask me each time"),        QStringLiteral("ask"));
    tabForm->addRow(tr("Opening a file:"), m_tabBehaviorCombo);
    layout->addWidget(tabGroup);

    layout->addStretch();
}

void GeneralPage::load(const GeneralSettings& s)
{
    m_reopenTabsCheck->setChecked(s.reopenTabsOnStartup);

    int homeIdx = m_homeStartCombo->findData(s.homeStartPage);
    m_homeStartCombo->setCurrentIndex(homeIdx >= 0 ? homeIdx : 0);

    int tabIdx = m_tabBehaviorCombo->findData(s.tabOpenBehavior);
    m_tabBehaviorCombo->setCurrentIndex(tabIdx >= 0 ? tabIdx : 0);
}

GeneralSettings GeneralPage::collect() const
{
    GeneralSettings s;
    s.reopenTabsOnStartup = m_reopenTabsCheck->isChecked();
    s.homeStartPage       = m_homeStartCombo->currentData().toString();
    s.tabOpenBehavior     = m_tabBehaviorCombo->currentData().toString();
    return s;
}

// ============================================================================
// ViewingPage
// ============================================================================
ViewingPage::ViewingPage(QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(16);

    QGroupBox* displayGroup = new QGroupBox(tr("Display"), this);
    QFormLayout* displayForm = new QFormLayout(displayGroup);
    m_zoomCombo = new QComboBox(this);
    m_zoomCombo->addItem(tr("Fit width"),  QStringLiteral("fit-width"));
    m_zoomCombo->addItem(tr("Fit page"),   QStringLiteral("fit-page"));
    m_zoomCombo->addItem(tr("75%"),        QStringLiteral("75"));
    m_zoomCombo->addItem(tr("100%"),       QStringLiteral("100"));
    m_zoomCombo->addItem(tr("125%"),       QStringLiteral("125"));
    m_zoomCombo->addItem(tr("150%"),       QStringLiteral("150"));
    displayForm->addRow(tr("Default zoom:"), m_zoomCombo);
    m_thumbnailsCheck = new QCheckBox(tr("Show thumbnails sidebar"), this);
    displayForm->addRow(m_thumbnailsCheck);
    layout->addWidget(displayGroup);

    QGroupBox* renderGroup = new QGroupBox(tr("Rendering"), this);
    QFormLayout* renderForm = new QFormLayout(renderGroup);
    m_qualityCombo = new QComboBox(this);
    m_qualityCombo->addItem(tr("Fast (best performance)"), QStringLiteral("fast"));
    m_qualityCombo->addItem(tr("Balanced (default)"),      QStringLiteral("balanced"));
    m_qualityCombo->addItem(tr("Best quality (slower)"),   QStringLiteral("best"));
    renderForm->addRow(tr("Rendering quality:"), m_qualityCombo);
    layout->addWidget(renderGroup);

    QGroupBox* cacheGroup = new QGroupBox(tr("Page Cache"), this);
    QVBoxLayout* cacheVLayout = new QVBoxLayout(cacheGroup);
    m_cacheCheck = new QCheckBox(tr("Enable page caching"), this);
    cacheVLayout->addWidget(m_cacheCheck);

    QHBoxLayout* sliderLayout = new QHBoxLayout;
    QLabel* minLabel = new QLabel(tr("50 MB"), this);
    QLabel* maxLabel = new QLabel(tr("500 MB"), this);
    m_cacheSlider = new QSlider(Qt::Horizontal, this);
    m_cacheSlider->setRange(50, 500);
    m_cacheSlider->setSingleStep(50);
    m_cacheSlider->setPageStep(50);
    m_cacheSlider->setToolTip(tr("Cache size in MB (50–500)"));
    m_cacheSizeLabel = new QLabel(tr("200 MB"), this);
    m_cacheSizeLabel->setMinimumWidth(55);

    sliderLayout->addWidget(minLabel);
    sliderLayout->addWidget(m_cacheSlider);
    sliderLayout->addWidget(maxLabel);
    sliderLayout->addWidget(m_cacheSizeLabel);
    cacheVLayout->addLayout(sliderLayout);
    layout->addWidget(cacheGroup);

    layout->addStretch();

    connect(m_cacheSlider, &QSlider::valueChanged, this, [this](int v) {
        m_cacheSizeLabel->setText(tr("%1 MB").arg(v));
    });
    connect(m_cacheCheck, &QCheckBox::toggled, m_cacheSlider, &QSlider::setEnabled);
}

void ViewingPage::load(const ViewingSettings& s)
{
    int zoomIdx = m_zoomCombo->findData(s.defaultZoom);
    m_zoomCombo->setCurrentIndex(zoomIdx >= 0 ? zoomIdx : 0);

    m_thumbnailsCheck->setChecked(s.showThumbnails);
    m_cacheCheck->setChecked(s.cacheEnabled);
    m_cacheSlider->setValue(s.cacheSizeMB);
    m_cacheSlider->setEnabled(s.cacheEnabled);
    m_cacheSizeLabel->setText(tr("%1 MB").arg(s.cacheSizeMB));

    int qualIdx = m_qualityCombo->findData(s.quality);
    m_qualityCombo->setCurrentIndex(qualIdx >= 0 ? qualIdx : 1);
}

ViewingSettings ViewingPage::collect() const
{
    ViewingSettings s;
    s.defaultZoom    = m_zoomCombo->currentData().toString();
    s.showThumbnails = m_thumbnailsCheck->isChecked();
    s.cacheEnabled   = m_cacheCheck->isChecked();
    s.cacheSizeMB    = m_cacheSlider->value();
    s.quality        = m_qualityCombo->currentData().toString();
    return s;
}

// ============================================================================
// SecurityPage
// ============================================================================
SecurityPage::SecurityPage(QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(16);

    // Locked settings banner
    QLabel* lockBanner = new QLabel(
        tr("\U0001F512  The following security settings are locked for your protection "
           "and cannot be changed."), this);
    lockBanner->setObjectName(QStringLiteral("SecurityLockBanner"));
    lockBanner->setWordWrap(true);
    lockBanner->setStyleSheet(QStringLiteral(
        "background: rgba(255,200,0,0.12); border: 1px solid rgba(255,200,0,0.5);"
        "border-radius: 4px; padding: 8px; color: #ffc800;"));
    layout->addWidget(lockBanner);

    QGroupBox* lockedGroup = new QGroupBox(tr("Locked Settings"), this);
    QFormLayout* lockedForm = new QFormLayout(lockedGroup);
    lockedGroup->setEnabled(false);  // Entire group disabled in UI

    auto makeLocked = [&](const QString& labelText, const QString& value) {
        QLineEdit* le = new QLineEdit(value, lockedGroup);
        le->setReadOnly(true);
        le->setToolTip(tr("This setting is always enabled for security and cannot be changed."));
        le->setStyleSheet(QStringLiteral("color: gray;"));
        lockedForm->addRow(labelText, le);
    };
    makeLocked(tr("Require password for signing:"), tr("Always enabled"));
    makeLocked(tr("Signing algorithm:"),             tr("SHA-256 (enforced)"));
    makeLocked(tr("Certificate validation:"),        tr("Always enabled"));
    layout->addWidget(lockedGroup);

    QGroupBox* userGroup = new QGroupBox(tr("Certificate Storage"), this);
    QFormLayout* userForm = new QFormLayout(userGroup);
    m_certPathEdit = new QLineEdit(this);
    m_certPathEdit->setPlaceholderText(tr("(default system store)"));
    m_certPathEdit->setToolTip(tr("Path to custom certificate store directory."));

    QPushButton* browseBtn = new QPushButton(tr("Browse…"), this);
    connect(browseBtn, &QPushButton::clicked, this, [this] {
        QString dir = QFileDialog::getExistingDirectory(
            this, tr("Select Certificate Directory"), m_certPathEdit->text());
        if (!dir.isEmpty()) m_certPathEdit->setText(dir);
    });

    QHBoxLayout* pathRow = new QHBoxLayout;
    pathRow->addWidget(m_certPathEdit);
    pathRow->addWidget(browseBtn);
    userForm->addRow(tr("Certificate path:"), pathRow);
    layout->addWidget(userGroup);

    layout->addStretch();
}

void SecurityPage::load(const SecuritySettings& s)
{
    m_certPathEdit->setText(s.certificatePath);
}

SecuritySettings SecurityPage::collect() const
{
    SecuritySettings s;
    // Locked values always enforced – never read from UI.
    s.requirePasswordForSigning = true;
    s.signingAlgorithm          = QStringLiteral("SHA-256");
    s.certificateValidation     = true;
    s.certificatePath           = m_certPathEdit->text();
    return s;
}

// ============================================================================
// AdvancedPage
// ============================================================================
AdvancedPage::AdvancedPage(QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(16);

    QGroupBox* logGroup = new QGroupBox(tr("Logging"), this);
    QVBoxLayout* logVLayout = new QVBoxLayout(logGroup);
    m_loggingCheck = new QCheckBox(tr("Enable application logging"), this);
    logVLayout->addWidget(m_loggingCheck);

    QFormLayout* logForm = new QFormLayout;
    m_logLevelCombo = new QComboBox(this);
    m_logLevelCombo->addItem(tr("Info"),    QStringLiteral("info"));
    m_logLevelCombo->addItem(tr("Debug"),   QStringLiteral("debug"));
    m_logLevelCombo->addItem(tr("Warning"), QStringLiteral("warning"));
    m_logLevelCombo->addItem(tr("Error"),   QStringLiteral("error"));
    logForm->addRow(tr("Log level:"), m_logLevelCombo);
    logVLayout->addLayout(logForm);
    layout->addWidget(logGroup);

    connect(m_loggingCheck, &QCheckBox::toggled, m_logLevelCombo, &QComboBox::setEnabled);

    QGroupBox* perfGroup = new QGroupBox(tr("Performance"), this);
    QVBoxLayout* perfVLayout = new QVBoxLayout(perfGroup);
    QHBoxLayout* threadRow = new QHBoxLayout;
    QLabel* minLabel = new QLabel(tr("1"), this);
    QLabel* maxLabel = new QLabel(tr("8"), this);
    m_threadSlider = new QSlider(Qt::Horizontal, this);
    m_threadSlider->setRange(1, 8);
    m_threadSlider->setSingleStep(1);
    m_threadSlider->setToolTip(tr("More threads = faster processing, higher memory usage."));
    m_threadLabel = new QLabel(tr("4 threads"), this);
    m_threadLabel->setMinimumWidth(65);
    threadRow->addWidget(minLabel);
    threadRow->addWidget(m_threadSlider);
    threadRow->addWidget(maxLabel);
    threadRow->addWidget(m_threadLabel);
    QLabel* threadDesc = new QLabel(tr("Worker threads (1–8):"), this);
    perfVLayout->addWidget(threadDesc);
    perfVLayout->addLayout(threadRow);
    layout->addWidget(perfGroup);

    connect(m_threadSlider, &QSlider::valueChanged, this, [this](int v) {
        m_threadLabel->setText(tr("%1 thread%2").arg(v).arg(v == 1 ? QString() : QStringLiteral("s")));
    });

    QGroupBox* expGroup = new QGroupBox(tr("Experimental"), this);
    QVBoxLayout* expVLayout = new QVBoxLayout(expGroup);
    m_experimentalCheck = new QCheckBox(tr("Enable experimental features"), this);
    m_experimentalCheck->setToolTip(tr("Enables OCR, AI-powered search, and other preview features."));
    expVLayout->addWidget(m_experimentalCheck);
    layout->addWidget(expGroup);

    layout->addStretch();
}

void AdvancedPage::load(const AdvancedSettings& s)
{
    m_loggingCheck->setChecked(s.enableLogging);
    m_logLevelCombo->setEnabled(s.enableLogging);
    int logIdx = m_logLevelCombo->findData(s.logLevel);
    m_logLevelCombo->setCurrentIndex(logIdx >= 0 ? logIdx : 0);
    m_threadSlider->setValue(s.maxWorkerThreads);
    m_threadLabel->setText(tr("%1 thread%2").arg(s.maxWorkerThreads).arg(s.maxWorkerThreads == 1 ? QString() : QStringLiteral("s")));
    m_experimentalCheck->setChecked(s.experimentalFeatures);
}

AdvancedSettings AdvancedPage::collect() const
{
    AdvancedSettings s;
    s.enableLogging        = m_loggingCheck->isChecked();
    s.logLevel             = m_logLevelCombo->currentData().toString();
    s.maxWorkerThreads     = m_threadSlider->value();
    s.experimentalFeatures = m_experimentalCheck->isChecked();
    return s;
}

// ============================================================================
// SettingsDialog
// ============================================================================
SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Preferences"));
    setMinimumSize(760, 520);
    resize(860, 580);
    m_originalSettings = SettingsManager::instance()->load();
    setupUI();
    loadSettingsIntoUI(m_originalSettings);
}

void SettingsDialog::setupUI()
{
    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // ---- Search bar ----
    QWidget* searchBar = new QWidget(this);
    searchBar->setObjectName(QStringLiteral("SettingsSearchBar"));
    QHBoxLayout* searchLayout = new QHBoxLayout(searchBar);
    searchLayout->setContentsMargins(12, 8, 12, 8);
    QLabel* searchIcon = new QLabel(tr("\U0001F50D"), searchBar);
    m_searchEdit = new QLineEdit(searchBar);
    m_searchEdit->setPlaceholderText(tr("Search settings…"));
    m_searchEdit->setClearButtonEnabled(true);
    searchLayout->addWidget(searchIcon);
    searchLayout->addWidget(m_searchEdit);
    rootLayout->addWidget(searchBar);

    // Thin separator line
    QFrame* sep = new QFrame(this);
    sep->setFrameShape(QFrame::HLine);
    sep->setFrameShadow(QFrame::Sunken);
    rootLayout->addWidget(sep);

    // ---- Main area: sidebar + page stack ----
    QSplitter* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setHandleWidth(1);

    // Category list (sidebar)
    m_categoryList = new QListWidget(splitter);
    m_categoryList->setObjectName(QStringLiteral("SettingsCategoryList"));
    m_categoryList->setMaximumWidth(170);
    m_categoryList->setMinimumWidth(130);
    m_categoryList->addItem(tr("Appearance"));
    m_categoryList->addItem(tr("General"));
    m_categoryList->addItem(tr("Viewing"));
    m_categoryList->addItem(tr("Security"));
    m_categoryList->addItem(tr("Advanced"));
    m_categoryList->setCurrentRow(0);

    // Page stack
    m_pageStack = new QStackedWidget(splitter);

    m_appearancePage = new AppearancePage(m_pageStack);
    m_generalPage    = new GeneralPage(m_pageStack);
    m_viewingPage    = new ViewingPage(m_pageStack);
    m_securityPage   = new SecurityPage(m_pageStack);
    m_advancedPage   = new AdvancedPage(m_pageStack);

    m_pageStack->addWidget(m_appearancePage);
    m_pageStack->addWidget(m_generalPage);
    m_pageStack->addWidget(m_viewingPage);
    m_pageStack->addWidget(m_securityPage);
    m_pageStack->addWidget(m_advancedPage);

    splitter->addWidget(m_categoryList);
    splitter->addWidget(m_pageStack);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);

    rootLayout->addWidget(splitter, 1);

    // ---- Bottom separator ----
    QFrame* bottomSep = new QFrame(this);
    bottomSep->setFrameShape(QFrame::HLine);
    rootLayout->addWidget(bottomSep);

    // ---- Button bar ----
    QWidget* btnBar = new QWidget(this);
    QHBoxLayout* btnLayout = new QHBoxLayout(btnBar);
    btnLayout->setContentsMargins(12, 8, 12, 8);

    m_exportBtn  = new QPushButton(tr("Export Settings…"), btnBar);
    m_importBtn  = new QPushButton(tr("Import Settings…"), btnBar);
    m_restoreBtn = new QPushButton(tr("Restore Defaults"), btnBar);

    m_buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply,
        Qt::Horizontal, btnBar);

    btnLayout->addWidget(m_exportBtn);
    btnLayout->addWidget(m_importBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(m_restoreBtn);
    btnLayout->addWidget(m_buttonBox);
    rootLayout->addWidget(btnBar);

    // ---- Connections ----
    connect(m_categoryList, &QListWidget::currentRowChanged, this, &SettingsDialog::onCategoryChanged);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &SettingsDialog::onSearchTextChanged);
    connect(m_buttonBox, &QDialogButtonBox::clicked, this, &SettingsDialog::onButtonClicked);
    connect(m_restoreBtn, &QPushButton::clicked, this, &SettingsDialog::onRestoreDefaults);
    connect(m_exportBtn,  &QPushButton::clicked, this, &SettingsDialog::onExportSettings);
    connect(m_importBtn,  &QPushButton::clicked, this, &SettingsDialog::onImportSettings);
}

void SettingsDialog::loadSettingsIntoUI(const AppSettings& s)
{
    m_appearancePage->load(s.appearance);
    m_generalPage->load(s.general);
    m_viewingPage->load(s.viewing);
    m_securityPage->load(s.security);
    m_advancedPage->load(s.advanced);
}

AppSettings SettingsDialog::collectSettingsFromUI() const
{
    AppSettings s;
    s.schemaVersion = SettingsManager::currentSchemaVersion();
    s.appearance    = m_appearancePage->collect();
    s.general       = m_generalPage->collect();
    s.viewing       = m_viewingPage->collect();
    s.security      = m_securityPage->collect();
    s.advanced      = m_advancedPage->collect();
    return s;
}

bool SettingsDialog::applyAndValidate()
{
    AppSettings s = collectSettingsFromUI();

    // Validate individually before delegating to SettingsManager.
    if (s.appearance.fontSize < 10 || s.appearance.fontSize > 20) {
        QMessageBox::warning(this, tr("Validation Error"),
            tr("Font size must be between 10 and 20 pt."));
        m_categoryList->setCurrentRow(0);
        return false;
    }
    if (s.viewing.cacheSizeMB < 50 || s.viewing.cacheSizeMB > 500) {
        QMessageBox::warning(this, tr("Validation Error"),
            tr("Cache size must be between 50 and 500 MB."));
        m_categoryList->setCurrentRow(2);
        return false;
    }
    if (s.advanced.maxWorkerThreads < 1 || s.advanced.maxWorkerThreads > 8) {
        QMessageBox::warning(this, tr("Validation Error"),
            tr("Worker threads must be between 1 and 8."));
        m_categoryList->setCurrentRow(4);
        return false;
    }
    if (s.general.homeStartPage != QStringLiteral("recent") &&
        s.general.homeStartPage != QStringLiteral("browse")) {
        QMessageBox::warning(this, tr("Validation Error"),
            tr("Home start page must be 'recent' or 'browse'."));
        m_categoryList->setCurrentRow(1);
        return false;
    }

    SettingsManager::instance()->save(s);

    // Apply theme immediately via ThemeManager.
    ThemeManager::Theme theme = ThemeManager::Dark;
    if (s.appearance.theme == QStringLiteral("light"))  theme = ThemeManager::Light;
    else if (s.appearance.theme == QStringLiteral("system")) theme = ThemeManager::System;
    ThemeManager::instance()->applyTheme(theme);

    return true;
}

void SettingsDialog::onCategoryChanged(int row)
{
    m_pageStack->setCurrentIndex(row);
}

void SettingsDialog::onSearchTextChanged(const QString& text)
{
    if (text.isEmpty()) {
        // Restore all items
        for (int i = 0; i < m_categoryList->count(); ++i)
            m_categoryList->item(i)->setHidden(false);
        return;
    }
    // Simple filter: hide categories whose name doesn't contain the search text.
    // A real implementation would also search within page contents.
    for (int i = 0; i < m_categoryList->count(); ++i) {
        QListWidgetItem* item = m_categoryList->item(i);
        bool match = item->text().contains(text, Qt::CaseInsensitive);
        item->setHidden(!match);
    }
    // Auto-navigate to first visible match.
    for (int i = 0; i < m_categoryList->count(); ++i) {
        if (!m_categoryList->item(i)->isHidden()) {
            m_categoryList->setCurrentRow(i);
            break;
        }
    }
}

void SettingsDialog::onButtonClicked(QAbstractButton* button)
{
    QDialogButtonBox::StandardButton sb = m_buttonBox->standardButton(button);
    if (sb == QDialogButtonBox::Ok) {
        if (applyAndValidate())
            accept();
    } else if (sb == QDialogButtonBox::Apply) {
        onApply();
    } else if (sb == QDialogButtonBox::Cancel) {
        // Restore original settings on cancel.
        SettingsManager::instance()->save(m_originalSettings);
        ThemeManager::Theme theme = ThemeManager::Dark;
        if (m_originalSettings.appearance.theme == QStringLiteral("light"))  theme = ThemeManager::Light;
        else if (m_originalSettings.appearance.theme == QStringLiteral("system")) theme = ThemeManager::System;
        ThemeManager::instance()->applyTheme(theme);
        reject();
    }
}

void SettingsDialog::onApply()
{
    applyAndValidate();
}

void SettingsDialog::onRestoreDefaults()
{
    QMessageBox::StandardButton result = QMessageBox::question(
        this, tr("Restore Defaults"),
        tr("Reset all settings to their default values?\n\nThis cannot be undone."),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (result != QMessageBox::Yes) return;

    SettingsManager::instance()->resetToDefaults();
    AppSettings defaults;
    loadSettingsIntoUI(defaults);

    // Apply default theme immediately.
    ThemeManager::instance()->applyTheme(ThemeManager::Dark);
}

void SettingsDialog::onExportSettings()
{
    QString filePath = QFileDialog::getSaveFileName(
        this, tr("Export Settings"),
        QStringLiteral("pdfviewer_settings.json"),
        tr("JSON files (*.json)"));
    if (filePath.isEmpty()) return;

    // Apply current UI state before export so the exported file is current.
    AppSettings s = collectSettingsFromUI();
    SettingsManager::instance()->save(s);

    if (!SettingsManager::instance()->exportToJson(filePath)) {
        QMessageBox::warning(this, tr("Export Failed"),
            SettingsManager::instance()->lastError());
    } else {
        QMessageBox::information(this, tr("Export Successful"),
            tr("Settings exported to:\n%1").arg(filePath));
    }
}

void SettingsDialog::onImportSettings()
{
    QString filePath = QFileDialog::getOpenFileName(
        this, tr("Import Settings"),
        QString(),
        tr("JSON files (*.json)"));
    if (filePath.isEmpty()) return;

    QStringList ignoredKeys;
    if (!SettingsManager::instance()->importFromJson(filePath, ignoredKeys)) {
        QMessageBox::warning(this, tr("Import Failed"),
            SettingsManager::instance()->lastError());
        return;
    }

    // Reload UI with imported values.
    loadSettingsIntoUI(SettingsManager::instance()->current());

    if (!ignoredKeys.isEmpty()) {
        QMessageBox::information(this, tr("Import Successful"),
            tr("Settings imported successfully.\n\n"
               "The following locked security settings were ignored:\n• %1")
               .arg(ignoredKeys.join(QStringLiteral("\n• "))));
    } else {
        QMessageBox::information(this, tr("Import Successful"),
            tr("Settings imported successfully."));
    }

    // Apply imported theme.
    const AppSettings& cur = SettingsManager::instance()->current();
    ThemeManager::Theme theme = ThemeManager::Dark;
    if (cur.appearance.theme == QStringLiteral("light"))  theme = ThemeManager::Light;
    else if (cur.appearance.theme == QStringLiteral("system")) theme = ThemeManager::System;
    ThemeManager::instance()->applyTheme(theme);
}
