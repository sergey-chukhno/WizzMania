#pragma once

#include <QColor>
#include <QFont>
#include <QFontDatabase>
#include <QGraphicsDropShadowEffect>
#include <QString>
#include <QWidget>

namespace wizz {
namespace ui {

/**
 * @brief ThemeEngine — Centralized design system for WizzMania Pro.
 *
 * All colors, fonts, spacing, radii, and shadow presets live here.
 * Every color is a static getter function (never a static data member)
 * to guarantee construction happens AFTER QApplication is initialized.
 *
 * Usage:
 *   label->setFont(ThemeEngine::fontBody());
 *   painter.setBrush(ThemeEngine::surface());
 *   effect = ThemeEngine::shadow(2, this);
 */
struct ThemeEngine {

  // ── Typography ─────────────────────────────────────────────────────────────

  /// Inter Bold 18px — window / section headers
  static QFont fontHeader() {
    QFont f("Inter", 18, QFont::Bold);
    f.setStyleHint(QFont::SansSerif);
    return f;
  }

  /// Inter SemiBold 15px — contact names, dialog titles
  static QFont fontTitle() {
    QFont f("Inter", 15, QFont::DemiBold);
    f.setStyleHint(QFont::SansSerif);
    return f;
  }

  /// Inter Regular 13px — body text, messages
  static QFont fontBody() {
    QFont f("Inter", 13, QFont::Normal);
    f.setStyleHint(QFont::SansSerif);
    return f;
  }

  /// Inter Regular 10px — status lines, sub-text, captions
  static QFont fontCaption() {
    QFont f("Inter", 10, QFont::Normal);
    f.setStyleHint(QFont::SansSerif);
    return f;
  }

  /// JetBrains Mono Regular 11px — timestamps, counters, monospaced data
  static QFont fontMono() {
    QFont f("JetBrains Mono", 11, QFont::Normal);
    f.setStyleHint(QFont::Monospace);
    return f;
  }

  // ── Color Tokens ───────────────────────────────────────────────────────────

  // Surfaces (glass layering)
  static QColor surface()     { return QColor(255, 255, 255, 45);  } // glass card
  static QColor surfaceHigh() { return QColor(255, 255, 255, 80);  } // elevated card
  static QColor surfaceDim()  { return QColor(255, 255, 255, 20);  } // subtle overlay

  // Text (dark navy palette — readable on light glass)
  static QColor onSurface()   { return QColor(26,  37,  48);       } // #1a2530  primary
  static QColor onSurface2()  { return QColor(74,  85, 104);       } // #4a5568  secondary
  static QColor onSurface3()  { return QColor(113, 128, 150);      } // #718096  muted

  // Accent (blue)
  static QColor accent()      { return QColor(64, 153, 255);        } // #4099FF  primary accent
  static QColor accentDark()  { return QColor(37, 99,  235);        } // #2563EB  pressed/active

  // Accent (purple — sovereign / premium)
  static QColor accentAlt()   { return QColor(124, 58, 237);        } // #7C3AED

  // Semantic
  static QColor success()     { return QColor(34,  197, 94);        } // #22C55E  online / sent
  static QColor warning()     { return QColor(245, 158, 11);        } // #F59E0B  away
  static QColor danger()      { return QColor(239, 68,  68);        } // #EF4444  busy / badge
  static QColor divider()     { return QColor(0,   0,   0,   20);   } // subtle separator

  // Chat bubbles
  static QColor bubbleSelf()  { return QColor(64, 153, 255, 220);   } // blue, semi-transparent
  static QColor bubbleOther() { return QColor(255, 255, 255, 180);  } // white glass
  static QColor bubbleTextSelf()  { return QColor(255, 255, 255);   }
  static QColor bubbleTextOther() { return QColor(26, 37, 48);      }

  // ── Spacing (8px grid) ─────────────────────────────────────────────────────
  static constexpr int sp1 = 4;
  static constexpr int sp2 = 8;
  static constexpr int sp3 = 16;
  static constexpr int sp4 = 24;
  static constexpr int sp5 = 32;
  static constexpr int sp6 = 48;

  // ── Border Radii ───────────────────────────────────────────────────────────
  static constexpr int rSm   = 6;
  static constexpr int rMd   = 12;
  static constexpr int rLg   = 18;
  static constexpr int rXl   = 24;
  static constexpr int rFull = 999; // pill / circle

