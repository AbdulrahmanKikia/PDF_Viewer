# PDFViewer — Known Issues and Notes

This document records issues encountered during development, their causes, and current status. Useful for future maintainers and contributors.

---

## Zoom

### Zoom-to-viewport-center

**Intended behavior:** When zooming in or out (toolbar, menu, Ctrl+wheel), the point at the **center of the screen** should stay fixed — i.e. zoom “into” or “out of” the middle of the viewport.

**Issues encountered:**

1. **MultiPage mode:** The viewer originally used `QPdfView::PageMode::MultiPage` (stacked pages with gaps). In that mode, content is not a single uniformly scaled rectangle; the scroll formula `newScroll = centerContent * (newZoom/oldZoom) - viewportHalfSize` does not hold because of page spacing and gaps. Zoom-to-center was incorrect or jumpy.

2. **Timing:** Applying scroll immediately after `setZoomFactor()` can run before layout and scroll bar maximum are updated. The scroll values may be wrong or clamped incorrectly.

3. **User report:** “Still doesn’t work” — zoom-to-center behavior may still be imperfect or inconsistent in some edge cases.

**Current implementation:**

- **SinglePage mode** is used so the visible content is one page scaled by zoom. The scroll formula is valid for this layout.
- **zoomAnchoredToViewportCenter()**:
  1. Captures viewport center in content coordinates before zoom.
  2. Sets new zoom.
  3. Connects once to `QPdfView::zoomFactorChanged`.
  4. In the slot, schedules a 0ms timer so layout and scroll bar maximum are updated, then applies the computed scroll position.

**Trade-off:** SinglePage mode means only one page is visible at a time. Multi-page scroll mode was removed to get correct zoom-to-center. If zoom-to-center still misbehaves, possible next steps: debug scroll values after layout, try a longer delay, or check Qt DPI/zoom semantics (e.g. QTBUG-119634).

---

## Annotations (removed)

**Context:** An annotation overlay and tools (comment, highlight, underline, draw, eraser) were implemented and then removed to make the viewer read-only.

**Issues that led to removal:**

1. **Text selection broken:** Any overlay on top of `QPdfView` that intercepts mouse events (e.g. `WA_TransparentForMouseEvents` off) blocks `QPdfView` from handling text selection. The fix was to make the overlay transparent to mouse events by default; only when an annotation tool was active did it intercept events.

2. **Annotations drifting:** The overlay’s coordinate transform (viewport pixels ↔ PDF page points) did not match `QPdfView`’s internal layout in all zoom/scroll modes. Annotations appeared to “float” or drift.

3. **Eraser unreliable:** `removeNearest` used a small hit radius; annotations were hard to hit. Increasing the radius helped but did not fully resolve the issue.

4. **Superposition:** Annotations sometimes appeared to superimpose or misalign with the page.

**Resolution:** All annotation code was stripped. The viewer is now read-only; no overlay, no annotation tools, no coordinate mapping. Text selection and native `QPdfView` interactions work as intended.

---

## Other removed features

**Save As / Export:**

- Removed by design; the viewer is read-only.
- No Save As or Export as images.

**Signatures:**

- Signature menu and related actions were removed.
- No signing, verification, or certificate management.

**Page order:**

- No PageOrder, delete/duplicate/move page.
- Session no longer stores annotations or page order.

---

## Build and environment

- **PowerShell:** Use `;` instead of `&&` for chaining commands in PowerShell.
- **Build directory:** Use `build-msvc` on Windows (not `build`) if that’s what the project expects.
- **QMessageBox buttons:** `QMessageBox::addButton()` returns `QPushButton*`; use that type for `QAbstractButton*` when assigning to avoid MSVC conversion errors.

---

## Qt PDF specifics

- **Zoom factor:** Qt docs say zoom factor is not simply points-per-pixel; it involves DPI. See QTBUG-119634 and forum discussions.
- **QPdfView** inherits from `QAbstractScrollArea`; scroll bars and viewport are available for custom zoom logic.

---

## Summary

| Area | Issue | Status |
|------|-------|--------|
| Zoom | Zoom-to-center unreliable in MultiPage | Switched to SinglePage; zoom-to-center implemented |
| Zoom | Zoom-to-center may still misbehave | Documented; may need further tuning |
| Annotations | Broke text selection | Removed; viewer is read-only |
| Annotations | Drifting, eraser, superposition | Removed |
| Save/Export | Removed by design | N/A |
| Signatures | Removed by design | N/A |
