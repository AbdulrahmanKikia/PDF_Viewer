#pragma once

#include <QString>

namespace ZipExtractor {

// Extracts all contents of the ZIP file at zipPath into targetDir.
// Returns true on success; on failure, returns false and sets errorMessage.
bool extractAll(const QString &zipPath, const QString &targetDir, QString *errorMessage);

} // namespace ZipExtractor

