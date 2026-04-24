#include "SocialArcade.h"
#include <QScrollArea>
#include <QLabel>
#include <QVBoxLayout>

namespace wizz::ui::arcade {

using TE = ThemeEngine;

SocialArcade::SocialArcade(QWidget* parent)
    : QWidget(parent)
{
    setFixedHeight(110);
    
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(4);

    auto* header = new QLabel("SOCIAL ARCADE", this);
    header->setFont(TE::fontCaption());
    header->setStyleSheet(QString("color: %1; font-weight: 800; letter-spacing: 1px; padding-left: 4px;").arg(TE::onSurface3().name()));
    mainLayout->addWidget(header);

    // Use a scroll area so tiles can overflow horizontally if needed
    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setStyleSheet("background: transparent;");

    auto* container = new QWidget(scrollArea);
    container->setStyleSheet("background: transparent;");
    
    m_layout = new QHBoxLayout(container);
    m_layout->setContentsMargins(4, 0, 4, 0);
    m_layout->setSpacing(8);
    m_layout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    setupTiles();

    scrollArea->setWidget(container);
    mainLayout->addWidget(scrollArea);
}

void SocialArcade::setupTiles() {
    auto categories = getDefaultCategories();
    for (const auto& cat : categories) {
        auto* tile = new ArcadeTile(cat, this);
        connect(tile, &ArcadeTile::tileClicked, this, &SocialArcade::categorySelected);
        m_layout->addWidget(tile);
    }
}

} // namespace wizz::ui::arcade
