/**
 * ThemeManager.cpp - UI theme management
 * 
 * Provides Light/Dark theme support, manages system palette and Qt stylesheets.
 * Singleton pattern for application-wide theme coordination.
 */

#include "ThemeManager.h"
#include "Logger.h"

ThemeManager& ThemeManager::instance() {
    static ThemeManager instance;
    return instance;
}

ThemeManager::ThemeManager()
    : m_currentTheme(Light) {
}

void ThemeManager::setTheme(Theme theme) {
    m_currentTheme = theme;
    LOG_INFO(QString("Theme changed to: %1").arg(getCurrentThemeName()));
}

QString ThemeManager::getCurrentThemeName() const {
    return (m_currentTheme == Light) ? "Light" : "Dark";
}

void ThemeManager::applyTheme(QApplication *app) {
    if (!app) {
        LOG_ERROR("Cannot apply theme: QApplication is null");
        return;
    }

    QString styleSheet = (m_currentTheme == Light) ? getLightStyleSheet() : getDarkStyleSheet();
    app->setStyleSheet(styleSheet);
    LOG_DEBUG(QString("Applied %1 theme stylesheet").arg(getCurrentThemeName()));
}

QString ThemeManager::getLightStyleSheet() const {
    return R"(
        QMainWindow {
            background-color: #f5f5f5;
        }

        QTextEdit {
            background-color: #ffffff;
            color: #333333;
            border: 1px solid #cccccc;
            border-radius: 4px;
            padding: 8px;
            font-family: 'Segoe UI', Arial, sans-serif;
            font-size: 10pt;
        }

        QLineEdit {
            background-color: #ffffff;
            color: #333333;
            border: 1px solid #cccccc;
            border-radius: 4px;
            padding: 8px;
            font-family: 'Segoe UI', Arial, sans-serif;
            font-size: 10pt;
        }

        QLineEdit:focus {
            border: 2px solid #0078d4;
        }

        QPushButton {
            background-color: #0078d4;
            color: #ffffff;
            border: none;
            border-radius: 4px;
            padding: 10px 20px;
            font-family: 'Segoe UI', Arial, sans-serif;
            font-size: 10pt;
            font-weight: bold;
        }

        QPushButton:hover {
            background-color: #106ebe;
        }

        QPushButton:pressed {
            background-color: #005a9e;
        }

        QPushButton:disabled {
            background-color: #cccccc;
            color: #666666;
        }

        QMenuBar {
            background-color: #ffffff;
            color: #333333;
        }

        QMenuBar::item:selected {
            background-color: #e5e5e5;
        }

        QMenu {
            background-color: #ffffff;
            color: #333333;
            border: 1px solid #cccccc;
        }

        QMenu::item:selected {
            background-color: #0078d4;
            color: #ffffff;
        }
    )";
}

QString ThemeManager::getDarkStyleSheet() const {
    return R"(
        QMainWindow {
            background-color: #1e1e1e;
        }

        QTextEdit {
            background-color: #2d2d2d;
            color: #e0e0e0;
            border: 1px solid #3f3f3f;
            border-radius: 4px;
            padding: 8px;
            font-family: 'Segoe UI', Arial, sans-serif;
            font-size: 10pt;
        }

        QLineEdit {
            background-color: #2d2d2d;
            color: #e0e0e0;
            border: 1px solid #3f3f3f;
            border-radius: 4px;
            padding: 8px;
            font-family: 'Segoe UI', Arial, sans-serif;
            font-size: 10pt;
        }

        QLineEdit:focus {
            border: 2px solid #0078d4;
        }

        QPushButton {
            background-color: #0078d4;
            color: #ffffff;
            border: none;
            border-radius: 4px;
            padding: 10px 20px;
            font-family: 'Segoe UI', Arial, sans-serif;
            font-size: 10pt;
            font-weight: bold;
        }

        QPushButton:hover {
            background-color: #106ebe;
        }

        QPushButton:pressed {
            background-color: #005a9e;
        }

        QPushButton:disabled {
            background-color: #3f3f3f;
            color: #888888;
        }

        QMenuBar {
            background-color: #2d2d2d;
            color: #e0e0e0;
        }

        QMenuBar::item:selected {
            background-color: #3f3f3f;
        }

        QMenu {
            background-color: #2d2d2d;
            color: #e0e0e0;
            border: 1px solid #3f3f3f;
        }

        QMenu::item:selected {
            background-color: #0078d4;
            color: #ffffff;
        }
    )";
}
