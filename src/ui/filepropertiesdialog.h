#ifndef FILEPROPERTIESDIALOG_H
#define FILEPROPERTIESDIALOG_H

#include <QDialog>

class QLabel;
class QLineEdit;
class QCheckBox;
class QPushButton;
class QFormLayout;
class QVBoxLayout;

class FilePropertiesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FilePropertiesDialog(const QString &filePath, QWidget *parent = nullptr);
    ~FilePropertiesDialog();

private:
    void setupUI();
    void loadFileInfo();

    QString m_filePath;
    QLabel *m_nameLabel = nullptr;
    QLineEdit *m_pathEdit = nullptr;
    QLabel *m_typeLabel = nullptr;
    QLabel *m_sizeLabel = nullptr;
    QLabel *m_createdLabel = nullptr;
    QLabel *m_modifiedLabel = nullptr;
    QCheckBox *m_readOnlyCheck = nullptr;
    QPushButton *m_okBtn = nullptr;
};

#endif // FILEPROPERTIESDIALOG_H
