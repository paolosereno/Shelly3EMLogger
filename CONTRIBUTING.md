# Contributing to Shelly Logger

Thank you for your interest in contributing to Shelly Logger! This document provides guidelines for contributing to the project.

## 🎯 How Can I Contribute?

### Reporting Bugs

Before creating bug reports, please check existing issues to avoid duplicates. When creating a bug report, include:

- **Description**: Clear and concise description of the bug
- **Steps to Reproduce**: Detailed steps to reproduce the behavior
- **Expected Behavior**: What you expected to happen
- **Actual Behavior**: What actually happened
- **Screenshots**: If applicable, add screenshots
- **Environment**:
  - OS: Windows version
  - Qt version: 6.9.1
  - Shelly device model: EM3
  - Shelly firmware version
  - Log output: From Help → View Logs (Ctrl+L)

### Suggesting Features

Feature suggestions are welcome! Please include:

- **Use Case**: Why is this feature needed?
- **Description**: Clear description of the feature
- **Mockups**: If applicable, add mockups or wireframes
- **Alternatives**: Have you considered any alternatives?

### Pull Requests

1. **Fork** the repository
2. **Create** a new branch (`git checkout -b feature/amazing-feature`)
3. **Make** your changes
4. **Test** thoroughly with a real Shelly EM3 device
5. **Commit** with clear messages (`git commit -m 'Add amazing feature'`)
6. **Push** to your branch (`git push origin feature/amazing-feature`)
7. **Open** a Pull Request

## 🛠️ Development Setup

### Prerequisites

- **Qt 6.9.1** with MinGW 64-bit (13.1.0)
- **QCustomPlot 2.1.1+** (download separately)
- **Git** for version control
- **Qt Creator** (recommended IDE)
- **Shelly EM3 device** for testing (optional but recommended)

### Building from Source

```bash
# Clone repository
git clone https://github.com/yourusername/shelly-logger.git
cd shelly-logger

# Download QCustomPlot
# Follow instructions in DOWNLOAD_QCUSTOMPLOT.md
# Copy qcustomplot.h and qcustomplot.cpp to project root

# Build with Qt Creator (recommended)
# OR command line:

# Generate Makefile
"C:\Qt\6.9.1\mingw_64\bin\qmake.exe" shelly_logger.pro -spec win32-g++ "CONFIG+=debug"

# Compile
"C:\Qt\Tools\mingw1310_64\bin\mingw32-make.exe" -j16

# Run
debug\shelly_logger.exe
```

## 📝 Code Style Guidelines

### C++ Code

- **Indentation**: 4 spaces (no tabs)
- **Naming**:
  - Classes: `PascalCase` (e.g., `MainWindow`, `ShellyManager`)
  - Methods: `camelCase` (e.g., `onConnectClicked`, `updatePlot`)
  - Private members: `m_camelCase` (e.g., `m_shellyManager`, `m_dataPoints`)
  - Constants: `UPPER_SNAKE_CASE` (e.g., `MAX_DATA_POINTS`)
- **Braces**: Opening brace on same line
  ```cpp
  void myFunction() {
      // code here
  }
  ```
- **Comments**: Use `//` for single-line, `/** */` for Doxygen documentation
- **Header Guards**: Use `#ifndef/#define/#endif` pattern

### Qt Specific

- **Signals/Slots**: Use new-style connections
  ```cpp
  connect(m_connectButton, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
  ```
- **Memory Management**: Use parent-child ownership when possible
- **Logging**: Use `qDebug()`, `qWarning()`, `qCritical()` with centralized `Logger` class
  ```cpp
  LOG_INFO("Connected to Shelly device");
  LOG_ERROR(QString("Failed to connect: %1").arg(error));
  ```
- **Translations**: Wrap user-visible strings with `tr()`
  ```cpp
  QMessageBox::information(this, tr("Success"), tr("Data saved successfully"));
  ```

### Database

- **Queries**: Use prepared statements for security
  ```cpp
  QSqlQuery query(m_db);
  query.prepare("INSERT INTO samples (timestamp, powerA) VALUES (?, ?)");
  query.addBindValue(timestamp);
  query.addBindValue(powerA);
  ```
- **Transactions**: Use transactions for bulk operations
- **Error handling**: Always check query execution and log errors

### Commit Messages

Follow conventional commits format:

```
<type>(<scope>): <subject>

<body>

<footer>
```

**Types**:
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `style`: Code style changes (formatting)
- `refactor`: Code refactoring
- `test`: Adding tests
- `chore`: Build/tooling changes

**Examples**:
```
feat(alarms): Add 3-phase power threshold alarms

- Add independent enable/threshold per phase A, B, C
- Implement horizontal layout with 2-row grid
- Add visual status bar notifications per phase
- Add persistent alarm configuration in QSettings
- Add automatic alarm clear when values normalize

Closes #15
```

