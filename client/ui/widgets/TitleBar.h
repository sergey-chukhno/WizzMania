#pragma once

#include "../theme/ThemeEngine.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QWidget>

/**
 * @brief TitleBar — Reusable frameless macOS-style title bar.
 *
 * Provides:
 *  - macOS traffic-light buttons (exact Apple behavior: red=quit, yellow=minimize, green=maximize)
 *  - Window dragging via mouse press/move on the bar itself
 *  - Left slot  : icon label or back-button (optional)
 *  - Center slot: title text
 *  - Right slots: up to 3 icon action buttons (configurable per window)
 *
 * Usage:
 *   auto* bar = new TitleBar("My Window", this);
 *   bar->setRightAction(0, QIcon(":/assets/search.png"), "Search", [](){ ... });
 *   mainLayout->addWidget(bar);
 */
class TitleBar : public QWidget {
    Q_OBJECT

public:
    explicit TitleBar(const QString& title,
                      QWidget* parent = nullptr);

    /// Update the center title text at runtime
    void setTitle(const QString& title);

    /// Set a right-side icon action button (index 0–2, left-to-right)
    void setRightAction(int index, const QIcon& icon,
                        const QString& tooltip,
                        std::function<void()> callback);

    /// Show/hide the left slot widget
    void setLeftWidget(QWidget* w);

    /// Tint the background (e.g., to match the window theme)
    void setBackgroundColor(const QColor& color);

signals:
    void closeClicked();
    void minimizeClicked();
    void maximizeClicked();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    void buildLayout();
    void updateTrafficLightVisibility(bool visible);

    // Traffic-light buttons (drawn via QPainter, backed by invisible QPushButton hit areas)
    QPushButton* m_btnClose    = nullptr;
    QPushButton* m_btnMinimize = nullptr;
    QPushButton* m_btnMaximize = nullptr;

    bool m_trafficLightVisible = false;  // only shown on hover (authentic macOS)
    bool m_dragging            = false;
    QPoint m_dragStartPos;

    QLabel*      m_titleLabel  = nullptr;
    QWidget*     m_leftSlot    = nullptr;
    QWidget*     m_rightArea   = nullptr;
    QPushButton* m_rightBtns[3] = { nullptr, nullptr, nullptr };

    QColor m_bgColor;
};
