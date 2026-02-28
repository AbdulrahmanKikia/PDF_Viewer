#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include <QObject>
#include <QString>
#include <QJsonDocument>

struct TabSessionData {
    QString type;           // "home" or "pdf"
    QString filePath;       // Empty for home tabs
    int currentPage = 1;
    float zoomLevel = 100.0f;
    bool isActive = false;

    TabSessionData() = default;
    TabSessionData(const QString& t, const QString& path = QString(), int page = 1, float zoom = 100.0f, bool active = false)
        : type(t), filePath(path), currentPage(page), zoomLevel(zoom), isActive(active) {}
};

class SessionManager : public QObject
{
    Q_OBJECT

public:
    static SessionManager* instance();

    bool saveSession(const QList<TabSessionData>& tabs, int activeTabIndex);
    bool loadSession(QList<TabSessionData>& tabs, int& activeTabIndex);

    QString sessionFilePath() const;

private:
    explicit SessionManager(QObject* parent = nullptr);
    static SessionManager* s_instance;

    QString m_sessionFilePath;
    QJsonDocument m_sessionData;

    void ensureSessionDirectory() const;
};

#endif // SESSIONMANAGER_H
