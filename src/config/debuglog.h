#ifndef PDFVIEWER_DEBUGLOG_H
#define PDFVIEWER_DEBUGLOG_H

#include <QString>

// Lightweight, optional debug logging helper used by a few UI components.
// Logging is compiled out entirely unless PDFVIEWER_ENABLE_DEBUGLOG is defined.

namespace DebugLog
{
    void write(const QString &location,
               const QString &hypothesisId,
               const QString &message,
               const QString &dataJson = QString());
}

#endif // PDFVIEWER_DEBUGLOG_H

