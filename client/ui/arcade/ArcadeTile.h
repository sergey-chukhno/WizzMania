#pragma once

#include "ArcadeCategory.h"
#include "../theme/ThemeEngine.h"
#include <QPushButton>
#include <QPropertyAnimation>

class QGraphicsDropShadowEffect;

namespace wizz::ui::arcade {

class ArcadeTile : public QPushButton {
    Q_OBJECT
    Q_PROPERTY(int lift READ lift WRITE setLift)

public:
    explicit ArcadeTile(const ArcadeCategory& category, QWidget* parent = nullptr);

    ArcadeCategory category() const { return m_category; }
    
    int lift() const { return m_liftValue; }
    void setLift(int lift);

signals:
    void tileClicked(const ArcadeCategory& category);

protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    ArcadeCategory m_category;
    QPropertyAnimation* m_liftAnim;
    int m_liftValue = 0;
    bool m_isPressed = false;
    
    void animateHover(bool hover);
};

} // namespace wizz::ui::arcade
