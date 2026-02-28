# MuPDF — Third-Party Dependency

PDFViewer uses MuPDF as its **primary PDF rendering backend** for performance.

## Licensing

MuPDF is licensed under the **AGPL-3.0** for open-source projects.  
Commercial licences are available from [Artifex Software](https://artifex.com/).

This folder (`third_party/mupdf/`) keeps all MuPDF artefacts isolated from
the main source tree. Removing or replacing it does not require changing any
UI or business-logic code — just set `PDFVIEWER_USE_MUPDF=OFF` in CMake.

---

## Expected directory layout

```
third_party/mupdf/
  include/
    mupdf/
      fitz.h          ← main public API header
      pdf.h
      ...
  lib/
    x64/Release/
      libmupdf.lib    ← MSVC static import library (or static lib)
    mingw/
      libmupdf.a      ← MinGW static library
```

---

## Option A — Pre-built Windows binaries (quickest)

MuPDF ships pre-built Windows packages on their release page.

### For MSVC (Visual Studio)

1. Download `mupdf-<version>-windows.zip` from https://mupdf.com/releases/
2. Extract so that `include/mupdf/fitz.h` and `lib/x64/Release/libmupdf.lib`
   are present under this folder.
3. Configure CMake:
   ```
   cmake -DPDFVIEWER_USE_MUPDF=ON ..
   ```
   CMake will pick up the library automatically from the default paths above.

### For MinGW (Qt Creator / Qt standalone MinGW)

MinGW cannot link `.lib` files directly. You have two sub-options:

#### Sub-option A1 — Rebuild MuPDF with MinGW (recommended)

```bash
# Install MSYS2 + MinGW-w64 toolchain (or use the one bundled with Qt)
# In the MSYS2 MinGW64 shell:
git clone --depth=1 git://git.ghostscript.com/mupdf.git
cd mupdf
git submodule update --init --depth=1
make HAVE_X11=no HAVE_GLUT=no build=release -j4
# Produces: build/release/libmupdf.a
cp -r include/mupdf   <this_folder>/include/mupdf
cp build/release/libmupdf.a  <this_folder>/lib/mingw/libmupdf.a
```

Then configure:
```
cmake -DPDFVIEWER_USE_MUPDF=ON -G "MinGW Makefiles" ..
```

#### Sub-option A2 — Convert MSVC .lib to .a with `reimp` / `dlltool`

If MuPDF is built as a DLL (not recommended for static linking):
```bash
reimp libmupdf.lib          # produces libmupdf.a + libmupdf.def
dlltool -d libmupdf.def -l libmupdf.a
```
Place the resulting `libmupdf.a` in `third_party/mupdf/lib/mingw/`.

---

## Option B — Build from source inside CMake (advanced)

You can use CMake `ExternalProject_Add` or `FetchContent` to download and
build MuPDF automatically. This is significantly more complex because MuPDF's
build system uses `make` (not CMake). A contributed `CMakeLists.txt` wrapper
is available at https://github.com/ArtifexSoftware/mupdf-cmake.

---

## Fallback behaviour

If `PDFVIEWER_USE_MUPDF=OFF` (the default):
- CMake checks for Qt6::Pdf → uses `QtPdfRenderer` if found.
- Otherwise → uses `NullPdfRenderer` (grey placeholder image).

Both fallbacks compile without any extra dependencies.
