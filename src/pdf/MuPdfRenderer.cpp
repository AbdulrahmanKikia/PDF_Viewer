// MuPdfRenderer.cpp
// Implementation of the MuPDF rendering backend.
//
// Included only when PDFVIEWER_USE_MUPDF is defined. The MuPDF include path
// is injected by CMake via PDFVIEWER_MUPDF_INCLUDE_DIR.
#include "MuPdfRenderer.h"

#ifdef PDFVIEWER_USE_MUPDF

#include <mupdf/fitz.h>     // Full MuPDF public API
#include <QMutexLocker>

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------
namespace {

// Map a MuPDF anti-aliasing level (0–8) from a zoom value.
// Higher zoom benefits from more AA.
int aaLevelForZoom(float zoom)
{
    if (zoom <= 0.5f) return 2;
    if (zoom <= 1.0f) return 4;
    return 8;  // Full AA for zoom > 1.0
}

// Normalise a rotation angle to 0 | 90 | 180 | 270.
int normaliseRotation(int deg)
{
    return ((deg / 90) % 4 * 90 + 360) % 360;
}

// Build the page-to-device transformation matrix.
// Combines: scale-to-dpi → rotate → translate-to-origin.
fz_matrix makeMatrix(float dpiScale, int rotDeg)
{
    fz_matrix m = fz_scale(dpiScale, dpiScale);
    if (rotDeg != 0)
        m = fz_concat(m, fz_rotate(static_cast<float>(rotDeg)));
    return m;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------
MuPdfRenderer::MuPdfRenderer()
{
    // Create the master context with the default allocator and FZ_STORE_DEFAULT
    // memory cap for the internal object store.
    // Passing nullptr for locks is fine: we use our own Qt mutex for doc access.
    m_ctx = fz_new_context(nullptr, nullptr, FZ_STORE_DEFAULT);
    if (m_ctx) {
        // Register handlers for PDF, XPS, CBZ, etc.
        fz_register_document_handlers(m_ctx);
    }
}

MuPdfRenderer::~MuPdfRenderer()
{
    // Order matters: release document first, then context.
    {
        QMutexLocker lk(&m_docMutex);
        releaseDocument();
    }
    if (m_ctx) {
        fz_drop_context(m_ctx);
        m_ctx = nullptr;
    }
}

// ---------------------------------------------------------------------------
// releaseDocument  (must be called with m_docMutex held, OR from destructor)
// ---------------------------------------------------------------------------
void MuPdfRenderer::releaseDocument()
{
    if (m_doc && m_ctx) {
        fz_drop_document(m_ctx, m_doc);
        m_doc = nullptr;
    }
    m_pageCount = 0;
}

// ---------------------------------------------------------------------------
// load
// ---------------------------------------------------------------------------
bool MuPdfRenderer::load(const QString& filePath)
{
    QMutexLocker lk(&m_docMutex);
    m_lastError.clear();

    if (!m_ctx) {
        m_lastError = QStringLiteral("MuPDF context failed to initialise.");
        return false;
    }

    // Close any currently open document.
    releaseDocument();

    // MuPDF expects UTF-8 encoded paths.
    const QByteArray pathUtf8 = filePath.toUtf8();

    fz_try(m_ctx) {
        m_doc       = fz_open_document(m_ctx, pathUtf8.constData());
        m_pageCount = fz_count_pages(m_ctx, m_doc);
    }
    fz_catch(m_ctx) {
        m_lastError = QString::fromUtf8(fz_caught_message(m_ctx));
        releaseDocument();
        return false;
    }

    return true;
}

// ---------------------------------------------------------------------------
// close
// ---------------------------------------------------------------------------
void MuPdfRenderer::close()
{
    QMutexLocker lk(&m_docMutex);
    releaseDocument();
}

// ---------------------------------------------------------------------------
// pageCount
// ---------------------------------------------------------------------------
int MuPdfRenderer::pageCount() const
{
    QMutexLocker lk(&m_docMutex);
    return m_pageCount;
}

// ---------------------------------------------------------------------------
// pageSizePoints
// ---------------------------------------------------------------------------
QSizeF MuPdfRenderer::pageSizePoints(int pageIndex) const
{
    // Snapshot under lock, then do the actual MuPDF work outside the lock
    // using a cloned context.
    fz_document* doc    = nullptr;
    fz_context*  ctx    = nullptr;
    {
        QMutexLocker lk(&m_docMutex);
        if (!m_ctx || !m_doc || pageIndex < 0 || pageIndex >= m_pageCount)
            return {};
        doc = m_doc;
        ctx = fz_clone_context(m_ctx);  // Clone while holding the lock is OK
    }
    if (!ctx) return {};

    QSizeF result;
    fz_try(ctx) {
        fz_page* page   = fz_load_page(ctx, doc, pageIndex);
        fz_rect  bounds = fz_bound_page(ctx, page);
        result = QSizeF(qAbs(bounds.x1 - bounds.x0),
                        qAbs(bounds.y1 - bounds.y0));
        fz_drop_page(ctx, page);
    }
    fz_catch(ctx) {
        result = QSizeF();
    }
    fz_drop_context(ctx);
    return result;
}

// ---------------------------------------------------------------------------
// renderPage  — thread-safe, each call uses its own cloned context
// ---------------------------------------------------------------------------
QImage MuPdfRenderer::renderPage(int pageIndex, QSize targetPx, float zoom, int rotationDeg)
{
    // --- 1. Snapshot document pointer + validate under lock ---
    fz_document* doc = nullptr;
    fz_context*  ctx = nullptr;
    {
        QMutexLocker lk(&m_docMutex);
        m_lastError.clear();
        if (!m_ctx || !m_doc || pageIndex < 0 || pageIndex >= m_pageCount) {
            m_lastError = QStringLiteral("renderPage: invalid state (page %1 of %2)")
                              .arg(pageIndex).arg(m_pageCount);
            return {};
        }
        doc = m_doc;
        // Clone inside the lock so the master context isn't used concurrently.
        ctx = fz_clone_context(m_ctx);
    }

    if (!ctx) {
        QMutexLocker lk(&m_docMutex);
        m_lastError = QStringLiteral("fz_clone_context failed.");
        return {};
    }

    const int rot = normaliseRotation(rotationDeg);

    QImage result;

    fz_try(ctx) {
        // --- 2. Load page and measure its natural size in PDF points ---
        fz_page* page       = fz_load_page(ctx, doc, pageIndex);
        fz_rect  pageBounds = fz_bound_page(ctx, page);

        float pageW = qAbs(pageBounds.x1 - pageBounds.x0);
        float pageH = qAbs(pageBounds.y1 - pageBounds.y0);

        // For 90/270 rotations the rendered width/height are swapped.
        bool swapped  = (rot == 90 || rot == 270);
        float logicW  = swapped ? pageH : pageW;
        float logicH  = swapped ? pageW : pageH;

        // --- 3. Compute pixel dimensions ---
        // Base: zoom * 72dpi → pixels.  72pt = 1in, so pixW = pageW * zoom.
        float pixW = logicW * zoom;
        float pixH = logicH * zoom;

        // Fit inside targetPx (if constrained) without upscaling.
        if (targetPx.width() > 0 && targetPx.height() > 0 && pixW > 0 && pixH > 0) {
            float scaleW = float(targetPx.width())  / pixW;
            float scaleH = float(targetPx.height()) / pixH;
            float scale  = qMin(scaleW, scaleH);
            if (scale < 1.0f) {
                pixW *= scale;
                pixH *= scale;
                zoom *= scale;
            }
        }

        int imgW = qMax(1, qRound(pixW));
        int imgH = qMax(1, qRound(pixH));

        // --- 4. Build the page→device matrix (scale + rotate) ---
        // MuPDF's coordinate system: PDF y-axis points up; screen y-axis points down.
        // fz_rotate() rotates counter-clockwise (MuPDF convention), so to get
        // a clockwise visual rotation we negate. Then translate to move the
        // rotated page origin back into positive territory.
        float scale = zoom;   // zoom == dpi/72 by construction
        fz_matrix mat = fz_scale(scale, scale);
        if (rot != 0) {
            // Rotate about the page centre.
            fz_matrix rotMat = fz_rotate(static_cast<float>(-rot));
            mat = fz_concat(mat, rotMat);
        }

        // After scale+rotate, compute the actual bounding box to find the
        // translation needed to put the content in the (0,0)-(imgW,imgH) box.
        fz_rect bounds = fz_transform_rect(pageBounds, mat);
        fz_matrix translate = fz_translate(-bounds.x0, -bounds.y0);
        mat = fz_concat(mat, translate);

        // --- 5. Set anti-aliasing ---
        fz_set_aa_level(ctx, aaLevelForZoom(zoom));

        // --- 6. Create pixmap and render ---
        // Use BGRA colorspace; on little-endian machines this maps directly
        // to QImage::Format_ARGB32 in memory layout.
        fz_colorspace* cs    = fz_device_bgr(ctx);
        fz_irect       clip  = { 0, 0, imgW, imgH };
        fz_pixmap*     pixmap = fz_new_pixmap_with_bbox(ctx, cs, clip, nullptr, 1 /*alpha*/);

        // White background fill.
        fz_clear_pixmap_with_value(ctx, pixmap, 0xFF);

        fz_device* dev = fz_new_draw_device(ctx, mat, pixmap);
        fz_run_page(ctx, page, dev, mat, nullptr);
        fz_close_device(ctx, dev);
        fz_drop_device(ctx, dev);

        // --- 7. Convert pixmap → QImage (deep copy, so pixmap can be freed) ---
        int          stride  = fz_pixmap_stride(pixmap);
        const uchar* samples = fz_pixmap_samples(ctx, pixmap);
        result = QImage(samples, imgW, imgH, stride, QImage::Format_ARGB32).copy();

        fz_drop_pixmap(ctx, pixmap);
        fz_drop_page(ctx, page);
    }
    fz_catch(ctx) {
        QMutexLocker lk(&m_docMutex);
        m_lastError = QString::fromUtf8(fz_caught_message(ctx));
        result      = QImage();
    }

    fz_drop_context(ctx);
    return result;
}

// ---------------------------------------------------------------------------
// lastError
// ---------------------------------------------------------------------------
QString MuPdfRenderer::lastError() const
{
    QMutexLocker lk(&m_docMutex);
    return m_lastError;
}

#endif // PDFVIEWER_USE_MUPDF
