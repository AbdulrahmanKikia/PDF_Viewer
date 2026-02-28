# COMPLETE PDF VIEWER DESIGN PACKAGE FOR CURSOR
## All Documents Consolidated - Ready for Context

This file contains ALL design specifications needed for Cursor AI to implement your PDF Viewer. Copy and paste this entire document into Cursor's context.

---

# DOCUMENT 1: UPDATED IMPLEMENTATION ROADMAP

## Complete Weekly Breakdown (15-16 weeks)

### **WEEKS 1-5: UI FOUNDATION & FILE MANAGEMENT** (Enhanced + New)

#### **Week 1:** Modern UI Framework + Dark/Light Theme (0.5 weeks)
- Set up Qt6 project with responsive layouts
- Implement design system (QSS stylesheets)
- Create theme system (Dark/Light/System)
- **Cursor will help with:** CMake setup, Qt6 configuration, QSS styling

#### **Weeks 1-2:** Menu Bar & Tabbed Interface (1.5 weeks) ✨ NEW
- Create classic QMenuBar with 6 menus (File, Edit, View, Tools, Signature, Help)
- File Menu: New Tab, Open, Recent, Save, Export, Print, Exit
- Edit Menu: Undo, Redo, Cut/Copy/Paste, Preferences
- View Menu: Zoom, Full Screen, Dark Mode, Reload
- Implement QTabWidget with tab bar below toolbar
- Tab management: Create, close, switch tabs
- **Keyboard Shortcuts:**
  - Ctrl+T: New Tab
  - Ctrl+Tab / Ctrl+Shift+Tab: Switch between tabs
  - Ctrl+1 through Ctrl+8: Jump to tab 1-8
  - Ctrl+W: Close current tab
  - Ctrl+Shift+W: Close all tabs
- Tab features: close button (×), unsaved indicator (*), context menu
- Each tab maintains separate state: page number, zoom, scroll position
- Session persistence: restore tabs on app restart
- **Cursor will help with:** QMenuBar/QMenu creation, QTabWidget, keyboard event handling, tab state management

#### **Week 2:** Home Page Scaffolding
- Create main window with sidebar + content split
- Implement view switching (Home ↔ PDF Viewer ↔ Other tabs)
- Build location sidebar with bookmarks
- **Cursor will help with:** QMainWindow, QSplitter, QStackedWidget

#### **Week 3:** Recent Files & File Browser
- Display recent files as cards with thumbnails
- Generate and cache PDF thumbnails
- Create file list/grid/compact views
- Implement folder navigation and breadcrumbs
- **Cursor will help with:** QGridLayout, thumbnail generation, caching

#### **Week 4:** Search & File Operations
- Implement file search with real-time results
- Advanced search dialog (date, size filters)
- Context menu with copy, cut, paste, delete, rename, move
- File properties dialog
- **Cursor will help with:** QFileSystemModel, search algorithms, context menus

#### **Week 5:** Settings System
- Create SettingsManager with QSettings persistence
- Build Settings dialog with 5 category tabs
- Appearance settings (theme, colors, fonts)
- General (tab behavior, session restore), Viewing, Security, Advanced tabs
- Validation and error handling
- **Cursor will help with:** QSettings implementation, settings UI, data validation

### **WEEKS 6-11: PDF RENDERING & EDITING**

#### **Week 6:** PDF Rendering Engine
- MuPDF integration for fast rendering
- Page caching with LRU eviction
- PDFium fallback renderer
- Error handling for corrupted PDFs

#### **Week 7:** Basic PDF Viewer UI
- Canvas widget for page display
- Navigation controls (next, prev, goto page)
- Zoom controls (fit width, fit page, zoom %)
- Thumbnail sidebar

#### **Week 8:** Session Persistence
- Save/restore last file and page **per tab**
- Remember zoom level, scroll position **per tab**
- Recent files database
- Autosave and crash recovery
- Restore all open tabs on app restart

#### **Week 9:** Annotations (Comments, Highlights, Drawing)
- Text comments with timestamps
- Highlight annotations (multiple colors)
- Freehand drawing (ink annotations)
- Undo/redo for all annotations

#### **Week 10:** Text Editing & Page Management
- Extract and edit text in PDFs
- Delete, insert, reorder pages
- Form field detection and filling
- All operations support undo/redo

