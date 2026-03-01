cmake_minimum_required(VERSION 3.20)
project(PDFViewer VERSION 1.2.0 LANGUAGES CXX)

# -----------------------------------------------------------------------------
# C++ and Qt build setup
# -----------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# -----------------------------------------------------------------------------
# Qt6 path (Windows): must match the compiler you use
# - Visual Studio (MSVC)  -> Qt for MSVC (msvc2022_64 or msvc2019_64)
# - MinGW / Qt Creator   -> Qt for MinGW (mingw_64)
# In CMake Settings you can set: CMAKE_PREFIX_PATH or Qt6_DIR (see comments below).
# -----------------------------------------------------------------------------
set(QT_VERSION "6.10.2")
set(QT_MSVC_2022 "C:/Qt/${QT_VERSION}/msvc2022_64")
set(QT_MSVC_2019 "C:/Qt/${QT_VERSION}/msvc2019_64")
set(QT_MINGW     "C:/Qt/${QT_VERSION}/mingw_64")

if(WIN32)
    if(MSVC)
        # Never use MinGW Qt when building with MSVC (would cause LNK1104 mingw32.lib).
        if(Qt6_DIR AND Qt6_DIR MATCHES "mingw")
            unset(Qt6_DIR CACHE)
        endif()
        if(Qt6_DIR AND Qt6_DIR MATCHES "msvc")
            if(NOT EXISTS "${Qt6_DIR}/Qt6Config.cmake" AND EXISTS "${Qt6_DIR}/lib/cmake/Qt6/Qt6Config.cmake")
                set(Qt6_DIR "${Qt6_DIR}/lib/cmake/Qt6" CACHE PATH "Qt6 config dir" FORCE)
            endif()
        else()
            unset(Qt6_DIR CACHE)
            set(CMAKE_PREFIX_PATH "${QT_MSVC_2022}" "${QT_MSVC_2019}" CACHE STRING "Qt for MSVC" FORCE)
            if(EXISTS "${QT_MSVC_2022}/lib/cmake/Qt6/Qt6Config.cmake")
                set(Qt6_DIR "${QT_MSVC_2022}/lib/cmake/Qt6" CACHE PATH "Qt6 MSVC 2022" FORCE)
            elseif(EXISTS "${QT_MSVC_2019}/lib/cmake/Qt6/Qt6Config.cmake")
                set(Qt6_DIR "${QT_MSVC_2019}/lib/cmake/Qt6" CACHE PATH "Qt6 MSVC 2019" FORCE)
            else()
                message(FATAL_ERROR
                    "Qt ${QT_VERSION} for MSVC not found at ${QT_MSVC_2022} or ${QT_MSVC_2019}.\n"
                    "Set in CMake Settings: Qt6_DIR = <your path>/lib/cmake/Qt6")
            endif()
        endif()
    else()
        # MinGW
        set(CMAKE_PREFIX_PATH "${QT_MINGW}" CACHE STRING "Qt for MinGW" FORCE)
        unset(Qt6_DIR CACHE)
    endif()
endif()

# Optional: Vulkan SDK (removes "Could NOT find WrapVulkanHeaders" when installed)
if(DEFINED ENV{VULKAN_SDK})
    set(Vulkan_INCLUDE_DIR "$ENV{VULKAN_SDK}/Include" CACHE PATH "Vulkan include directory")
endif()

# -----------------------------------------------------------------------------
# Qt6: required + optional modules
# -----------------------------------------------------------------------------
find_package(Qt6 6.10.2 REQUIRED COMPONENTS Core Gui Widgets Concurrent)
find_package(Qt6 6.10.2 QUIET COMPONENTS Svg Pdf PdfWidgets PrintSupport)

# -----------------------------------------------------------------------------
# Qt6 Pdf / PdfWidgets (optional): used for QtPdfRenderer and SimplePdfWindow
#
# If you recently added Qt PDF via Qt Maintenance Tool, delete your build
# directory (e.g. build-msvc) and reconfigure so CMake can detect the new components.
# -----------------------------------------------------------------------------
if(TARGET Qt6::Pdf)
    message(STATUS "Qt6 Pdf: found (${Qt6_Pdf_DIR})")
else()
    message(STATUS "Qt6 Pdf: not found")
endif()
if(TARGET Qt6::PdfWidgets)
    message(STATUS "Qt6 PdfWidgets: found (${Qt6_PdfWidgets_DIR})")
else()
    message(STATUS "Qt6 PdfWidgets: not found")
endif()

