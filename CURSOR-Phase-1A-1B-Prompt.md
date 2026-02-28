# CURSOR PROMPT - PHASE 1A & 1B COMBINED

## INITIAL SETUP & CONTEXT

You are helping a Computer Engineering student build a professional PDF Viewer application in Qt6/C++. This is a comprehensive 15-16 week project with multiple phases. The app will have:

- Classic menu bar with 6 menus (File, Edit, View, Tools, Signature, Help)
- Tabbed interface for multi-document support
- Modern home page with file browser
- PDF rendering with annotations
- Digital signatures
- Settings system with customization
- Windows/Linux support

Location: Montreal, Quebec, Canada
Project: PDFViewer Application
Target: Production-ready desktop application
Timeline: 15-16 weeks total

---

## PHASE 1A: MODERN UI FRAMEWORK + DARK/LIGHT THEME (Week 1 - 0.5 weeks)

Duration: 2-3 days

### OBJECTIVES:

Create the foundation for a modern Qt6 desktop application with responsive layouts, design system, and dark/light theme support.

### REQUIREMENTS:

1. **Qt6 Project Structure**
   - Set up CMake build system (NOT qmake)
   - Create directory structure:
     ```
     PDFViewer/
     ├── CMakeLists.txt
     ├── src/
     │   ├── main.cpp
     │   ├── application.h
     │   ├── application.cpp
     │   ├── ui/
     │   │   ├── mainwindow.h
     │   │   ├── mainwindow.cpp
     │   │   └── styles/
     │   │       ├── dark.qss
     │   │       ├── light.qss
     │   │       └── common.qss
     │   └── config/
     │       ├── theme.h
     │       └── theme.cpp
     ├── resources/
     │   └── resources.qrc
     └── CMakeLists.txt
     ```

2. **CMakeLists.txt**
   - Qt6 components: Core, Gui, Widgets, Svg (for icons)
   - C++17 standard
   - Proper Qt moc/uic configuration
   - Windows/Linux cross-platform support
   - Link necessary Qt libraries

3. **Main Application Class**
   ```cpp
   class PDFViewerApp : public QApplication {
       Q_OBJECT
   public:
       PDFViewerApp(int argc, char* argv[]);
       ~PDFViewerApp();
       
       static PDFViewerApp* instance();
       void setTheme(const QString& themeName);
       QString currentTheme() const;
       
   signals:
       void themeChanged(const QString& themeName);
   };
   ```

4. **Design System (QSS Stylesheets)**
   - Create consistent color palette:
     * Dark theme: Base background #1e1e1e, Text #ffffff
     * Light theme: Base background #ffffff, Text #1a1a1a
     * Accent colors: Blue, Teal, Green, Purple, Orange
   
   - Style all components:
     * QPushButton (normal, hover, pressed, disabled)
     * QLineEdit (normal, focus, error)
     * QMenuBar and menus
     * QTabWidget (tabs, tab bar)
     * QSlider
     * QComboBox
     * QCheckBox, QRadioButton
     * QScrollBar
     * QStatusBar
   
   - Files:
     * common.qss: Shared styles, typography, margins
     * dark.qss: Dark theme specific colors and textures
     * light.qss: Light theme specific colors and textures

5. **Theme Manager Class**
   ```cpp
   class ThemeManager : public QObject {
       Q_OBJECT
   public:
       enum Theme { Light, Dark, System };
       
       static ThemeManager* instance();
       
       void applyTheme(Theme theme);
       void loadStylesheet(const QString& path);
       QString getThemeColor(const QString& colorName);
       Theme currentTheme() const;
       
       // System theme detection
       bool shouldUseDarkTheme() const;
       
   signals:
       void themeChanged(Theme theme);
   };
   ```

6. **Main Window Scaffold**
   ```cpp
   class MainWindow : public QMainWindow {
       Q_OBJECT
   public:
       MainWindow(QWidget* parent = nullptr);
       ~MainWindow();
       
   protected:
       void changeEvent(QEvent* event) override;
       
   private:
       void setupUI();
       void createMenuBar();
       void createToolBar();
       void createStatusBar();
       void connectSignals();
   };
   ```
   - Basic menu bar (menus created but empty for now)
   - Empty toolbar
   - Empty status bar
   - Central widget placeholder

7. **Icon System**
   - Create QRC resource file for icons
   - SVG icons for scalability:
     * File operations: new, open, save, print, export
     * Edit: undo, redo, cut, copy, paste
     * View: zoom in/out, fit width, fit page, full screen
     * Tools: signature, annotations
     * Settings: gear icon
     * Navigation: next, previous, home
   - 16x16, 24x24, 32x32 sizes

8. **Application Configuration**
   - Create config files:
     * app.conf: App name, version, organization
     * Default settings paths
   - Store in:
     * Windows: AppData/Roaming/PDFViewer/
     * Linux: ~/.config/PDFViewer/

