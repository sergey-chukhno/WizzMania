#pragma once

#include "ArcadeTile.h"
#include <QWidget>
#include <QHBoxLayout>

namespace wizz::ui::arcade {

/**
 * @brief SocialArcade — The main collapsible container for ArcadeTiles.
 */
class SocialArcade : public QWidget {
    Q_OBJECT

public:
    explicit SocialArcade(QWidget* parent = nullptr);

signals:
    void categorySelected(const ArcadeCategory& category);

private:
    QHBoxLayout* m_layout;
    void setupTiles();
};

} // namespace wizz::ui::arcade