```
fix(charts): Fix cursor position on zoom

Fixed cursor tooltip showing incorrect values after zoom.
Now correctly recalculates position based on visible axis range.

Fixes #23
```

## 🧪 Testing

### Manual Testing Checklist

Before submitting a PR, ensure:

- [ ] Application builds without errors (warnings OK if documented)
- [ ] All existing features still work
- [ ] New feature works as expected
- [ ] Database operations don't corrupt existing data
- [ ] Settings persist correctly across restarts
- [ ] Dark theme remains consistent
- [ ] Charts render correctly with real Shelly data
- [ ] No memory leaks in long-running sessions
- [ ] Help → View Logs shows expected output

### Test Cases

When adding new features, test:

1. **Normal usage**: Feature works as designed with typical data
2. **Edge cases**:
   - Empty/null data from Shelly
   - Very high/low power values
   - Network timeouts
   - Database full/corrupted
3. **Error handling**: Invalid inputs, connection failures
4. **Performance**: Long polling sessions (24+ hours)
5. **UI responsiveness**: No freezing with large datasets

### Testing with Shelly EM3

If you have access to a Shelly EM3:
- Test with different polling intervals (5s to 300s)
- Test connection/disconnection cycles
- Test with different load conditions on all 3 phases
- Test alarm triggers with real power variations
- Test database persistence with extended sessions

If you don't have a Shelly EM3:
- Use the mock data generator (if implemented)
- Test UI components independently
- Document that testing was done without real device

## 📚 Documentation

When adding features:

- Update `README.md` with new functionality
- Add inline code comments for complex logic
- Update keyboard shortcuts table if adding shortcuts
- Add screenshots if UI changes significantly
- Update `DOWNLOAD_QCUSTOMPLOT.md` if QCustomPlot integration changes

## 🌍 Translations

The app supports i18n via Qt Linguist:

1. **Add translatable strings**: Wrap in `tr()`
   ```cpp
   QString message = tr("Connection successful");
   ```

2. **Update translation files**:
   ```bash
   "C:\Qt\6.9.1\mingw_64\bin\lupdate.exe" shelly_logger.pro
   ```

3. **Translate**: Open `.ts` files in `translations/` with Qt Linguist

4. **Compile**:
   ```bash
   "C:\Qt\6.9.1\mingw_64\bin\lrelease.exe" shelly_logger.pro
   ```

## 🔧 Architecture Guidelines

### Adding New Tabs

When adding new tabs (like Statistics, History):

1. Create `newtab.h` and `newtab.cpp`
2. Inherit from `QWidget`
3. Use sub-tabs if content is complex
4. Add to `m_mainTabWidget` in `MainWindow`
5. Implement proper cleanup in destructor
6. Add settings persistence if needed

### Database Schema Changes

When modifying database:

1. Add migration code in `DatabaseManager::initialize()`
2. Check schema version before migration
3. Use transactions for schema changes
4. Test with existing databases (backward compatibility)
5. Document schema changes in code comments

### Alarm System

When adding new alarm types:

1. Update `AlarmManager::AlarmType` enum
2. Add configuration in `SettingsTab`
3. Implement check logic in `MainWindow::onDataReceived()`
4. Add persistent storage in QSettings
5. Consider visual (status bar) and audio (beep) notifications

## 🚀 Release Process

(For maintainers)

1. Update version in badge: `README.md`
2. Update version in About dialog
3. Update `CHANGELOG.md` (create if doesn't exist)
4. Test build on clean environment
5. Tag release: `git tag -a v2.5.1 -m "Release version 2.5.1"`
6. Push tags: `git push --tags`
7. Create GitHub release with:
   - Release notes from CHANGELOG
   - Compiled binary (shelly_logger.exe)
   - Required DLLs (Qt runtime)
   - Installation instructions

## 📊 Performance Guidelines

- **Database**: Use batch inserts, index frequently queried columns
- **Charts**: Limit visible data points, use decimation for old data
- **Network**: Implement timeout handling, retry logic
- **Memory**: Clean up old data from `m_dataPoints` vector
- **UI**: Use QTimer for periodic updates, avoid blocking main thread

## ❓ Questions?

If you have questions:

1. Check existing issues and documentation
2. Read `README.md` thoroughly
3. Open a new issue with `[Question]` prefix
4. Be specific and include context

## 📄 License

By contributing, you agree that your contributions will be licensed under the MIT License.

## 🙏 Third-Party Dependencies

This project uses:

- **Qt 6.9.1**: LGPL v3 / Commercial
- **QCustomPlot 2.1.1+**: GPL v3 (compatible with MIT for applications)
- **SQLite**: Public Domain

When contributing, ensure compatibility with these licenses.

---

**Thank you for contributing to Shelly Logger!** ⚡📊✨
