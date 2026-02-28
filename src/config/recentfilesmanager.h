#ifndef RECENTFILESMANAGER_H
#define RECENTFILESMANAGER_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QList>
#include <QFileInfo>

struct RecentFileEntry {
    QString filePath;
    QDateTime lastOpened;
    QString displayName;

    RecentFileEntry() = default;
    RecentFileEntry(const QString &path, const QDateTime &date = QDateTime::currentDateTime())
        : filePath(path), lastOpened(date)
    {
        QFileInfo info(path);
        displayName = info.fileName();
    }

    bool operator==(const RecentFileEntry &other) const {
        return filePath == other.filePath;
    }
};

class RecentFilesManager : public QObject
{
    Q_OBJECT

public:
    static RecentFilesManager* instance();

    void addFile(const QString &filePath);
    QList<RecentFileEntry> getRecentFiles(int maxCount = 20) const;
    void clearRecentFiles();
    void removeFile(const QString &filePath);

signals:
    void recentFilesChanged();

private:
    explicit RecentFilesManager(QObject *parent = nullptr);
    void loadRecentFiles();
    void saveRecentFiles();

    static RecentFilesManager *s_instance;
    QList<RecentFileEntry> m_recentFiles;
    static constexpr int MAX_RECENT_FILES = 50;
};

#endif // RECENTFILESMANAGER_H