### IMPLEMENTATION CHECKLIST:

- [ ] CMake project builds successfully
- [ ] Qt6 configured with proper components
- [ ] Theme manager switches between dark/light
- [ ] QSS stylesheets load without errors
- [ ] All UI components styled consistently
- [ ] Main window displays with proper layout
- [ ] Icons load and display correctly
- [ ] Application follows platform conventions
- [ ] No build warnings (resolve all)
- [ ] Can run `cmake .. && make && ./pdfviewer` successfully

### TESTING:

```bash
# Build project
cd build
cmake ..
make

# Run application
./PDFViewer

# Verify:
1. Window opens with proper size (1200x800)
2. Dark theme applies on startup
3. Light theme switches correctly
4. Menu bar shows placeholder items
5. Status bar displays "Ready"
6. Icons appear in toolbar
7. No console errors or warnings
```

### NEXT STEP:

Once Phase 1A is complete, proceed to **Phase 1B: Menu Bar & Tabbed Interface**.

---

## PHASE 1B: MENU BAR & TABBED INTERFACE (Weeks 1-2 - 1.5 weeks)

Duration: 4-5 days

### OBJECTIVES:

Implement classic menu bar with 6 menus and tabbed interface for multi-document support with keyboard shortcuts.

### REQUIREMENTS:

#### PART 1: MENU BAR STRUCTURE

1. **QMenuBar Setup**
   ```cpp
   class MenuBar {
   private:
       void createFileMenu();
       void createEditMenu();
       void createViewMenu();
       void createToolsMenu();
       void createSignatureMenu();
       void createHelpMenu();
       
       void connectFileMenuActions();
       void connectEditMenuActions();
       // ... etc
   };
   ```

2. **File Menu** (9 items + separators)
   - New Tab (Ctrl+T)
   - Open File... (Ctrl+O)
   - Open Recent (submenu - populated dynamically)
   - ─────────────
   - Save (Ctrl+S)
   - Save As... (Ctrl+Shift+S)
   - Export (submenu: Images, Text, PDF)
   - ─────────────
   - Print (Ctrl+P)
   - Print Preview
   - ─────────────
   - Properties
   - ─────────────
   - Exit (Ctrl+Q)

3. **Edit Menu** (8 items + separators)
   - Undo (Ctrl+Z)
   - Redo (Ctrl+Y)
   - ─────────────
   - Cut (Ctrl+X)
   - Copy (Ctrl+C)
   - Paste (Ctrl+V)
   - ─────────────
   - Select All (Ctrl+A)
   - Find & Replace (Ctrl+H)
   - ─────────────
   - Preferences (Ctrl+,)

4. **View Menu** (11 items + separators)
   - Zoom In (Ctrl+Plus)
   - Zoom Out (Ctrl-Minus)
   - Fit Width (Ctrl+1)
   - Fit Page (Ctrl+2)
   - ─────────────
   - Navigation Pane (Ctrl+Shift+N)
   - Properties Pane
   - ─────────────
   - Full Screen (F11)
   - Dark Mode (checkable, shows current state)
   - Light Mode (checkable)
   - ─────────────
   - Reload (Ctrl+R)

5. **Tools Menu** (placeholder implementation)
   - Annotations submenu (Add Comment, Highlight, Underline, Draw, Erase)
   - ─────────────
   - Text Recognition (placeholder)
   - Extract Text (placeholder)
   - ─────────────
   - File Management submenu (Rename, Duplicate, Move to Trash, Show in Folder)

6. **Signature Menu** (placeholder implementation)
   - Sign Document (Ctrl+Shift+S alternative)
   - Add Signature Field
   - ─────────────
   - Verify Signatures
   - Remove Signature
   - ─────────────
   - Certificate Manager
   - Digital ID

7. **Help Menu**
   - Getting Started
   - Documentation
   - Keyboard Shortcuts (Ctrl+?)
   - ─────────────
   - Report Bug
   - Request Feature
   - ─────────────
   - About PDFViewer
   - Check for Updates

#### PART 2: TABBED INTERFACE

1. **Tab Management Class**
   ```cpp
   struct PDFViewerTab {
       QWidget* widget;  // PDF viewer or home page
       QString filePath;  // Empty if home page
       QString title;
       bool isDirty;
       int currentPage;
       float zoomLevel;
       bool isHomeTab;
   };
   
   class TabManager : public QObject {
       Q_OBJECT
   public:
       TabManager(QTabWidget* parent);
       
       void createNewTab(bool isHome = false);
       void openPDFInNewTab(const QString& filePath);
       void closeTab(int index);
       void closeAllTabs();
       void switchToTab(int index);
       void switchToNextTab();
       void switchToPreviousTab();
       void switchToTabNumber(int n);  // 1-8
       void switchToLastTab();
       
       PDFViewerTab* currentTab() const;
       PDFViewerTab* tabAt(int index) const;
       int tabCount() const;
       
   signals:
       void tabChanged(int index);
       void tabClosed(int index);
       void tabCountChanged(int count);
       void tabTitleChanged(int index, const QString& title);
       
   private:
       void updateTabBar();
       void setupTabContextMenu();
   };
   ```

