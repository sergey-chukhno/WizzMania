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
    setMinimumHeight(340);
    setMaximumHeight(340);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    auto* headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(12, 6, 12, 4);

    auto* header = new QLabel("🕹️ SOCIAL ARCADE", this);
    header->setFont(TE::fontCaption());
    header->setStyleSheet("color: #FFFFFF; font-weight: 900; letter-spacing: 2px;");
    
    m_toggleBtn = new QPushButton("▼", this);
    m_toggleBtn->setFixedSize(28, 28);
    m_toggleBtn->setCursor(Qt::PointingHandCursor);
    m_toggleBtn->setStyleSheet(QString(R"(
        QPushButton {
            background: rgba(255, 255, 255, 25);
            color: white;
            border: 1px solid rgba(255, 255, 255, 40);
            border-radius: 14px;
            font-weight: bold;
            font-size: 11px;
        }
        QPushButton:hover {
            background: rgba(255, 255, 255, 45);
            border: 1px solid %1;
        }
    )").arg(TE::accent().name()));
    connect(m_toggleBtn, &QPushButton::clicked, this, &SocialArcade::toggleCollapse);

    headerLayout->addWidget(header);
    headerLayout->addStretch();
    headerLayout->addWidget(m_toggleBtn);

    mainLayout->addLayout(headerLayout);

    m_contentContainer = new QWidget(this);
    m_contentContainer->setStyleSheet("background: transparent;");
    setupTiles(m_contentContainer);
    
    mainLayout->addWidget(m_contentContainer, 1);
}

void SocialArcade::toggleCollapse() {
    m_isCollapsed = !m_isCollapsed;
    m_toggleBtn->setText(m_isCollapsed ? "▲" : "▼");
    
    int targetHeight = m_isCollapsed ? 40 : 340;
    
    if (!m_isCollapsed) {
        m_contentContainer->show();
    }
    
    auto* anim = new QPropertyAnimation(this, "maximumHeight");
    anim->setDuration(300);
    anim->setStartValue(height());
    anim->setEndValue(targetHeight);
    anim->setEasingCurve(QEasingCurve::InOutQuart);
    
    connect(anim, &QPropertyAnimation::finished, this, [this, targetHeight]() {
        setMinimumHeight(targetHeight);
        setMaximumHeight(targetHeight);
        if (m_isCollapsed) {
            m_contentContainer->hide();
        }
    });
    
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

    QRectF drawingRect = rect();
    drawingRect.adjust(0.5, 0.5, -0.5, -0.5); // Offset for crisp 1px border
    qreal radius = 20.0;

    // Subtle dark scrim with rounded corners
    QLinearGradient scrim(0, 0, 0, height());
    scrim.setColorAt(0, QColor(255, 255, 255, 10)); // Very faint top glass
    scrim.setColorAt(1, QColor(0, 0, 0, 120));      // Darker bottom for legibility
    
    p.setBrush(scrim);
    p.setPen(QPen(QColor(255, 255, 255, 40), 1));   // Subtle glass border
    p.drawRoundedRect(drawingRect, radius, radius);
}

} // namespace wizz::ui::arcade