#### **Week 11:** Digital Signatures
- Certificate management
- Signature field placement
- Sign PDFs with cryptographic hashing
- Signature verification and display

### **WEEKS 12-14: EXPORT, PACKAGING, POLISHING**

#### **Week 12:** Save, Export & Advanced Features
- Save edited PDFs
- Export pages as images
- Export text and form data
- Multi-format support roadmap (DOCX, XLSX, PPTX)

#### **Week 13:** Windows Installer & Deployment
- Create NSIS installer
- File associations (.pdf)
- Start menu shortcuts
- Versioning system
- Update checking

#### **Week 14:** Linux Support & Optimization
- Cross-platform code abstraction
- Linux AppImage creation
- Performance profiling and optimization
- Memory usage tuning
- Final testing and bug fixes

---

# DOCUMENT 2: MENU BAR & TABBED INTERFACE DESIGN

## PART 1: APPLICATION UI LAYOUT

### Main Window Structure

```
┌─────────────────────────────────────────────────────────────┐
│ PDFViewer  File  Edit  View  Tools  Signature  Help         │ ← Menu Bar
├─────────────────────────────────────────────────────────────┤
│ [New] [Open] [Save] [Print] [Edit] | Search [____] | [⚙️]  │ ← Toolbar
├─────────────────────────────────────────────────────────────┤
│ invoice.pdf × │ contract.pdf × │ report.pdf ×  │ [+] [v]  │ ← Tab Bar
├──────────────────────┬──────────────────────────────────────┤
│                      │                                       │
│  📁 Locations        │   PDF Viewer / Home Page Content     │
│  ├─ Recent           │   (depends on which tab is active)   │
│  ├─ Downloads        │                                       │
│  ├─ Documents        │                                       │
│  ├─ Desktop          │                                       │
│  └─ [+ Add]          │                                       │
│                      │                                       │
│                      │   Tab 1: Home Page (file browser)    │
│                      │   Tab 2: PDF Viewer (invoice.pdf)    │
│                      │   Tab 3: PDF Viewer (contract.pdf)   │
│                      │   Tab 4: PDF Viewer (report.pdf)     │
│                      │                                       │
├──────────────────────┴──────────────────────────────────────┤
│ Ready | Pg 1 of 1 | Zoom 100% | 2 Files Open              │ ← Status Bar
└──────────────────────────────────────────────────────────────┘
```

## PART 2: MENU BAR STRUCTURE

### File Menu
```
File
├── New Tab                      Ctrl+T
├── Open File...                 Ctrl+O
├── Open Recent                  →
│   ├── invoice.pdf
│   ├── contract.pdf
│   └── report.pdf
├── ─────────────────
├── Save                         Ctrl+S
├── Save As...                   Ctrl+Shift+S
├── Export                       →
│   ├── Export as Images
│   ├── Export as Text
│   └── Export as PDF
├── ─────────────────
├── Print                        Ctrl+P
├── Print Preview
├── ─────────────────
├── Properties
├── ─────────────────
├── Exit                         Ctrl+Q
```

### Edit Menu
```
Edit
├── Undo                         Ctrl+Z
├── Redo                         Ctrl+Y
├── ─────────────────
├── Cut                          Ctrl+X
├── Copy                         Ctrl+C
├── Paste                        Ctrl+V
├── ─────────────────
├── Select All                   Ctrl+A
├── Find & Replace               Ctrl+H
├── ─────────────────
├── Preferences                  Ctrl+,
```

### View Menu
```
View
├── Zoom In                      Ctrl+Plus
├── Zoom Out                     Ctrl-Minus
├── Fit Width                    Ctrl+1
├── Fit Page                     Ctrl+2
├── ─────────────────
├── Navigation Pane              Ctrl+Shift+N
├── Properties Pane
├── ─────────────────
├── Full Screen                  F11
├── Dark Mode                    (checked/unchecked)
├── Light Mode
├── ─────────────────
├── Reload                       Ctrl+R
```

### Tools Menu
```
Tools
├── Annotations                  (submenu or toggle)
│   ├── Add Comment
│   ├── Highlight
│   ├── Underline
│   ├── Draw
│   └── Erase
├── ─────────────────
├── Text Recognition (OCR)       (if available)
├── Extract Text
├── ─────────────────
├── File Management
│   ├── Rename
│   ├── Duplicate
│   ├── Move to Trash
│   └── Show in Folder
```

