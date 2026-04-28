#include "SocialArcade.h"
#include <QScrollArea>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QPainter>

namespace wizz::ui::arcade {

using TE = ThemeEngine;

SocialArcade::SocialArcade(QWidget* parent)
    : QWidget(parent), m_isCollapsed(false)
{
    setFixedHeight(260);
    
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(4);

    auto* headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(4, 0, 4, 0);

    auto* header = new QLabel("🕹️ SOCIAL ARCADE", this);
    header->setFont(TE::fontCaption());
    header->setStyleSheet("color: #FFFFFF; font-weight: 900; letter-spacing: 2px;");
    
    m_toggleBtn = new QPushButton("▼", this);
    m_toggleBtn->setFixedSize(24, 24);
    m_toggleBtn->setCursor(Qt::PointingHandCursor);
    m_toggleBtn->setStyleSheet(QString(R"(
        QPushButton {
            background: rgba(255, 255, 255, 15);
            color: %1;
            border: 1px solid rgba(255, 255, 255, 20);
            border-radius: 12px;
            font-weight: bold;
            font-size: 10px;
        }
        QPushButton:hover {
            background: rgba(255, 255, 255, 35);
            border: 1px solid %2;
        }
    )").arg(TE::onSurface().name()).arg(TE::accent().name()));
    connect(m_toggleBtn, &QPushButton::clicked, this, &SocialArcade::toggleCollapse);

    headerLayout->addWidget(header);
    headerLayout->addStretch();
    headerLayout->addWidget(m_toggleBtn);

    mainLayout->addLayout(headerLayout);

    // Use a scroll area so tiles can overflow horizontally if needed
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setStyleSheet("background: transparent;");

    auto* container = new QWidget(m_scrollArea);
    container->setStyleSheet("background: transparent;");
    
    setupTiles(container);

    m_scrollArea->setWidget(container);
    mainLayout->addWidget(m_scrollArea);
}

void SocialArcade::toggleCollapse() {
    m_isCollapsed = !m_isCollapsed;
    m_toggleBtn->setText(m_isCollapsed ? "▶" : "▼");
    
    auto* anim = new QPropertyAnimation(this, "maximumHeight");
    anim->setDuration(250);
    anim->setStartValue(height());
    anim->setEndValue(m_isCollapsed ? 30 : 260);
    anim->setEasingCurve(QEasingCurve::InOutQuad);
    
    if (!m_isCollapsed) {
        m_scrollArea->show();
    } else {
        connect(anim, &QPropertyAnimation::finished, m_scrollArea, &QScrollArea::hide);
    }
    
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void SocialArcade::setupTiles(QWidget* container) {
    auto categories = getDefaultCategories();
    
    // Use a vertical layout to stack two rows
    auto* contentStack = new QVBoxLayout();
    contentStack->setContentsMargins(0, 8, 0, 8);
    contentStack->setSpacing(12);

    auto* row1 = new QHBoxLayout();
    row1->setAlignment(Qt::AlignCenter);
    row1->setSpacing(16);

    auto* row2 = new QHBoxLayout();
    row2->setAlignment(Qt::AlignCenter);
    row2->setSpacing(16);

    for (int i = 0; i < categories.size(); ++i) {
        auto* tile = new ArcadeTile(categories[i], this);
        connect(tile, &ArcadeTile::tileClicked, this, &SocialArcade::categorySelected);
        
        if (i < 3) {
            row1->addWidget(tile);
        } else {
            row2->addWidget(tile);
        }
    }

    contentStack->addLayout(row1);
    contentStack->addLayout(row2);
    
    container->setLayout(contentStack);
}

void SocialArcade::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Subtle dark scrim at the bottom to ensure tile legibility
    QLinearGradient scrim(0, 0, 0, height());
    scrim.setColorAt(0, Qt::transparent);
    scrim.setColorAt(1, QColor(0, 0, 0, 100));
    p.fillRect(rect(), scrim);
}

} // namespace wizz::ui::arcade
