#pragma once

#include "ArcadeTile.h"
#include <QWidget>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QPushButton>

namespace wizz::ui::arcade {

class SocialArcade : public QWidget {
    Q_OBJECT

public:
    explicit SocialArcade(QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;

signals:
    void categorySelected(const ArcadeCategory& category);

private slots:
    void toggleCollapse();

private:
    QScrollArea* m_scrollArea;
    QPushButton* m_toggleBtn;
    bool m_isCollapsed;
    void setupTiles(QWidget* container);
};

} // namespace wizz::ui::arcade
