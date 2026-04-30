#pragma once

#include <QWidget>
#include <QPushButton>
#include <QList>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include "../AppContext.h"

namespace wizz::ui {

/**
 * @brief A futuristic radial navigation hub that blossoms from the WizzMania logo.
 */
class NavigationHub : public QWidget {
    Q_OBJECT
    Q_PROPERTY(qreal expansion READ expansion WRITE setExpansion)

public:
    explicit NavigationHub(QWidget* parent = nullptr);
    
    qreal expansion() const { return m_expansion; }
    void setExpansion(qreal val);

signals:
    void contextRequested(AppContext context);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void toggleMenu();
    void onIconClicked();

private:
    void setupIcons();
    void updateIconPositions();

    QPushButton* m_coreBtn;
    QList<QPushButton*> m_icons;
    QParallelAnimationGroup* m_animGroup;
    
    qreal m_expansion = 0.0; // 0.0 (closed) to 1.0 (fully expanded)
    bool m_isOpen = false;
    
    QWidget* m_blurOverlay = nullptr;
};

} // namespace wizz::ui