### Signature Menu
```
Signature
├── Sign Document                Ctrl+Shift+S (different from Save As)
├── Add Signature Field
├── ─────────────────
├── Verify Signatures
├── Remove Signature
├── ─────────────────
├── Certificate Manager
├── Digital ID
```

### Help Menu
```
Help
├── Getting Started
├── Documentation
├── Keyboard Shortcuts            Ctrl+?
├── ─────────────────
├── Report Bug
├── Request Feature
├── ─────────────────
├── About PDFViewer
├── Check for Updates
```

## PART 3: TABBED INTERFACE IMPLEMENTATION

### Tab Bar Architecture

```cpp
class MainApplication : public QMainWindow {
    QTabWidget* tabWidget;
    std::vector<PDFViewerTab*> openTabs;
    
    // Tab management
    void createNewTab();
    void openPDFInNewTab(const std::string& filePath);
    void closeTab(int tabIndex);
    void closeAllTabs();
    void switchToNextTab();  // Ctrl+Tab
    void switchToPreviousTab();  // Ctrl+Shift+Tab
    void switchToTabNumber(int n);  // Ctrl+1 through Ctrl+8
};

struct PDFViewerTab {
    QWidget* widget;  // Contains PDF viewer or home page
    std::string filePath;  // Empty if home page
    std::string tabTitle;
    bool isDirty;  // Has unsaved changes
    int lastPage;  // Current page for this PDF
    float zoomLevel;
};
```

### Tab Bar Features

**Visual Design:**
- Tab bar below menu bar and toolbar
- Each tab shows: PDF icon + filename (truncated if long)
- Active tab highlighted (brighter background, different color)
- Close button (X) on each tab
- "+" button to create new tab
- Dropdown menu for tab switching (if many tabs)

**Tab Styling:**
```
Active Tab:      [📄 invoice.pdf ×]  (Bright, current tab)
Inactive Tab:    [📄 contract.pdf ×]  (Dim)
Unsaved Tab:     [📄 report.pdf* ×]  (Asterisk indicates unsaved)
Home Tab:        [📁 Home ×]         (Special icon for home page)
```

### Keyboard Shortcuts for Tabs

| Shortcut | Action | Windows |
|----------|--------|---------|
| Ctrl+T | New Tab | ✓ |
| Ctrl+Tab | Next Tab | ✓ |
| Ctrl+Shift+Tab | Previous Tab | ✓ |
| Ctrl+1 to Ctrl+8 | Go to Tab 1-8 | ✓ |
| Ctrl+9 | Go to Last Tab | ✓ |
| Ctrl+W | Close Current Tab | ✓ |
| Ctrl+Shift+W | Close All Tabs | ✓ |

## PART 4: TAB TYPES & STATES

### Home Page Tab
- Always one home page tab available
- First tab by default
- Shows file browser, recent files, search
- Cannot be closed unless another tab is active
- Tab Label: "📁 Home"

### PDF Viewer Tabs
- One tab per open PDF
- Shows PDF viewer with toolbar, sidebar
- Page number and zoom saved per tab
- Can be reordered
- Can be closed
- Tab Label: "📄 filename.pdf" (truncate if > 20 chars)
- Unsaved State: "📄 filename.pdf*" (asterisk indicates unsaved)

### Tab Context Menu (Right-click)
```
Close Tab
Close Other Tabs
Close Tabs to the Right
─────────────────
Duplicate Tab
─────────────────
Reload Tab
Pin Tab (optional)
─────────────────
Show All Tabs (if > 20 tabs)
```

## PART 5: INTEGRATION WITH EXISTING FEATURES

### Session Persistence with Tabs

**Save Session (JSON):**
```json
{
  "tabs": [
    {
      "type": "home",
      "isActive": false
    },
    {
      "type": "pdf",
      "filePath": "/Users/user/Documents/invoice.pdf",
      "currentPage": 3,
      "zoomLevel": 125,
      "isActive": true
    },
    {
      "type": "pdf",
      "filePath": "/Users/user/Documents/contract.pdf",
      "currentPage": 1,
      "zoomLevel": 100,
      "isActive": false
    }
  ],
  "activeTabIndex": 1
}
```

