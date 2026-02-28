# INSTRUCTIONS: HOW TO USE WITH CURSOR

## FILES YOU NOW HAVE (3 FILES TOTAL)

1. **CURSOR-Complete-Design-Package.md** ← All design specifications consolidated
2. **CURSOR-Phase-1A-1B-Prompt.md** ← Detailed implementation prompts for Phases 1A & 1B
3. **Updated-Roadmap-Tabs.md** ← Reference roadmap with timeline

---

## SETUP INSTRUCTIONS

### Step 1: Open Cursor
Open Cursor AI on your machine

### Step 2: Create New Project
```bash
mkdir PDFViewer
cd PDFViewer
```

### Step 3: Add Design Documents to Cursor Context

In Cursor, do one of these:

**Option A: Copy-Paste into Chat**
1. Open the file: `CURSOR-Complete-Design-Package.md`
2. Copy the entire contents (Ctrl+A, Ctrl+C)
3. Paste into Cursor chat (first message)
4. Then paste the prompt: `CURSOR-Phase-1A-1B-Prompt.md`

**Option B: Use Codebase Context (Better)**
1. Create `.cursor/context` folder in your project
2. Copy both design files into the project folder
3. Reference them in your prompt: "You have access to the design documents in the project"

**Option C: Create in Project Files**
1. Copy the design content into your project as README files:
   ```
   PDFViewer/
   ├── DESIGN_PACKAGE.md (full design)
   ├── PROMPT_PHASE_1.md (phases 1A & 1B)
   └── ROADMAP.md (timeline reference)
   ```

### Step 4: Start with Phase 1A Prompt

Copy this exact message into Cursor:

```
I'm starting development on a Professional PDF Viewer in Qt6/C++. 

This is a 15-16 week project. I've attached comprehensive design documents with specifications for:
- Complete roadmap and timeline
- UI/UX design with menu bar, tabs, and settings
- Keyboard shortcuts and interaction patterns
- Architecture and class structures

I'm ready to start with PHASE 1A: Modern UI Framework + Dark/Light Theme.

Please help me implement Phase 1A by:
1. Setting up the Qt6 CMake project structure
2. Implementing the ThemeManager for dark/light switching
3. Creating QSS stylesheets for the design system
4. Building the main window scaffold with menu bar, toolbar, status bar
5. Adding the icon system with SVG resources

Phase 1A duration: 2-3 days

Here are the detailed requirements: [PASTE PHASE 1A SECTION FROM PROMPT]
```

### Step 5: Let Cursor Generate Phase 1A Code

Cursor will:
- Generate CMakeLists.txt
- Create directory structure
- Build Theme/Application classes
- Generate QSS files (dark.qss, light.qss, common.qss)
- Create main.cpp and necessary headers
- Set up the project to build and run

### Step 6: Build and Test Phase 1A

```bash
cd PDFViewer
mkdir build
cd build
cmake ..
make
./PDFViewer
```

**Verify:**
- Application window opens (1200x800)
- Dark theme applies on startup
- Menu bar shows placeholder items
- Status bar displays "Ready"
- Icons appear in toolbar
- Theme switching works (menu items)
- No errors in console

### Step 7: Move to Phase 1B

Once Phase 1A is working:

```
Phase 1A is complete! Now I'm ready for PHASE 1B: Menu Bar & Tabbed Interface.

Please implement Phase 1B by:
1. Creating the complete menu structure (File, Edit, View, Tools, Signature, Help)
2. Implementing QTabWidget with tab management
3. Adding all keyboard shortcuts (Ctrl+Tab, Ctrl+1-8, Ctrl+W, etc.)
4. Building tab context menu
5. Implementing session persistence (save/load tabs)
6. Adding QSS styling for tabs and menus

Phase 1B duration: 4-5 days

Here are the detailed requirements: [PASTE PHASE 1B SECTION FROM PROMPT]
```

### Step 8: Continue the Cycle

- Test Phase 1B thoroughly
- When complete, move to Phase 2A (Home Page Scaffolding)
- Use the same workflow for each phase
- Reference the updated roadmap for phase dependencies

---

## WHAT EACH FILE CONTAINS

### CURSOR-Complete-Design-Package.md
**5 complete documents in one file:**
1. Updated Implementation Roadmap (15-16 weeks)
2. Menu Bar & Tabbed Interface Design
3. Settings System Design
4. Quick Reference Keyboard Shortcuts
5. Complete Success Metrics

**Use for:** Reference and context throughout entire project

### CURSOR-Phase-1A-1B-Prompt.md
**Detailed implementation guides for first 1.5 weeks:**
1. Phase 1A objectives, requirements, checklist, testing
2. Phase 1B objectives, requirements, checklist, testing
3. Code structure templates (classes, functions)
4. QSS styling examples
5. Keyboard event handling implementation

**Use for:** Starting the actual development

### Updated-Roadmap-Tabs.md
**Timeline and phase overview:**
- 15-16 week breakdown
- All phases and deliverables
- Integration checklist
- Success metrics
- Quick keyboard shortcut reference

