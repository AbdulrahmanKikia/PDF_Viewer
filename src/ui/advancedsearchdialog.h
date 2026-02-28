#ifndef ADVANCEDSEARCHDIALOG_H
#define ADVANCEDSEARCHDIALOG_H

#include <QDialog>
#include "../config/searchquery.h"

class QLineEdit;
class QCheckBox;
class QComboBox;
class QDateEdit;
class QSpinBox;
class QLabel;
class QPushButton;
class QVBoxLayout;
class QHBoxLayout;
class QGroupBox;

class AdvancedSearchDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AdvancedSearchDialog(QWidget *parent = nullptr);
    ~AdvancedSearchDialog();

    SearchQuery getSearchQuery() const;
    void setSearchQuery(const SearchQuery &query);
    void setCurrentFolder(const QString &folderPath);

signals:
    void searchRequested(const SearchQuery &query);

private slots:
    void onDateFilterChanged(int index);
    void onSizeFilterChanged(int index);
    void onSearchClicked();
    void onResetClicked();

private:
    void setupUI();
    void loadPreferences();
    void savePreferences();

    QLineEdit *m_textSearchEdit = nullptr;
    QCheckBox *m_pdfOnlyCheck = nullptr;
    
    QComboBox *m_dateFilterCombo = nullptr;
    QDateEdit *m_dateFromEdit = nullptr;
    QDateEdit *m_dateToEdit = nullptr;
    QLabel *m_dateFromLabel = nullptr;
    QLabel *m_dateToLabel = nullptr;
    
    QComboBox *m_sizeFilterCombo = nullptr;
    QSpinBox *m_sizeMinSpin = nullptr;
    QSpinBox *m_sizeMaxSpin = nullptr;
    QLabel *m_sizeMinLabel = nullptr;
    QLabel *m_sizeMaxLabel = nullptr;
    
    QComboBox *m_scopeCombo = nullptr;
    QString m_currentFolderPath;
    
    QPushButton *m_searchBtn = nullptr;
    QPushButton *m_resetBtn = nullptr;
    QPushButton *m_cancelBtn = nullptr;
};

#endif // ADVANCEDSEARCHDIALOG_H
