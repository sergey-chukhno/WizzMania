#include "NavigationHub.h"
#include "../theme/ThemeEngine.h"
#include <QPainter>
#include <QPainterPath>
#include <QtMath>
#include <QGraphicsBlurEffect>

namespace wizz::ui {

using TE = ThemeEngine;

NavigationHub::NavigationHub(QWidget* parent) : QWidget(parent) {
    m_coreBtn = new QPushButton(this);
    m_coreBtn->setFixedSize(48, 48);
    m_coreBtn->setCursor(Qt::PointingHandCursor);
    m_coreBtn->setIcon(QIcon(":/assets/butterfly.png"));
    m_coreBtn->setIconSize(QSize(32, 32));
    m_coreBtn->setStyleSheet(R"(
        QPushButton {
            background: rgba(255, 255, 255, 20);
            border: 1px solid rgba(255, 255, 255, 40);
            border-radius: 24px;
        }
        QPushButton:hover {
            background: rgba(255, 255, 255, 40);
            border: 1px solid rgba(255, 255, 255, 80);
        }
    )");
    
    connect(m_coreBtn, &QPushButton::clicked, this, &NavigationHub::toggleMenu);
    
    setupIcons();
    
    m_animGroup = new QParallelAnimationGroup(this);
}

void NavigationHub::setupIcons() {
    struct HubIcon {
        QString icon;
        AppContext context;
        QString tooltip;
    };
    
    QList<HubIcon> configs = {
        {"💬", AppContext::Messenger, "Messenger"},
        {"👥", AppContext::Groups, "Groups & Channels"},
        {"📞", AppContext::Calls, "Calls & Video"},
        {"⚙️", AppContext::Settings, "Settings"}
    };
    
    for (const auto& config : configs) {
        auto* btn = new QPushButton(config.icon, this);
        btn->setFixedSize(40, 40);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setProperty("context", static_cast<int>(config.context));
        btn->setToolTip(config.tooltip);
        btn->setStyleSheet(R"(
            QPushButton {
                background: rgba(20, 20, 30, 200);
                color: white;
                border: 1px solid rgba(255, 255, 255, 30);
                border-radius: 20px;
                font-size: 16px;
            }
            QPushButton:hover {
                background: rgba(64, 153, 255, 200);
                border: 1px solid white;
            }
        )");
        
        connect(btn, &QPushButton::clicked, this, &NavigationHub::onIconClicked);
        btn->hide();
        m_icons.append(btn);
    }
}

void NavigationHub::toggleMenu() {
    m_isOpen = !m_isOpen;
    
    m_animGroup->stop();
    m_animGroup->clear();
    
    auto* anim = new QPropertyAnimation(this, "expansion");
    anim->setDuration(400);
    anim->setStartValue(m_expansion);
    anim->setEndValue(m_isOpen ? 1.0 : 0.0);
    anim->setEasingCurve(m_isOpen ? QEasingCurve::OutBack : QEasingCurve::InBack);
    m_animGroup->addAnimation(anim);
    
    if (m_isOpen) {
        for (auto* btn : m_icons) btn->show();
    } else {
        connect(m_animGroup, &QParallelAnimationGroup::finished, this, [this]() {
            if (!m_isOpen) {
                for (auto* btn : m_icons) btn->hide();
            }
        });
    }
    
    m_animGroup->start();
}

void NavigationHub::setExpansion(qreal val) {
    m_expansion = val;
    updateIconPositions();
    update();
}

void NavigationHub::updateIconPositions() {
    if (m_icons.isEmpty()) return;
    
    QPoint center(width() / 2, height() - 30);
    m_coreBtn->move(center.x() - 24, center.y() - 24);
    
    qreal radius = 80.0 * m_expansion;
    qreal startAngle = 180.0;
    qreal endAngle = 0.0;
    qreal angleStep = (endAngle - startAngle) / (m_icons.size() - 1);
    
    for (int i = 0; i < m_icons.size(); ++i) {
        qreal angle = qDegreesToRadians(startAngle + i * angleStep);
        int x = center.x() + radius * qCos(angle) - 20;
        int y = center.y() - radius * qSin(angle) - 20; // Subtract for upward blossom
        m_icons[i]->move(x, y);
    }
}

void NavigationHub::onIconClicked() {
    auto* btn = qobject_cast<QPushButton*>(sender());
    if (btn) {
        AppContext context = static_cast<AppContext>(btn->property("context").toInt());
        emit contextRequested(context);
        toggleMenu(); // Auto-close
    }
}

void NavigationHub::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    
    if (m_expansion > 0.01) {
        // Draw the connecting "Neural Tendrils"
        QPoint center(width() / 2, height() - 30);
        p.setPen(QPen(QColor(255, 255, 255, 40 * m_expansion), 1, Qt::DashLine));
        
        for (auto* btn : m_icons) {
            if (btn->isVisible()) {
                p.drawLine(center, btn->geometry().center());
            }
        }
    }
}

void NavigationHub::resizeEvent(QResizeEvent*) {
    updateIconPositions();
}

} // namespace wizz::ui
