#pragma once

#include <QColor>
#include <QString>

namespace wizz {
namespace ui {

/**
 * @brief StyleConstants - Centralized theme tokens for the Glassmorphism UI
 */
struct StyleConstants {
    // Glass Colors (Light Theme)
    static QColor GLASS_BG()        { return QColor(255, 255, 255, 45); }
    static QColor GLASS_BORDER()    { return QColor(255, 255, 255, 150); }
    static QColor GLASS_HIGHLIGHT() { return QColor(255, 255, 255, 80); }

    // Accent Colors (Soft Blue Palette)
    static QColor ACCENT_NEON_PINK()   { return QColor(233, 30, 99); }
    static QColor ACCENT_NEON_BLUE()   { return QColor(64, 153, 255); }
    static QColor ACCENT_NEON_PURPLE() { return QColor(103, 58, 183); }

    // Text Colors (Dark for readability on light glass)
    static QColor TEXT_PRIMARY()   { return QColor(26, 37, 48); }    // #1a2530
    static QColor TEXT_SECONDARY() { return QColor(74, 85, 104); }   // #4a5568
    static QColor TEXT_MUTED()     { return QColor(93, 109, 126); }  // #5d6d7e

    // Status Colors
    static QColor STATUS_ONLINE() { return QColor(76, 175, 80); }   // Material Green
    static QColor STATUS_AWAY()   { return QColor(255, 152, 0); }   // Material Orange
    static QColor STATUS_BUSY()   { return QColor(244, 67, 54); }   // Material Red

    // QSS Helpers
    static QString getGlobalStyleSheet() {
        return R"(
            QWidget {
                font-family: 'Inter', 'Segoe UI', sans-serif;
            }
            QListWidget {
                background: transparent;
                border: none;
                outline: none;
            }
            QListWidget::item {
                border-radius: 10px;
                margin: 2px;
                padding: 4px;
            }
            QListWidget::item:hover {
                background: rgba(100, 180, 255, 40);
            }
            QListWidget::item:selected {
                background: rgba(80, 160, 255, 80);
            }
            QComboBox {
                background-color: rgba(255, 255, 255, 80);
                border: 1px solid rgba(200, 220, 240, 150);
                border-radius: 10px;
                padding: 4px 10px;
                font-size: 12px;
                color: #2d3748;
            }
            QComboBox:hover {
                border: 1px solid rgba(100, 180, 255, 200);
            }
            QComboBox::drop-down { border: none; }
            QLineEdit {
                background-color: rgba(220, 240, 255, 120);
                border: none;
                border-radius: 12px;
                padding: 5px 12px;
                font-size: 11px;
                color: #2d3748;
                font-style: italic;
            }
            QLineEdit:focus {
                background-color: rgba(255, 255, 255, 200);
                font-style: normal;
            }
        )";
    }
};

} // namespace ui
} // namespace wizz