**Use for:** Tracking progress and planning future phases

---

## QUICK REFERENCE: THE WORKFLOW

```
Week 0: Setup
├─ Phase 1A: Modern UI (2-3 days)
│  └─ Output: Buildable Qt6 app with themes
│
└─ Phase 1B: Menu Bar & Tabs (4-5 days)
   └─ Output: Working menu bar and tabbed interface

Week 1-2: File Management
├─ Phase 2A: Home Page Scaffolding
├─ Phase 2B: Recent Files & File Browser
├─ Phase 2C: Search & Filters
└─ Phase 2D: File Operations

Week 3: Settings
└─ Phase 3G: Settings System

Weeks 4-9: PDF Rendering & Editing
├─ Phase 2: PDF Rendering Engine
├─ Phase 3: Basic Viewer UI
├─ Phase 4: Session Persistence
├─ Phase 5: Annotations
├─ Phase 6: Text Editing & Pages
└─ Phase 7: Digital Signatures

Weeks 10-12: Finalization
├─ Phase 8: Save & Export
├─ Phase 9: Windows Installer
└─ Phase 10: Linux Support
```

---

## TIPS FOR SUCCESS

1. **Work Phase by Phase**
   - Don't skip ahead
   - Each phase builds on previous ones
   - Testing between phases is critical

2. **Commit to Git After Each Phase**
   ```bash
   git add .
   git commit -m "Phase 1A: Modern UI Framework complete"
   ```

3. **Keep Cursor Context Updated**
   - If making major changes, update the design doc
   - Reference the design when Cursor gets off track
   - Use "Review and check against design" as a regular prompt

4. **Test Thoroughly**
   - Follow the testing checklist for each phase
   - Test on both Windows and Linux if possible
   - Don't move to next phase until current phase is fully working

5. **Debug with Cursor**
   - "Why does [behavior] not work as expected?"
   - "Help me debug this [error message]"
   - "Check this code against the design requirements"

6. **Ask for Clarifications**
   - "Can you explain the [component] architecture?"
   - "Why did you implement [feature] this way?"
   - "What are the performance implications of [decision]?"

---

## EXPECTED DELIVERABLES BY PHASE

### Phase 1A (Day 1-3)
✅ Qt6 project builds with CMake
✅ Theme manager working (dark/light switching)
✅ QSS stylesheets loaded correctly
✅ Main window with menu bar, toolbar, status bar
✅ Icon system working
✅ No build warnings

### Phase 1B (Day 4-8)
✅ All 6 menus with correct items
✅ All keyboard shortcuts working
✅ Tabbed interface functional
✅ Tab context menu works
✅ Session persistence works
✅ Professional appearance

### Phase 2A-2D (Week 2-3)
✅ Home page with file browser
✅ Recent files with thumbnails
✅ Search functionality
✅ File operations (copy, paste, delete, etc.)

### Phase 3G (Week 3)
✅ Settings system with 5 categories
✅ All settings configurable and persistent
✅ Security settings locked

### Phases 2-7 (Weeks 4-9)
✅ PDF rendering engine
✅ Viewer UI with controls
✅ Annotations system
✅ Text editing and page management
✅ Digital signatures

### Phases 8-10 (Weeks 10-14)
✅ Save/Export functionality
✅ Windows installer
✅ Linux support
✅ Final polish and testing

---

## FILE STRUCTURE AFTER PHASE 1A

```
PDFViewer/
├── CMakeLists.txt
├── build/ (created after cmake)
├── src/
│   ├── main.cpp
│   ├── application.h
│   ├── application.cpp
│   ├── ui/
│   │   ├── mainwindow.h
│   │   ├── mainwindow.cpp
│   │   ├── tabmanager.h (added in Phase 1B)
│   │   ├── tabmanager.cpp (added in Phase 1B)
│   │   └── styles/
│   │       ├── dark.qss
│   │       ├── light.qss
│   │       └── common.qss
│   └── config/
│       ├── theme.h
│       └── theme.cpp
├── resources/
│   └── resources.qrc
└── README.md
```

---

## TROUBLESHOOTING

**"Qt6 not found"**
- Install Qt6: `sudo apt install qt6-base-dev` (Linux)
- Or download from qt.io

**"CMake errors"**
- Make sure you have CMake 3.20+
- Check CMAKE_PREFIX_PATH includes Qt6 location

**"QSS files not loading"**
- Verify files are in src/ui/styles/
- Check resources.qrc references them
- Run `make` to rebuild resources

**"Keyboard shortcuts not working"**
- Check they're registered in keyPressEvent
- Verify QAction connections
- Check modifiers (Ctrl, Shift) are correct

**"Theme not switching"**
- Verify ThemeManager::instance() is correct
- Check qApp->setStyle() is called
- Verify stylesheet path is correct

---

## NEXT STEPS

1. Gather the 3 files created above
2. Set up your PDFViewer project folder
3. Add the design documents to your project
4. Start with Phase 1A prompt
5. Build the project and test
6. Move to Phase 1B once Phase 1A works

**You're ready to build! 🚀**