2. **Tab Bar Implementation**
   - QTabWidget with custom tab bar
   - Each tab displays:
     * Icon (📄 for PDF, 📁 for home)
     * Filename (truncated to 20 chars if long)
     * Close button (X) on each tab
   - "+" button at right end to create new tab
   - Dropdown menu if >10 tabs
   - Right-click context menu

3. **Tab Visual States**
   ```
   Active Tab:   [📄 invoice.pdf ×]    (Bright background, bold text)
   Inactive Tab: [📄 contract.pdf ×]   (Dim background, normal text)
   Unsaved Tab:  [📄 report.pdf* ×]    (Asterisk indicates unsaved)
   Home Tab:     [📁 Home ×]          (Special icon, fixed position at start)
   ```

4. **Keyboard Event Handling**
   ```cpp
   void MainWindow::keyPressEvent(QKeyEvent* event) {
       if (event->modifiers() & Qt::ControlModifier) {
           // Ctrl+T: New Tab
           if (event->key() == Qt::Key_T) {
               tabManager->createNewTab();
               return;
           }
           
           // Ctrl+Tab: Next Tab
           if (event->key() == Qt::Key_Tab) {
               tabManager->switchToNextTab();
               return;
           }
           
           // Ctrl+Shift+Tab: Previous Tab
           if (event->modifiers() & Qt::ShiftModifier && event->key() == Qt::Key_Tab) {
               tabManager->switchToPreviousTab();
               return;
           }
           
           // Ctrl+1 through Ctrl+8: Jump to tab
           if (event->key() >= Qt::Key_1 && event->key() <= Qt::Key_8) {
               int tabNum = event->key() - Qt::Key_1;
               tabManager->switchToTabNumber(tabNum + 1);
               return;
           }
           
           // Ctrl+9: Last tab
           if (event->key() == Qt::Key_9) {
               tabManager->switchToLastTab();
               return;
           }
           
           // Ctrl+W: Close current tab
           if (event->key() == Qt::Key_W) {
               tabManager->closeTab(tabWidget->currentIndex());
               return;
           }
           
           // Ctrl+Shift+W: Close all tabs
           if (event->modifiers() & Qt::ShiftModifier && event->key() == Qt::Key_W) {
               tabManager->closeAllTabs();
               return;
           }
       }
       
       QMainWindow::keyPressEvent(event);
   }
   ```

5. **Tab Context Menu** (Right-click on tab)
   ```cpp
   QMenu contextMenu;
   contextMenu.addAction("Close Tab");
   contextMenu.addAction("Close Other Tabs");
   contextMenu.addAction("Close Tabs to the Right");
   contextMenu.addSeparator();
   contextMenu.addAction("Duplicate Tab");
   contextMenu.addSeparator();
   contextMenu.addAction("Reload Tab");
   contextMenu.addSeparator();
   contextMenu.addAction("Pin Tab (optional)");
   ```

6. **Tab State Management**
   - Each tab maintains:
     * Current page number
     * Zoom level
     * Scroll position
     * Dirty flag (unsaved changes)
   - Switching tabs restores all state

7. **Menu Action Implementation** (Basic - more details later)
   - File > New Tab: Calls `tabManager->createNewTab(false)`
   - File > New Home Tab: Calls `tabManager->createNewTab(true)`
   - File > Open File: Opens file dialog, then `tabManager->openPDFInNewTab(path)`
   - File > Open Recent: Shows recent files, opens in new tab
   - View > Dark Mode: Calls `ThemeManager->applyTheme(Dark)`
   - Edit > Preferences: Opens settings dialog (placeholder)
   - Help > Keyboard Shortcuts: Shows shortcuts dialog (placeholder)
   - Other menu items: Add to menus but connect to placeholder functions for now

#### PART 3: SESSION PERSISTENCE (BASIC)

1. **Session Save File Format** (JSON)
   ```json
   {
     "tabs": [
       {
         "type": "home",
         "isActive": false
       },
       {
         "type": "pdf",
         "filePath": "/path/to/invoice.pdf",
         "currentPage": 3,
         "zoomLevel": 125,
         "isActive": true
       },
       {
         "type": "pdf",
         "filePath": "/path/to/contract.pdf",
         "currentPage": 1,
         "zoomLevel": 100,
         "isActive": false
       }
     ],
     "activeTabIndex": 1
   }
   ```

