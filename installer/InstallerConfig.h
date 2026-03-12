#pragma once

#include <QString>

namespace InstallerConfig {

// Product metadata
inline QString productName() { return QStringLiteral("PDF Viewer"); }
inline QString productPublisher() { return QStringLiteral("PDF Viewer Project"); }
inline QString productVersion() { return QStringLiteral("1.2.0"); }

// URL of the ZIP payload to download.
// TODO: replace with your real release URL before shipping.
inline QString downloadUrl() {
    return QStringLiteral("https://example.com/PDFViewer-1.2.0-win64.zip");
}

// Default relative name of the downloaded ZIP file.
inline QString downloadedZipFileName() {
    return QStringLiteral("PDFViewer-1.2.0-win64.zip");
}

// Default install directory name under the chosen base path.
inline QString defaultInstallDirName() {
    return QStringLiteral("PDF Viewer");
}

} // namespace InstallerConfig

