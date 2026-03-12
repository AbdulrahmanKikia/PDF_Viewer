#pragma once

#include <QString>

namespace InstallActions {

// Performs extraction of the ZIP at zipPath into installDir and creates
// any required directory structure. Returns true on success.
bool performInstall(const QString &zipPath, const QString &installDir, QString *errorMessage);

// Creates Start menu / Desktop shortcuts on Windows. Non-fatal.
void createShortcuts(const QString &installDir);

} // namespace InstallActions

