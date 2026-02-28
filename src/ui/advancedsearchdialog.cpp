#include "advancedsearchdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QFormLayout>
#include <QSettings>
#include <QFileInfo>
#include <QStandardPaths>
#include <QDate>
#include <QDateTime>

AdvancedSearchDialog::AdvancedSearchDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Advanced Search"));
    setModal(true);
    resize(500, 400);
    
    loadPreferences();
    setupUI();
}

AdvancedSearchDialog::~AdvancedSearchDialog()
{
}

void AdvancedSearchDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);

    // Text search
    QGroupBox *textGroup = new QGroupBox(tr("Search Text"), this);
    QVBoxLayout *textLayout = new QVBoxLayout(textGroup);
    m_textSearchEdit = new QLineEdit(this);
    m_textSearchEdit->setPlaceholderText(tr("Enter filename pattern (supports * and ?)"));
    textLayout->addWidget(m_textSearchEdit);
    mainLayout->addWidget(textGroup);

    // File type filter
    QGroupBox *typeGroup = new QGroupBox(tr("File Type"), this);
    QVBoxLayout *typeLayout = new QVBoxLayout(typeGroup);
    m_pdfOnlyCheck = new QCheckBox(tr("PDF files only"), this);
    m_pdfOnlyCheck->setChecked(true);
    typeLayout->addWidget(m_pdfOnlyCheck);
    mainLayout->addWidget(typeGroup);

    // Date filter
    QGroupBox *dateGroup = new QGroupBox(tr("Date Modified"), this);
    QFormLayout *dateLayout = new QFormLayout(dateGroup);
    m_dateFilterCombo = new QComboBox(this);
    m_dateFilterCombo->addItem(tr("Any"), "any");
    m_dateFilterCombo->addItem(tr("Last 24 hours"), "24h");
    m_dateFilterCombo->addItem(tr("Last 7 days"), "7d");
    m_dateFilterCombo->addItem(tr("Custom range"), "custom");
    connect(m_dateFilterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AdvancedSearchDialog::onDateFilterChanged);
    dateLayout->addRow(tr("Filter:"), m_dateFilterCombo);

    m_dateFromLabel = new QLabel(tr("From:"), this);
    m_dateFromEdit = new QDateEdit(this);
    m_dateFromEdit->setCalendarPopup(true);
    m_dateFromEdit->setDate(QDate::currentDate().addDays(-7));
    m_dateFromEdit->setEnabled(false);
    dateLayout->addRow(m_dateFromLabel, m_dateFromEdit);

    m_dateToLabel = new QLabel(tr("To:"), this);
    m_dateToEdit = new QDateEdit(this);
    m_dateToEdit->setCalendarPopup(true);
    m_dateToEdit->setDate(QDate::currentDate());
    m_dateToEdit->setEnabled(false);
    dateLayout->addRow(m_dateToLabel, m_dateToEdit);
    mainLayout->addWidget(dateGroup);

    // Size filter
    QGroupBox *sizeGroup = new QGroupBox(tr("File Size"), this);
    QFormLayout *sizeLayout = new QFormLayout(sizeGroup);
    m_sizeFilterCombo = new QComboBox(this);
    m_sizeFilterCombo->addItem(tr("Any"), "any");
    m_sizeFilterCombo->addItem(tr("Less than 5 MB"), "<5MB");
    m_sizeFilterCombo->addItem(tr("5 - 50 MB"), "5-50MB");
    m_sizeFilterCombo->addItem(tr("Greater than 50 MB"), ">50MB");
    m_sizeFilterCombo->addItem(tr("Custom range"), "custom");
    connect(m_sizeFilterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AdvancedSearchDialog::onSizeFilterChanged);
    sizeLayout->addRow(tr("Filter:"), m_sizeFilterCombo);

    m_sizeMinLabel = new QLabel(tr("Min (MB):"), this);
    m_sizeMinSpin = new QSpinBox(this);
    m_sizeMinSpin->setRange(0, 10000);
    m_sizeMinSpin->setSuffix(" MB");
    m_sizeMinSpin->setEnabled(false);
    sizeLayout->addRow(m_sizeMinLabel, m_sizeMinSpin);

    m_sizeMaxLabel = new QLabel(tr("Max (MB):"), this);
    m_sizeMaxSpin = new QSpinBox(this);
    m_sizeMaxSpin->setRange(0, 10000);
    m_sizeMaxSpin->setSuffix(" MB");
    m_sizeMaxSpin->setValue(50);
    m_sizeMaxSpin->setEnabled(false);
    sizeLayout->addRow(m_sizeMaxLabel, m_sizeMaxSpin);
    mainLayout->addWidget(sizeGroup);

    // Scope
    QGroupBox *scopeGroup = new QGroupBox(tr("Search Scope"), this);
    QFormLayout *scopeLayout = new QFormLayout(scopeGroup);
    m_scopeCombo = new QComboBox(this);
    m_scopeCombo->addItem(tr("Current folder"), "currentFolder");
    m_scopeCombo->addItem(tr("Specific location"), "location");
    m_scopeCombo->addItem(tr("All indexed locations"), "all");
    scopeLayout->addRow(tr("Scope:"), m_scopeCombo);
    mainLayout->addWidget(scopeGroup);

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    m_searchBtn = new QPushButton(tr("Search"), this);
    m_searchBtn->setDefault(true);
    connect(m_searchBtn, &QPushButton::clicked, this, &AdvancedSearchDialog::onSearchClicked);
    buttonLayout->addWidget(m_searchBtn);

    m_resetBtn = new QPushButton(tr("Reset"), this);
    connect(m_resetBtn, &QPushButton::clicked, this, &AdvancedSearchDialog::onResetClicked);
    buttonLayout->addWidget(m_resetBtn);

    m_cancelBtn = new QPushButton(tr("Cancel"), this);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(m_cancelBtn);

    mainLayout->addLayout(buttonLayout);
}