### Settings Integration

**New Settings in "General" tab:**
- Toggle: "Reopen tabs from last session"
- Dropdown: "Tab opening behavior: New tab / Replace current / Ask"
- Toggle: "Show unsaved indicator (asterisk)"

---

# DOCUMENT 3: SETTINGS SYSTEM DESIGN

## OVERVIEW

Settings Manager provides:
1. **Persistent storage** using Qt's QSettings
2. **5 organized categories** in settings dialog
3. **Smart validation** with error prevention
4. **Locked security settings** for protection
5. **Real-time theme switching**
6. **Settings search** to find any setting instantly

## PART 1: SETTINGS ARCHITECTURE

### AppSettings Structure

```cpp
struct AppSettings {
    // Appearance
    struct {
        QString theme;  // "dark", "light", "system"
        QString accentColor;  // Hex color #RRGGBB
        QString fontFamily;  // Arial, Segoe UI, etc.
        int fontSize;  // 10-20pt
        bool showAnimations;
    } appearance;
    
    // General
    struct {
        bool reopenTabsOnStartup;
        QString tabBehavior;  // "new", "replace", "ask"
        bool autoSaveInterval;  // minutes
        QString defaultDownloadLocation;
    } general;
    
    // Viewing
    struct {
        QString zoomLevel;  // "fit-width", "fit-page", percentage
        bool showThumbnails;
        bool enablePageCache;
        int cacheSize;  // 50-500 MB
        QString renderingQuality;  // "fast", "balanced", "best"
    } viewing;
    
    // Security
    struct {
        bool requirePasswordForSigning;  // LOCKED
        QString signingAlgorithm;  // LOCKED (always SHA-256)
        bool enableCertificateValidation;  // LOCKED
        QString certificatePath;
    } security;
    
    // Advanced
    struct {
        bool enableLogging;
        int maxWorkerThreads;  // 1-8
        bool enableExperimentalFeatures;
        QString logLevel;  // "info", "debug", "warning", "error"
    } advanced;
};
```

### SettingsManager Class

```cpp
class SettingsManager {
public:
    SettingsManager();
    
    // Load/Save
    bool loadSettings();
    bool saveSettings();
    
    // Get/Set (type-safe)
    template<typename T>
    T get(const QString& key, const T& defaultValue);
    
    template<typename T>
    bool set(const QString& key, const T& value);
    
    // Validation
    bool validateSetting(const QString& key, const QVariant& value);
    QString getValidationError();
    
    // Locked settings
    bool isSettingLocked(const QString& key);
    
    // Signals
    void settingChanged(const QString& key, const QVariant& value);
    
private:
    QSettings* qsettings;
    AppSettings currentSettings;
    QMap<QString, QPair<QVariant, QVariant>> validationRules;  // min, max
};
```

## PART 2: SETTINGS CATEGORIES

### 1. APPEARANCE TAB
**Customizable user experience**

**Theme Selection:**
- Radio buttons: Dark / Light / System Default
- Changes apply immediately
- Icons preview in real-time

**Accent Color:**
- Color picker (preset colors + custom)
- Defaults: Blue, Teal, Green, Purple, Orange
- Preview in mini UI mockup

**Font Settings:**
- Font family dropdown (system fonts)
- Font size slider (10-20pt)
- Preview text showing current font

**UI Elements Toggle:**
- ☑ Show animations
- ☑ Show tabs count badge
- ☑ Show unsaved indicator (*)

### 2. GENERAL TAB
**Application behavior**

**Session Management:**
- ☑ Reopen tabs from last session
- ☑ Auto-save recent files
- Dropdown: "Tab opening behavior"
  - New tab (default)
  - Replace current tab
  - Ask me each time

**Files & Storage:**
- Dropdown: Default download location
- Button: Choose custom location
- Display: "Documents" (with path)

**Auto-save:**
- ☑ Enable auto-save
- Spinner: Auto-save interval (1-30 minutes)

### 3. VIEWING TAB
**PDF viewing optimization**

**Display Settings:**
- Dropdown: Default zoom
  - Fit width
  - Fit page
  - Custom: [_____]%
