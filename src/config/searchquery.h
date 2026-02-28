#ifndef SEARCHQUERY_H
#define SEARCHQUERY_H

#include <QString>
#include <QDateTime>
#include <QRegularExpression>

struct SearchQuery {
    QString text;                    // Search text (filename pattern)
    bool pdfOnly = true;             // Filter to PDF files only
    QString dateFilter;              // "any", "24h", "7d", "custom"
    QDateTime dateFrom;             // Custom date range start
    QDateTime dateTo;                // Custom date range end
    QString sizeFilter;              // "any", "<5MB", "5-50MB", ">50MB", "custom"
    qint64 sizeMin = -1;            // Custom size min (bytes, -1 = no limit)
    qint64 sizeMax = -1;            // Custom size max (bytes, -1 = no limit)
    QString scope;                   // "currentFolder", "location", "all"
    QString scopePath;               // Specific location path if scope == "location"

    SearchQuery() = default;
    
    bool isEmpty() const {
        return text.isEmpty() && dateFilter == "any" && sizeFilter == "any";
    }
    
    QRegularExpression namePattern() const {
        if (text.isEmpty()) {
            return QRegularExpression();
        }
        // Convert simple wildcards to regex
        QString pattern = QRegularExpression::escape(text);
        pattern.replace("\\*", ".*");
        pattern.replace("\\?", ".");
        return QRegularExpression(pattern, QRegularExpression::CaseInsensitiveOption);
    }
};

#endif // SEARCHQUERY_H
