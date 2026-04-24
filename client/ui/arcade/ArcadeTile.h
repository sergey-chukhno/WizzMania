#pragma once

#include "ArcadeCategory.h"
#include "../theme/ThemeEngine.h"
#include <QPushButton>
#include <QPropertyAnimation>

namespace wizz::ui::arcade {

/**
 * @brief ArcadeTile — An animated, clickable tile representing a category in the Arcade.
 */
class ArcadeTile : public QPushButton {
    Q_OBJECT

public:
    explicit ArcadeTile(const ArcadeCategory& category, QWidget* parent = nullptr);

    ArcadeCategory category() const { return m_category; }

signals:
    void tileClicked(const ArcadeCategory& category);

protected:
    bool event(QEvent* e) override;

private:
    ArcadeCategory m_category;
    QPropertyAnimation* m_hoverAnim;
    
    void animateHover(bool hover);
};

} // namespace wizz::ui::arcade
