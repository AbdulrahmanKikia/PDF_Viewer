#ifndef TABMANAGER_H
#define TABMANAGER_H

#include <QObject>
#include <QString>
#include <QWidget>
#include <QPointF>
#include <QList>

class QTabWidget;
class QMenu;
class PdfViewerWidget;

struct PDFViewerTab {
    QWidget* widget = nullptr;      // PDF viewer or home page widget
    QString filePath;               // Empty if home page
    QString title;                  // Display title
    bool isDirty    = false;        // Has unsaved changes
    int currentPage = 1;            // Current page number (1-based for display)
    float zoomLevel = 100.0f;       // Zoom level percentage
    bool isHomeTab  = false;        // True if this is the home tab

    // Extended per-tab viewer state (populated when the tab is deactivated).
    QPointF    scrollPos;
    bool       sidebarVisible = true;
    QList<int> splitterSizes;

    PDFViewerTab() = default;
    PDFViewerTab(QWidget* w, const QString& path, const QString& t, bool home = false)
        : widget(w), filePath(path), title(t), isHomeTab(home) {}

    // Convenience: returns the PdfViewerWidget* if this is a PDF tab, else nullptr.
    PdfViewerWidget* pdfViewer() const;
};

class TabManager : public QObject
{
    Q_OBJECT

public:
    explicit TabManager(QTabWidget* tabWidget, QObject* parent = nullptr);
    ~TabManager();

    // Tab operations
    void createNewTab(bool isHome = false);
    void openPDFInNewTab(const QString& filePath);
    void closeTab(int index);
    void closeAllTabs();
    void switchToTab(int index);
    void switchToNextTab();
    void switchToPreviousTab();
    void switchToTabNumber(int n);  // 1-8
    void switchToLastTab();

    // Tab queries
    PDFViewerTab* currentTab() const;
    PDFViewerTab* tabAt(int index) const;
    int tabCount() const;
    int currentIndex() const;

    // Tab state management
    void setTabDirty(int index, bool dirty);
    void setTabTitle(int index, const QString& title);
    void setTabPage(int index, int page);
    void setTabZoom(int index, float zoom);

    // Context menu
    void showTabContextMenu(int index, const QPoint& pos);

    // Session persistence helpers
    QList<PDFViewerTab*> getAllTabs() const;
    void restoreTab(const QString& filePath, int currentPage, float zoomLevel, bool isHome, bool isActive);

signals:
    void tabChanged(int index);
    void tabClosed(int index);
    void tabCountChanged(int count);
    void tabTitleChanged(int index, const QString& title);
    void tabDirtyChanged(int index, bool dirty);

private slots:
    void onTabCloseRequested(int index);
    void onCurrentTabChanged(int index);
    void onTabContextMenuRequested(const QPoint& pos);

private:
    void updateTabBar();
    void setupTabContextMenu();
    QString generateTabTitle(const QString& filePath, bool isHome) const;
    int findHomeTabIndex() const;

    QTabWidget* m_tabWidget;
    QList<PDFViewerTab*> m_tabs;
    QMenu* m_contextMenu;
    int m_contextMenuTabIndex;
};

#endif // TABMANAGER_H