# -----------------------------------------------------------------------------
# PDF backends (QtPdf + MuPDF)
#
# Security / hardening knobs:
# - PDFVIEWER_ENABLE_QTPDF:  ON  → allow using QtPdfRenderer when Qt6::Pdf is present
#                             OFF → never compile or link QtPdfRenderer
# - PDFVIEWER_USE_MUPDF:     ON  → enable MuPDF primary backend (see below)
#                             OFF → never link MuPDF; Null/QtPdf are used instead
#
# This makes it easy to build hardened variants, for example:
#   -D PDFVIEWER_USE_MUPDF=OFF  -D PDFVIEWER_ENABLE_QTPDF=OFF
#     → no real PDF backend, safe viewer shell for UI testing
#   -D PDFVIEWER_USE_MUPDF=ON   -D PDFVIEWER_ENABLE_QTPDF=OFF
#     → MuPDF only
#   -D PDFVIEWER_USE_MUPDF=OFF  -D PDFVIEWER_ENABLE_QTPDF=ON
#     → QtPdf only (when Qt6::Pdf is available)
# -----------------------------------------------------------------------------
option(PDFVIEWER_ENABLE_QTPDF "Allow QtPdfRenderer and QtPdf-based UI when Qt6::Pdf/Qt6::PdfWidgets are available" ON)

set(PDFVIEWER_USE_QTPDF OFF)
if(PDFVIEWER_ENABLE_QTPDF AND TARGET Qt6::Pdf AND TARGET Qt6::PdfWidgets)
    message(STATUS "Qt6 Pdf/PdfWidgets: enabled  → QtPdfRenderer + SimplePdfWindow will be built")
    set(PDFVIEWER_USE_QTPDF ON)
elseif(PDFVIEWER_ENABLE_QTPDF)
    message(STATUS "Qt6 Pdf/PdfWidgets: disabled (Pdf or PdfWidgets not found). "
                   "If you just installed Qt PDF, delete the build directory and reconfigure.")
endif()

# -----------------------------------------------------------------------------
# MuPDF (primary backend)
#
# To enable:
#   cmake -DPDFVIEWER_USE_MUPDF=ON [other options] ..
#
# Required variables (auto-detected from third_party/mupdf/ if not overridden):
#   PDFVIEWER_MUPDF_INCLUDE_DIR   path containing mupdf/fitz.h
#   PDFVIEWER_MUPDF_LIB           path to the MuPDF static library
#
# MSVC:  expects   third_party/mupdf/lib/x64/Release/libmupdf.lib
# MinGW: expects   third_party/mupdf/lib/mingw/libmupdf.a
#
# See third_party/mupdf/README.md for full setup instructions.
# -----------------------------------------------------------------------------
option(PDFVIEWER_USE_MUPDF "Enable MuPDF primary rendering backend" OFF)

if(PDFVIEWER_USE_MUPDF)
    # ---------- Auto-detect include dir ----------
    if(NOT PDFVIEWER_MUPDF_INCLUDE_DIR)
        set(PDFVIEWER_MUPDF_INCLUDE_DIR
            "${CMAKE_CURRENT_SOURCE_DIR}/third_party/mupdf/include"
            CACHE PATH "MuPDF include directory")
    endif()

    # ---------- Auto-detect library ----------
    if(NOT PDFVIEWER_MUPDF_LIB)
        if(MINGW)
            # MinGW: look for libmupdf.a
            set(_mupdf_lib_default
                "${CMAKE_CURRENT_SOURCE_DIR}/third_party/mupdf/lib/mingw/libmupdf.a")
        elseif(MSVC)
            # MSVC: look for .lib
            set(_mupdf_lib_default
                "${CMAKE_CURRENT_SOURCE_DIR}/third_party/mupdf/lib/x64/Release/libmupdf.lib")
        else()
            # Generic Linux / other
            set(_mupdf_lib_default
                "${CMAKE_CURRENT_SOURCE_DIR}/third_party/mupdf/lib/libmupdf.a")
        endif()
        set(PDFVIEWER_MUPDF_LIB "${_mupdf_lib_default}"
            CACHE FILEPATH "MuPDF static library")
    endif()

    # ---------- Validate ----------
    if(NOT EXISTS "${PDFVIEWER_MUPDF_INCLUDE_DIR}/mupdf/fitz.h")
        message(FATAL_ERROR
            "PDFVIEWER_USE_MUPDF=ON but mupdf/fitz.h not found at:\n"
            "  ${PDFVIEWER_MUPDF_INCLUDE_DIR}/mupdf/fitz.h\n"
            "See third_party/mupdf/README.md for setup instructions.")
    endif()

    message(STATUS "MuPDF: enabled")
    message(STATUS "  Include: ${PDFVIEWER_MUPDF_INCLUDE_DIR}")
    message(STATUS "  Library: ${PDFVIEWER_MUPDF_LIB}")
else()
    message(STATUS "MuPDF: disabled (use -DPDFVIEWER_USE_MUPDF=ON to enable)")
endif()

# -----------------------------------------------------------------------------
# PDFViewer executable
# -----------------------------------------------------------------------------
set(PDF_BACKEND_SOURCES
    src/pdf/PdfRendererFactory.cpp
    src/pdf/PageRenderCache.cpp
    src/pdf/PdfRenderService.cpp
)
if(PDFVIEWER_USE_QTPDF)
    list(APPEND PDF_BACKEND_SOURCES src/pdf/QtPdfRenderer.cpp)
