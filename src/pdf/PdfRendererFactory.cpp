// PdfRendererFactory.cpp
#include "PdfRendererFactory.h"
#include "NullPdfRenderer.h"

#ifdef PDFVIEWER_USE_MUPDF
#  include "MuPdfRenderer.h"
#endif

#ifdef PDFVIEWER_USE_QTPDF
#  include "QtPdfRenderer.h"
#endif

std::unique_ptr<IPdfRenderer> PdfRendererFactory::create()
{
#ifdef PDFVIEWER_USE_MUPDF
    return std::make_unique<MuPdfRenderer>();
#elif defined(PDFVIEWER_USE_QTPDF)
    return std::make_unique<QtPdfRenderer>();
#else
    return std::make_unique<NullPdfRenderer>();
#endif
}

QString PdfRendererFactory::activeBackendName()
{
#ifdef PDFVIEWER_USE_MUPDF
    return QStringLiteral("MuPDF");
#elif defined(PDFVIEWER_USE_QTPDF)
    return QStringLiteral("Qt PDF");
#else
    return QStringLiteral("Null (no backend compiled)");
#endif
}