  // ── Contact Row ────────────────────────────────────────────────────────────
  static constexpr int contactRowHeight  = 64;
  static constexpr int contactAvatarSize = 42;

  // ── Title Bar ──────────────────────────────────────────────────────────────
  static constexpr int titleBarHeight    = 42;
  static constexpr int trafficLightSize  = 13;  // diameter of each dot
  static constexpr int trafficLightGap   = 8;   // gap between dots
  static constexpr int trafficLightLeft  = 14;  // left margin

  // macOS traffic-light colors (exact Apple values)
  static QColor trafficRed()    { return QColor(255,  95,  86); }
  static QColor trafficYellow() { return QColor(255, 189,  46); }
  static QColor trafficGreen()  { return QColor( 39, 201,  63); }
  static QColor trafficRim()    { return QColor(  0,   0,   0,  30); } // subtle ring

  // ── Elevation Shadows ──────────────────────────────────────────────────────
  /**
   * Returns a pre-configured QGraphicsDropShadowEffect.
   * level 1 = subtle card shadow (4px)
   * level 2 = raised panel shadow (12px)
   * level 3 = floating window shadow (24px)
   * Caller does NOT take ownership — parent widget does.
   */
  static QGraphicsDropShadowEffect* shadow(int level, QWidget* parent) {
    auto* fx = new QGraphicsDropShadowEffect(parent);
    fx->setOffset(0, 0);
    switch (level) {
      case 1: fx->setBlurRadius(8);  fx->setColor(QColor(0,0,0,40));  break;
      case 2: fx->setBlurRadius(20); fx->setColor(QColor(0,0,0,60));  break;
      case 3: fx->setBlurRadius(40); fx->setColor(QColor(0,0,0,90));  break;
      default: fx->setBlurRadius(8); fx->setColor(QColor(0,0,0,40));  break;
    }
    return fx;
  }

  // ── Global QSS ─────────────────────────────────────────────────────────────
  /**
   * Base-level stylesheet applied to QApplication.
   * Component-level styles are set inline in each widget.
   */
  static QString globalStyleSheet() {
    return R"(
      QWidget {
        font-family: 'Inter', 'Segoe UI', 'SF Pro Display', sans-serif;
        font-size: 13px;
        color: #1a2530;
      }
      QScrollBar:vertical {
        background: transparent;
        width: 5px;
        border-radius: 3px;
      }
      QScrollBar::handle:vertical {
        background: rgba(0,0,0,60);
        border-radius: 3px;
        min-height: 30px;
      }
      QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
        height: 0px;
      }
      QScrollBar:horizontal {
        background: transparent;
        height: 5px;
        border-radius: 3px;
      }
      QScrollBar::handle:horizontal {
        background: rgba(0,0,0,60);
        border-radius: 3px;
        min-width: 30px;
      }
      QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
        width: 0px;
      }
      QToolTip {
        background-color: #1a2530;
        color: white;
        border: none;
        border-radius: 6px;
        padding: 4px 8px;
        font-size: 11px;
      }
      QMenu {
        background-color: rgba(255,255,255,240);
        border: 1px solid rgba(0,0,0,15);
        border-radius: 10px;
        padding: 4px;
      }
      QMenu::item {
        border-radius: 6px;
        padding: 7px 14px;
        font-size: 13px;
        color: #1a2530;
      }
      QMenu::item:selected {
        background-color: rgba(64,153,255,25);
        color: #2563EB;
      }
      QMenu::separator {
        height: 1px;
        background: rgba(0,0,0,12);
        margin: 4px 10px;
      }
    )";
  }

  // ── Font Loading ────────────────────────────────────────────────────────────
  /**
   * Call once in main() AFTER QApplication is created.
   * Loads embedded fonts from Qt resources.
   */
  static void loadFonts() {
    QFontDatabase::addApplicationFont(":/resources/fonts/Inter-Regular.ttf");
    QFontDatabase::addApplicationFont(":/resources/fonts/Inter-Bold.ttf");
    QFontDatabase::addApplicationFont(":/resources/fonts/JetBrainsMono-Regular.ttf");
  }
};

} // namespace ui
} // namespace wizz
