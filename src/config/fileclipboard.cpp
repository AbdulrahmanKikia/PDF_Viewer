#include "fileclipboard.h"
#include <QApplication>

FileClipboard *FileClipboard::s_instance = nullptr;

FileClipboard::FileClipboard(QObject *parent)
    : QObject(parent)
{
}

FileClipboard *FileClipboard::instance()
{
    if (!s_instance) {
        s_instance = new FileClipboard(qApp);
    }
    return s_instance;
}

void FileClipboard::copy(const QString &path)
{
    m_entries.clear();
    m_entries.append(ClipboardEntry(path, false));
    m_isCutOperation = false;
    emit clipboardChanged();
}

void FileClipboard::cut(const QString &path)
{
    m_entries.clear();
    m_entries.append(ClipboardEntry(path, true));
    m_isCutOperation = true;
    emit clipboardChanged();
}

void FileClipboard::copyMultiple(const QList<QString> &paths)
{
    m_entries.clear();
    for (const QString &path : paths) {
        m_entries.append(ClipboardEntry(path, false));
    }
    m_isCutOperation = false;
    emit clipboardChanged();
}

void FileClipboard::cutMultiple(const QList<QString> &paths)
{
    m_entries.clear();
    for (const QString &path : paths) {
        m_entries.append(ClipboardEntry(path, true));
    }
    m_isCutOperation = true;
    emit clipboardChanged();
}

void FileClipboard::clear()
{
    m_entries.clear();
    m_isCutOperation = false;
    emit clipboardChanged();
}

QList<QString> FileClipboard::getPaths() const
{
    QList<QString> paths;
    for (const ClipboardEntry &entry : m_entries) {
        paths.append(entry.sourcePath);
    }
    return paths;
}