SearchQuery AdvancedSearchDialog::getSearchQuery() const
{
    SearchQuery query;
    query.text = m_textSearchEdit->text();
    query.pdfOnly = m_pdfOnlyCheck->isChecked();
    query.dateFilter = m_dateFilterCombo->currentData().toString();
    query.dateFrom = m_dateFromEdit->dateTime();
    query.dateTo = m_dateToEdit->dateTime();
    query.sizeFilter = m_sizeFilterCombo->currentData().toString();
    query.sizeMin = (m_sizeMinSpin->isEnabled() && m_sizeMinSpin->value() > 0) 
                    ? m_sizeMinSpin->value() * 1024 * 1024 : -1;
    query.sizeMax = (m_sizeMaxSpin->isEnabled() && m_sizeMaxSpin->value() > 0) 
                    ? m_sizeMaxSpin->value() * 1024 * 1024 : -1;
    query.scope = m_scopeCombo->currentData().toString();
    query.scopePath = m_currentFolderPath;
    
    return query;
}

void AdvancedSearchDialog::setSearchQuery(const SearchQuery &query)
{
    m_textSearchEdit->setText(query.text);
    m_pdfOnlyCheck->setChecked(query.pdfOnly);
    
    int dateIndex = m_dateFilterCombo->findData(query.dateFilter);
    if (dateIndex >= 0) {
        m_dateFilterCombo->setCurrentIndex(dateIndex);
    }
    m_dateFromEdit->setDateTime(query.dateFrom);
    m_dateToEdit->setDateTime(query.dateTo);
    
    int sizeIndex = m_sizeFilterCombo->findData(query.sizeFilter);
    if (sizeIndex >= 0) {
        m_sizeFilterCombo->setCurrentIndex(sizeIndex);
    }
    if (query.sizeMin > 0) {
        m_sizeMinSpin->setValue(query.sizeMin / (1024 * 1024));
    }
    if (query.sizeMax > 0) {
        m_sizeMaxSpin->setValue(query.sizeMax / (1024 * 1024));
    }
    
    int scopeIndex = m_scopeCombo->findData(query.scope);
    if (scopeIndex >= 0) {
        m_scopeCombo->setCurrentIndex(scopeIndex);
    }
}

void AdvancedSearchDialog::setCurrentFolder(const QString &folderPath)
{
    m_currentFolderPath = folderPath;
}

void AdvancedSearchDialog::onDateFilterChanged(int index)
{
    bool isCustom = (m_dateFilterCombo->itemData(index).toString() == "custom");
    m_dateFromEdit->setEnabled(isCustom);
    m_dateToEdit->setEnabled(isCustom);
    m_dateFromLabel->setEnabled(isCustom);
    m_dateToLabel->setEnabled(isCustom);
}

void AdvancedSearchDialog::onSizeFilterChanged(int index)
{
    bool isCustom = (m_sizeFilterCombo->itemData(index).toString() == "custom");
    m_sizeMinSpin->setEnabled(isCustom);
    m_sizeMaxSpin->setEnabled(isCustom);
    m_sizeMinLabel->setEnabled(isCustom);
    m_sizeMaxLabel->setEnabled(isCustom);
}

void AdvancedSearchDialog::onSearchClicked()
{
    SearchQuery query = getSearchQuery();
    savePreferences();
    emit searchRequested(query);
    accept();
}

void AdvancedSearchDialog::onResetClicked()
{
    m_textSearchEdit->clear();
    m_pdfOnlyCheck->setChecked(true);
    m_dateFilterCombo->setCurrentIndex(0);
    m_sizeFilterCombo->setCurrentIndex(0);
    m_scopeCombo->setCurrentIndex(0);
}

void AdvancedSearchDialog::loadPreferences()
{
    QSettings settings;
    bool pdfOnly = settings.value(QStringLiteral("home/search/pdfOnly"), true).toBool();
    QString lastScope = settings.value(QStringLiteral("home/search/lastScope"), QStringLiteral("currentFolder")).toString();
    
    // Apply to UI in setupUI() after widgets are created
    Q_UNUSED(pdfOnly);
    Q_UNUSED(lastScope);
}

void AdvancedSearchDialog::savePreferences()
{
    QSettings settings;
    settings.setValue(QStringLiteral("home/search/pdfOnly"), m_pdfOnlyCheck->isChecked());
    settings.setValue(QStringLiteral("home/search/lastScope"), m_scopeCombo->currentData().toString());
}
