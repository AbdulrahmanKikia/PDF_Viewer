#include "recentfilesmanager.h"
#include <QSettings>
#include <QApplication>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDebug>
#include <algorithm>

RecentFilesManager *RecentFilesManager::s_instance = nullptr;

RecentFilesManager::RecentFilesManager(QObject *parent)
    : QObject(parent)
{
    loadRecentFiles();
}

RecentFilesManager *RecentFilesManager::instance()
{
    if (!s_instance) {
        s_instance = new RecentFilesManager(qApp);
    }
    return s_instance;
}

void RecentFilesManager::addFile(const QString &filePath)
{
    if (filePath.isEmpty() || !QFileInfo::exists(filePath)) {
        return;
    }

    QFileInfo info(filePath);
    if (info.suffix().toLower() != QStringLiteral("pdf")) {
        return; // Only track PDF files
    }

    RecentFileEntry entry(filePath, QDateTime::currentDateTime());

    // Remove existing entry if present
    m_recentFiles.removeAll(entry);

    // Add to front
    m_recentFiles.prepend(entry);

    // Limit to MAX_RECENT_FILES
    if (m_recentFiles.size() > MAX_RECENT_FILES) {
        m_recentFiles = m_recentFiles.mid(0, MAX_RECENT_FILES);
    }

    saveRecentFiles();
    emit recentFilesChanged();
}

QList<RecentFileEntry> RecentFilesManager::getRecentFiles(int maxCount) const
{
    // Filter out files that no longer exist
    QList<RecentFileEntry> validFiles;
    for (const RecentFileEntry &entry : m_recentFiles) {
        if (QFileInfo::exists(entry.filePath)) {
            validFiles.append(entry);
            if (validFiles.size() >= maxCount) {
                break;
            }
        }
    }

    return validFiles;
}

void RecentFilesManager::clearRecentFiles()
{
    m_recentFiles.clear();
    saveRecentFiles();
    emit recentFilesChanged();
}

void RecentFilesManager::removeFile(const QString &filePath)
{
    RecentFileEntry entry(filePath);
    m_recentFiles.removeAll(entry);
    saveRecentFiles();
    emit recentFilesChanged();
}

void RecentFilesManager::loadRecentFiles()
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("RecentFiles"));

    int count = settings.value(QStringLiteral("count"), 0).toInt();
    m_recentFiles.clear();

    for (int i = 0; i < count && i < MAX_RECENT_FILES; ++i) {
        QString key = QStringLiteral("file_%1").arg(i);
        QString filePath = settings.value(key + QStringLiteral("/path")).toString();
        QDateTime lastOpened = settings.value(key + QStringLiteral("/date")).toDateTime();

        if (!filePath.isEmpty() && QFileInfo::exists(filePath)) {
            RecentFileEntry entry(filePath, lastOpened);
            m_recentFiles.append(entry);
        }
    }

    settings.endGroup();
}

void RecentFilesManager::saveRecentFiles()
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("RecentFiles"));

    settings.setValue(QStringLiteral("count"), m_recentFiles.size());

    for (int i = 0; i < m_recentFiles.size(); ++i) {
        QString key = QStringLiteral("file_%1").arg(i);
        settings.setValue(key + QStringLiteral("/path"), m_recentFiles.at(i).filePath);
        settings.setValue(key + QStringLiteral("/date"), m_recentFiles.at(i).lastOpened);
    }

    settings.endGroup();
    settings.sync();
}
