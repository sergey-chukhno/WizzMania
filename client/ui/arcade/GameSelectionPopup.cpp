#include "GameSelectionPopup.h"
#include "../theme/ThemeEngine.h"
#include "../widgets/TitleBar.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>

namespace wizz::ui::arcade {

using TE = ThemeEngine;

GameSelectionPopup::GameSelectionPopup(const ArcadeCategory& category, QWidget* parent)
    : QDialog(parent)
    , m_category(category)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(400, 300);

    setupUI();
}

void GameSelectionPopup::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Sleek minimalist header instead of full title bar
    auto* headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(24, 16, 24, 0);
    
    auto* headerTitle = new QLabel("SELECT ARCADE GAME");
    headerTitle->setStyleSheet(QString("color: rgba(255, 255, 255, 180); font-weight: 900; letter-spacing: 3px; font-size: 11px;"));
    
    auto* closeBtn = new QPushButton("✕");
    closeBtn->setFixedSize(24, 24);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setStyleSheet(R"(
        QPushButton {
            background: rgba(255, 255, 255, 15);
            color: white;
            border-radius: 12px;
            font-weight: bold;
        }
        QPushButton:hover { background: rgba(255, 255, 255, 30); }
    )");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::reject);

    headerLayout->addWidget(headerTitle);
    headerLayout->addStretch();
    headerLayout->addWidget(closeBtn);
    
    mainLayout->addLayout(headerLayout);

    auto* contentLayout = new QVBoxLayout();
    contentLayout->setContentsMargins(30, 20, 30, 30);
    contentLayout->setSpacing(24);

    auto* titleLbl = new QLabel("CHOOSE YOUR CHALLENGE");
    titleLbl->setAlignment(Qt::AlignCenter);
    titleLbl->setStyleSheet("color: #FFFFFF; font-weight: 800; letter-spacing: 2px; font-size: 15px;");
    contentLayout->addWidget(titleLbl);

    auto* gamesLayout = new QHBoxLayout();
    gamesLayout->setAlignment(Qt::AlignCenter);
    gamesLayout->setSpacing(40);

    // Helper to create game buttons
    auto createGameBtn = [&](const QString& name, const QString& iconPath, const QString& gameId) {
        auto* container = new QWidget();
        auto* vlay = new QVBoxLayout(container);
        vlay->setContentsMargins(0, 0, 0, 0);
        vlay->setSpacing(12);
        vlay->setAlignment(Qt::AlignCenter);

        auto* btn = new QPushButton();
        btn->setFixedSize(120, 120);
        btn->setCursor(Qt::PointingHandCursor);
        
        QPixmap pix(iconPath);
        if (!pix.isNull()) {
            btn->setIcon(QIcon(pix));
            btn->setIconSize(QSize(90, 90));
        }

        btn->setStyleSheet(QString(R"(
            QPushButton {
                background-color: rgba(0, 0, 0, 40);
                border: 2px solid rgba(255, 255, 255, 20);
                border-radius: 20px;
            }
            QPushButton:hover {
                background-color: rgba(255, 255, 255, 10);
                border: 2px solid %1;
            }
        )").arg(m_category.colorHex));

        auto* label = new QLabel(name);
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet("color: rgba(255, 255, 255, 220); font-weight: 700; font-size: 14px;");

        vlay->addWidget(btn);
        vlay->addWidget(label);
        
        connect(btn, &QPushButton::clicked, this, [this, gameId]() {
            emit gameSelected(gameId);
            accept();
        });

        return container;
    };

    gamesLayout->addWidget(createGameBtn("Tile Twister", ":/assets/tiletwister_logo.png", "TileTwister"));
    gamesLayout->addWidget(createGameBtn("Brick Breaker", ":/assets/brickbreaker_logo.png", "BrickBreaker"));

    contentLayout->addLayout(gamesLayout);
    contentLayout->addStretch();

    mainLayout->addLayout(contentLayout);
}

void GameSelectionPopup::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QPainterPath path;
    path.addRoundedRect(rect().adjusted(1, 1, -1, -1), 24, 24);

    // Premium gradient background
    QLinearGradient grad(0, 0, 0, height());
    grad.setColorAt(0, QColor(40, 45, 60, 245));
    grad.setColorAt(1, QColor(20, 25, 35, 245));
    p.fillPath(path, grad);
    
    p.setPen(QPen(QColor(255, 255, 255, 40), 1));
    p.drawPath(path);
}

} // namespace wizz::ui::arcade