endif()
if(PDFVIEWER_USE_MUPDF)
    list(APPEND PDF_BACKEND_SOURCES src/pdf/MuPdfRenderer.cpp)
endif()

set(PDFVIEWER_UI_SOURCES
    src/ui/mainwindow.cpp
    src/ui/tabmanager.cpp
    src/ui/homepagewidget.cpp
    src/ui/locationssidebarwidget.cpp
    src/ui/homecontentwidget.cpp
    src/ui/recentfileswidget.cpp
    src/ui/advancedsearchdialog.cpp
    src/ui/filepropertiesdialog.cpp
    src/ui/settingsdialog.cpp
    src/ui/pdftestwidget.cpp
    src/ui/pdf/PdfViewerWidget.cpp
    src/ui/pdf/PageCanvasView.cpp
    src/ui/pdf/ThumbnailSidebarWidget.cpp
)

if(PDFVIEWER_USE_QTPDF)
    list(APPEND PDFVIEWER_UI_SOURCES
        src/ui/simplepdfwindow.cpp
        src/ui/pdftabwidget.cpp
    )
endif()

qt_add_executable(PDFViewer
    src/main.cpp
    src/application.cpp
    src/config/debuglog.cpp
    src/config/theme.cpp
    src/config/sessionmanager.cpp
    src/config/recentfilesmanager.cpp
    src/config/thumbnailcache.cpp
    src/config/fileclipboard.cpp
    src/config/settingsmanager.cpp
    ${PDFVIEWER_UI_SOURCES}
    ${PDF_BACKEND_SOURCES}
)

target_include_directories(PDFViewer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/pdf
    ${CMAKE_CURRENT_SOURCE_DIR}/src/config
    ${CMAKE_CURRENT_SOURCE_DIR}/src/pdf
)

target_link_libraries(PDFViewer PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Concurrent
)

if(TARGET Qt6::Svg)
    target_link_libraries(PDFViewer PRIVATE Qt6::Svg)
    message(STATUS "Qt6 Svg (toolbar icons): enabled")
else()
    message(STATUS "Qt6 Svg (toolbar icons): not found (optional)")
endif()

if(TARGET Qt6::PrintSupport)
    target_link_libraries(PDFViewer PRIVATE Qt6::PrintSupport)
    target_compile_definitions(PDFViewer PRIVATE PDFVIEWER_HAS_PRINT)
    message(STATUS "Qt6 PrintSupport: enabled")
else()
    message(STATUS "Qt6 PrintSupport: not found (optional) — print will be disabled")
endif()

if(PDFVIEWER_USE_QTPDF)
    target_compile_definitions(PDFViewer PRIVATE PDFVIEWER_USE_QTPDF)
    target_link_libraries(PDFViewer PRIVATE Qt6::Pdf Qt6::PdfWidgets)
endif()

if(PDFVIEWER_USE_MUPDF)
    target_compile_definitions(PDFViewer PRIVATE PDFVIEWER_USE_MUPDF)
    target_include_directories(PDFViewer PRIVATE ${PDFVIEWER_MUPDF_INCLUDE_DIR})
    target_link_libraries(PDFViewer PRIVATE ${PDFVIEWER_MUPDF_LIB})

    # MuPDF on Windows also links against these system DLLs.
    if(WIN32)
        target_link_libraries(PDFViewer PRIVATE
            gdi32 user32 comdlg32 advapi32 shell32)
    endif()

    # MinGW needs a few extra runtime libs that MuPDF's static lib may reference.
    if(MINGW)
        target_link_libraries(PDFViewer PRIVATE
            -static-libgcc -static-libstdc++)
    endif()
endif()

# Optional: enable lightweight debug logging to a file under AppDataLocation.
# Define PDFVIEWER_ENABLE_DEBUGLOG at configure time to turn this on:
#   -D PDFVIEWER_ENABLE_DEBUGLOG=ON
if(PDFVIEWER_ENABLE_DEBUGLOG)
    target_compile_definitions(PDFViewer PRIVATE PDFVIEWER_ENABLE_DEBUGLOG)
endif()

# MSVC: required by Qt6 headers
if(MSVC)
    target_compile_options(PDFViewer PRIVATE /Zc:__cplusplus /permissive-)
endif()

qt_add_resources(PDFViewer "app_resources"
    PREFIX "/"
    FILES
        resources/resources.qrc
)

# Embed the application icon in the .exe so it shows in Explorer and pinned taskbar.
if(WIN32)
    target_sources(PDFViewer PRIVATE resources/app.rc)
endif()

# On Windows, suppress the console window for Release/RelWithDebInfo builds only.
# Debug builds keep the console so that qDebug/qInfo/qWarning output and any
# runtime error messages (missing DLLs, Qt warnings, etc.) are visible in cmd.
if(WIN32)
    set_target_properties(PDFViewer PROPERTIES
        WIN32_EXECUTABLE "$<NOT:$<CONFIG:Debug>>"
    )
endif()
