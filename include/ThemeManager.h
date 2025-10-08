/**
 * ThemeManager.h - UI theme manager
 * 
 * Singleton for managing Light/Dark themes, system palette, and Qt stylesheets.
 * Provides application-wide theme coordination.
 */

#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include <QString>
#include <QApplication>

class ThemeManager {
public:
    enum Theme {
        Light,
        Dark
    };

    static ThemeManager& instance();

    void setTheme(Theme theme);
    Theme getCurrentTheme() const { return m_currentTheme; }
    QString getCurrentThemeName() const;

    void applyTheme(QApplication *app);
    QString getLightStyleSheet() const;
    QString getDarkStyleSheet() const;

private:
    ThemeManager();
    ~ThemeManager() = default;
    ThemeManager(const ThemeManager&) = delete;
    ThemeManager& operator=(const ThemeManager&) = delete;

    Theme m_currentTheme;
};

#endif // THEMEMANAGER_H
