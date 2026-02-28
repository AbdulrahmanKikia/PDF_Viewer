// PdfRendererFactory.h
// Creates the best available IPdfRenderer at runtime.
//
// Priority:
//   1. MuPdfRenderer   (PDFVIEWER_USE_MUPDF defined)
//   2. QtPdfRenderer   (PDFVIEWER_USE_QTPDF defined)
//   3. NullPdfRenderer (always available)

#ifndef PDFRENDERERFACTORY_H
#define PDFRENDERERFACTORY_H

#include "IPdfRenderer.h"
#include <memory>

class PdfRendererFactory
{
public:
    // Returns a heap-allocated renderer (caller owns it).
    static std::unique_ptr<IPdfRenderer> create();

    // Name of the backend that create() will return.
    static QString activeBackendName();
};

#endif // PDFRENDERERFACTORY_H
