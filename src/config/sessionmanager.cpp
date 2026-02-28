#include "sessionmanager.h"
#include <QApplication>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDebug>

SessionManager* SessionManager::s_instance = nullptr;

SessionManager::SessionManager(QObject* parent)
    : QObject(parent)
{
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (configPath.isEmpty()) {
        configPath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + QStringLiteral("/.config/PDFViewer");
    }
    m_sessionFilePath = configPath + QStringLiteral("/last_session.json");
    ensureSessionDirectory();
}

SessionManager* SessionManager::instance()
{
    if (!s_instance) {
        s_instance = new SessionManager(qApp);
    }
    return s_instance;
}

QString SessionManager::sessionFilePath() const
{
    return m_sessionFilePath;
}

bool SessionManager::saveSession(const QList<TabSessionData>& tabs, int activeTabIndex)
{
    QJsonObject root;
    QJsonArray tabsArray;

    for (int i = 0; i < tabs.size(); ++i) {
        const TabSessionData& tab = tabs.at(i);
        QJsonObject tabObj;
        tabObj[QStringLiteral("type")] = tab.type;
        if (!tab.filePath.isEmpty()) {
            tabObj[QStringLiteral("filePath")] = tab.filePath;
        }
        tabObj[QStringLiteral("currentPage")] = tab.currentPage;
        tabObj[QStringLiteral("zoomLevel")] = tab.zoomLevel;
        tabObj[QStringLiteral("isActive")] = (tab.isActive || i == activeTabIndex);
        tabsArray.append(tabObj);
    }

    root[QStringLiteral("tabs")] = tabsArray;
    root[QStringLiteral("activeTabIndex")] = activeTabIndex;

    QJsonDocument doc(root);
    QFile file(m_sessionFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to save session:" << file.errorString();
        return false;
    }

    file.write(doc.toJson());
    file.close();
    return true;
}

bool SessionManager::loadSession(QList<TabSessionData>& tabs, int& activeTabIndex)
{
    tabs.clear();
    activeTabIndex = 0;

    QFile file(m_sessionFilePath);
    if (!file.exists() || !file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "Failed to parse session JSON:" << error.errorString()
                   << "- clearing corrupt session file";
        file.remove();
        return false;
    }

    QJsonObject root = doc.object();
    QJsonArray tabsArray = root[QStringLiteral("tabs")].toArray();
    activeTabIndex = root[QStringLiteral("activeTabIndex")].toInt(0);

    for (const QJsonValue& value : tabsArray) {
        QJsonObject tabObj = value.toObject();
        QString type = tabObj[QStringLiteral("type")].toString();
        QString filePath = tabObj[QStringLiteral("filePath")].toString();
        int currentPage = tabObj[QStringLiteral("currentPage")].toInt(1);
        float zoomLevel = static_cast<float>(tabObj[QStringLiteral("zoomLevel")].toDouble(100.0));
        bool isActive = tabObj[QStringLiteral("isActive")].toBool(false);

        TabSessionData tab(type, filePath, currentPage, zoomLevel, isActive);
        tabs.append(tab);
    }

    return true;
}

void SessionManager::ensureSessionDirectory() const
{
    QFileInfo fileInfo(m_sessionFilePath);
    QDir dir = fileInfo.absoluteDir();
    if (!dir.exists()) {
        dir.mkpath(QStringLiteral("."));
    }
}
