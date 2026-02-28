#ifndef FILECLIPBOARD_H
#define FILECLIPBOARD_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QList>

struct ClipboardEntry {
    QString sourcePath;
    bool isCut;
    QDateTime copiedAt;

    ClipboardEntry() = default;
    ClipboardEntry(const QString &path, bool cut = false)
        : sourcePath(path), isCut(cut), copiedAt(QDateTime::currentDateTime()) {}
};

class FileClipboard : public QObject
{
    Q_OBJECT

public:
    static FileClipboard* instance();

    void copy(const QString &path);
    void cut(const QString &path);
    void copyMultiple(const QList<QString> &paths);
    void cutMultiple(const QList<QString> &paths);
    void clear();

    bool hasData() const { return !m_entries.isEmpty(); }
    bool isCutOperation() const { return m_isCutOperation; }
    QList<QString> getPaths() const;

signals:
    void clipboardChanged();

private:
    explicit FileClipboard(QObject *parent = nullptr);
    static FileClipboard *s_instance;

    QList<ClipboardEntry> m_entries;
    bool m_isCutOperation = false;
};

#endif // FILECLIPBOARD_H