2. **Session Manager Class**
   ```cpp
   class SessionManager : public QObject {
       Q_OBJECT
   public:
       static SessionManager* instance();
       
       bool saveSession(const QString& path);
       bool loadSession(const QString& path);
       
       void setTabState(int index, const PDFViewerTab& tab);
       PDFViewerTab getTabState(int index) const;
       
   private:
       QJsonDocument sessionData;
   };
   ```

3. **On Application Exit**
   - Save current tabs to JSON
   - Store in: `~/.config/PDFViewer/last_session.json`

4. **On Application Start**
   - Load `last_session.json` if exists
   - Restore all tabs
   - Restore active tab
   - Restore each tab's state (page, zoom)

### STYLING (QSS ADDITIONS)

Add to dark.qss and light.qss:

```qss
/* QTabWidget */
QTabWidget::pane {
    border: 1px solid #cccccc;  /* light: #e0e0e0 */
}

QTabBar::tab {
    background-color: #2e2e2e;  /* light: #f0f0f0 */
    color: #ffffff;              /* light: #000000 */
    padding: 8px 20px;
    margin-right: 2px;
    border-radius: 4px 4px 0 0;
}

QTabBar::tab:selected {
    background-color: #404040;  /* light: #ffffff */
    color: #00bfff;              /* light: #0084ff (accent) */
}

QTabBar::tab:hover:!selected {
    background-color: #353535;  /* light: #f5f5f5 */
}

/* Tab close button */
QTabBar::close-button {
    margin-left: 5px;
}

/* Menu styling */
QMenuBar {
    background-color: #1e1e1e;  /* light: #ffffff */
    color: #ffffff;              /* light: #000000 */
    border-bottom: 1px solid #3e3e3e;
}

QMenuBar::item:selected {
    background-color: #3e3e3e;  /* light: #e0e0e0 */
}

QMenu {
    background-color: #2e2e2e;  /* light: #ffffff */
    color: #ffffff;              /* light: #000000 */
    border: 1px solid #3e3e3e;
}

QMenu::item:selected {
    background-color: #404040;  /* light: #f0f0f0 */
}
```

### IMPLEMENTATION CHECKLIST:

- [ ] All 6 menus created with correct items
- [ ] All keyboard shortcuts registered and working
- [ ] QTabWidget displays below toolbar
- [ ] Tab bar shows active/inactive states correctly
- [ ] Ctrl+Tab switches between tabs
- [ ] Ctrl+1-8 jump to correct tabs
- [ ] Ctrl+W closes current tab
- [ ] Right-click tab context menu appears
- [ ] Tab close button (X) removes tab
- [ ] "+" button creates new tab
- [ ] Home tab cannot be closed (if it's the last tab)
- [ ] Each tab maintains separate state (page, zoom)
- [ ] Unsaved indicator (*) shows when tab has changes
- [ ] Session saves on exit
- [ ] Session restores on startup
- [ ] Tab titles truncate if too long
- [ ] Menu items have icons (if applicable)
- [ ] All theme colors apply correctly
- [ ] No console warnings or errors

### TESTING:

```bash
# Test keyboard shortcuts
1. Ctrl+T: Creates new tab
2. Ctrl+Tab: Switches to next tab
3. Ctrl+Shift+Tab: Switches to previous tab
4. Ctrl+1-8: Jump to specific tabs
5. Ctrl+9: Jump to last tab
6. Ctrl+W: Close current tab
7. Ctrl+Shift+W: Close all tabs

# Test tab operations
1. Click tab to switch to it
2. Right-click tab for context menu
3. Click X button to close tab
4. Click + button to create new tab

# Test theme switching
1. View > Dark Mode: Changes to dark theme
2. View > Light Mode: Changes to light theme
3. Tab bar updates colors correctly

# Test session persistence
1. Create 3 tabs with different names
2. Close application
3. Reopen application
4. Verify all 3 tabs restored with correct titles
5. Verify active tab is restored
```

### SUCCESS CRITERIA:

✅ Menu bar has all 6 menus with correct items
✅ All keyboard shortcuts work (Ctrl+Tab, Ctrl+1-8, Ctrl+W, etc.)
✅ Tabs display below toolbar
✅ Can create/close/switch tabs
✅ Each tab has independent state
✅ Session persists across restarts
✅ Visual feedback for active/inactive tabs
✅ Context menu on right-click
✅ No errors or warnings
✅ Professional appearance matching theme

### NEXT STEP:

Once Phase 1B is complete, proceed to **Phase 2A: Home Page Scaffolding**.

---

**END OF PHASE 1A & 1B PROMPTS**

Use these prompts sequentially. Start with Phase 1A, then proceed to Phase 1B once Phase 1A is complete.