- ☑ Show thumbnails sidebar
- ☑ Show page outline

**Rendering:**
- Dropdown: Rendering quality
  - Fast (best performance)
  - Balanced (default)
  - Best (best quality, slower)
- ☑ Enable page caching
- Slider: Cache size (50-500 MB, default 200)

**Shortcuts:**
- Button: [Restore Defaults]

### 4. SECURITY TAB
**🔒 LOCKED & PROTECTED**

**Critical Settings (Cannot be changed):**
- ☐ Require password for signing (ALWAYS ON)
  - Grayed out, locked icon
  - Tooltip: "This setting is always enabled for security"

- ☐ Signing algorithm
  - Display: "SHA-256 (Recommended)"
  - Grayed out, locked icon

- ☐ Certificate validation
  - Display: "Enabled (Always required)"
  - Grayed out, locked icon

**User Configurable:**
- 📁 Certificate storage path
- Button: [Browse] [Reset to Default]
- ☑ Verify certificate chain
- ☑ Check revocation status (if available)

**Certificate Management:**
- Button: [Open Certificate Manager]
  - Opens certificate management dialog
  - View installed certificates
  - Import/export certificates

### 5. ADVANCED TAB
**Power user options**

**Logging:**
- ☑ Enable logging
- Dropdown: Log level
  - Info
  - Debug
  - Warning
  - Error

**Performance Tuning:**
- Slider: Worker threads (1-8, default 4)
  - More threads = faster processing, higher memory
  - Fewer threads = slower processing, lower memory

**Experimental Features:**
- ☑ Enable experimental features
  - OCR (if available)
  - AI-powered search
  - Cloud sync (coming soon)

**Database & Cache:**
- Button: [Clear Cache] (confirms action)
- Button: [Reset All Settings] (requires confirmation)
- Button: [Export Settings] (saves as JSON)
- Button: [Import Settings] (loads from JSON)

---

# DOCUMENT 4: QUICK REFERENCE KEYBOARD SHORTCUTS

## Tab Management
- Ctrl+T: New Tab
- Ctrl+Tab: Next Tab
- Ctrl+Shift+Tab: Previous Tab
- Ctrl+1 to Ctrl+8: Jump to Tab 1-8
- Ctrl+9: Last Tab
- Ctrl+W: Close Tab
- Ctrl+Shift+W: Close All Tabs

## File Operations
- Ctrl+O: Open File
- Ctrl+S: Save
- Ctrl+Shift+S: Save As
- Ctrl+P: Print

## Editing
- Ctrl+Z: Undo
- Ctrl+Y: Redo
- Ctrl+X: Cut
- Ctrl+C: Copy
- Ctrl+V: Paste
- Ctrl+A: Select All
- Ctrl+H: Find & Replace

## Viewing
- Ctrl+Plus: Zoom In
- Ctrl-Minus: Zoom Out
- Ctrl+1: Fit Width
- Ctrl+2: Fit Page
- F11: Full Screen
- Ctrl+R: Reload
- Ctrl+?: Keyboard Shortcuts (Help)

## Application
- Ctrl+,: Preferences/Settings
- Ctrl+Q: Exit/Quit

---

# DOCUMENT 5: COMPLETE SUCCESS METRICS

## Functional Requirements ✅
- Classic menu bar with 6 menus
- Tabbed interface below toolbar
- Multiple PDFs open in separate tabs
- Ctrl+Tab switches between tabs
- Ctrl+1 through Ctrl+8 jump to tabs
- Each tab maintains separate state
- Tab context menu works
- Session persistence restores tabs
- Home page with modern file browser
- Recent files with thumbnails
- File search with advanced filters
- Full file operations
- Settings system with 5 categories
- Appearance customizable
- PDF fast rendering (60+ FPS)
- Annotations support
- Text editing capability
- Digital signatures
- Save/export functionality
- Windows installer
- Linux AppImage support

## Non-Functional Requirements ✅
- Lightweight (< 200MB)
- Fast (1000+ files browsable)
- Low memory (50-100MB typical)
- Modern UI appearance
- Session persistence
- Keyboard-centric
- Security-conscious
- User-friendly

---

**END OF COMPLETE DESIGN PACKAGE**

You now have all 5 documents in one consolidated file. Ready for Cursor context!
