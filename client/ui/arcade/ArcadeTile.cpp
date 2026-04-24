#include "ArcadeTile.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QEvent>

namespace wizz::ui::arcade {

using TE = ThemeEngine;

ArcadeTile::ArcadeTile(const ArcadeCategory& category, QWidget* parent)
    : QPushButton(parent)
    , m_category(category)
{
    setFixedSize(72, 80);
    setCursor(Qt::PointingHandCursor);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 12, 4, 8);
    layout->setSpacing(6);

    auto* iconLabel = new QLabel(category.icon, this);
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setStyleSheet("font-size: 28px; background: transparent;");
    
    auto* nameLabel = new QLabel(category.name, this);
    nameLabel->setAlignment(Qt::AlignCenter);
    nameLabel->setFont(TE::fontCaption());
    nameLabel->setStyleSheet(QString("color: %1; background: transparent; font-weight: 600;").arg(TE::onSurface().name()));

    layout->addWidget(iconLabel);
    layout->addWidget(nameLabel);
    layout->addStretch();

    // Default style (Subtle Glass)
    setStyleSheet(QString(R"(
        QPushButton {
            background-color: rgba(255, 255, 255, 10);
            border: 1px solid rgba(255, 255, 255, 20);
            border-radius: %1px;
        }
    )").arg(TE::rMd));

    m_hoverAnim = new QPropertyAnimation(this, "geometry", this);
    m_hoverAnim->setDuration(150);

    connect(this, &QPushButton::clicked, this, [this]() {
        emit tileClicked(m_category);
    });
}

bool ArcadeTile::event(QEvent* e) {
    if (e->type() == QEvent::HoverEnter) {
        animateHover(true);
    } else if (e->type() == QEvent::HoverLeave) {
        animateHover(false);
    }
    return QPushButton::event(e);
}

void ArcadeTile::animateHover(bool hover) {
    if (hover) {
        setStyleSheet(QString(R"(
            QPushButton {
                background-color: %1;
                border: 1px solid %2;
                border-radius: %3px;
            }
        )").arg(TE::surface().name()) // More solid on hover for readability
           .arg(m_category.colorHex)
           .arg(TE::rMd));
    } else {
        setStyleSheet(QString(R"(
            QPushButton {
                background-color: rgba(255, 255, 255, 10);
                border: 1px solid rgba(255, 255, 255, 20);
                border-radius: %1px;
            }
        )").arg(TE::rMd));
    }
}

} // namespace wizz::ui::arcade
