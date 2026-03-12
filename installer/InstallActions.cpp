#include "InstallActions.h"

#include "ZipExtractor.h"

#include <QDir>

#ifdef _WIN32
#define NOMINMAX
#include <ShlObj.h>
#include <Windows.h>
#include <combaseapi.h>
#include <shobjidl.h>
#include <shlguid.h>
#include <wrl/client.h>
#endif

namespace InstallActions {

bool performInstall(const QString &zipPath, const QString &installDir, QString *errorMessage)
{
    QDir().mkpath(installDir);
    return ZipExtractor::extractAll(zipPath, installDir, errorMessage);
}

static QString startMenuProgramsPath()
{
#ifdef _WIN32
    PWSTR path = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Programs, 0, nullptr, &path))) {
        QString result = QString::fromWCharArray(path);
        CoTaskMemFree(path);
        return result;
    }
#endif
    return QString();
}

#ifdef _WIN32
static void createShellLink(const QString &targetPath,
                            const QString &linkPath,
                            const QString &description)
{
    using Microsoft::WRL::ComPtr;

    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    ComPtr<IShellLinkW> shellLink;
    if (FAILED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(&shellLink)))) {
        return;
    }

    shellLink->SetPath(reinterpret_cast<LPCWSTR>(targetPath.utf16()));
    shellLink->SetDescription(reinterpret_cast<LPCWSTR>(description.utf16()));

    ComPtr<IPersistFile> persistFile;
    if (FAILED(shellLink.As(&persistFile)))
        return;

    persistFile->Save(reinterpret_cast<LPCWSTR>(linkPath.utf16()), TRUE);
}
#endif

void createShortcuts(const QString &installDir)
{
    QDir dir(installDir);
    const QString exePath = dir.filePath(QStringLiteral("PDFViewer.exe"));

#ifdef _WIN32
    const QString programs = startMenuProgramsPath();
    if (!programs.isEmpty()) {
        const QString startMenuDir = programs + QStringLiteral("\\PDF Viewer");
        QDir().mkpath(startMenuDir);

        const QString startMenuLink =
            startMenuDir + QStringLiteral("\\PDF Viewer.lnk");

        createShellLink(exePath, startMenuLink,
                        QStringLiteral("PDF Viewer"));
    }
#else
    Q_UNUSED(exePath);
#endif
}

} // namespace InstallActions

