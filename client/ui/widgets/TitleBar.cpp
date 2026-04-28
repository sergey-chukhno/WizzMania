#include "TitleBar.h"
#include <QApplication>
#include <QScreen>

using TE = wizz::ui::ThemeEngine;

// ─── Construction ────────────────────────────────────────────────────────────

TitleBar::TitleBar(const QString& title, QWidget* parent)
    : QWidget(parent)
    , m_bgColor(TE::surfaceHigh())
{
    setFixedHeight(TE::titleBarHeight);
    setAttribute(Qt::WA_StyledBackground, false);
    buildLayout();
    setTitle(title);
}

// ─── Public API ──────────────────────────────────────────────────────────────

void TitleBar::setTitle(const QString& title) {
    if (m_titleLabel)
        m_titleLabel->setText(title);
}

void TitleBar::setRightAction(int index, const QIcon& icon,
                               const QString& tooltip,
                               std::function<void()> callback) {
    if (index < 0 || index > 2 || !m_rightBtns[index])
        return;

    QPushButton* btn = m_rightBtns[index];
    btn->setIcon(icon);
    btn->setIconSize(QSize(16, 16));
    btn->setToolTip(tooltip);
    btn->setVisible(true);
    connect(btn, &QPushButton::clicked, this, callback);
}

void TitleBar::setLeftWidget(QWidget* w) {
    if (m_leftSlot && m_leftSlot->layout()) {
        // Remove old widget
        QLayoutItem* item;
        while ((item = m_leftSlot->layout()->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }
    }
    if (w && m_leftSlot) {
        m_leftSlot->layout()->addWidget(w);
        m_leftSlot->setVisible(true);
    }
}

void TitleBar::setBackgroundColor(const QColor& color) {
    m_bgColor = color;
    update();
}

// ─── Layout ──────────────────────────────────────────────────────────────────

void TitleBar::buildLayout() {
    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ── Traffic-light area (left, fixed width) ──
    const int tlLeft  = TE::trafficLightLeft;
    const int tlSize  = TE::trafficLightSize;
    const int tlGap   = TE::trafficLightGap;
    const int tlAreaW = tlLeft + 3 * tlSize + 2 * tlGap + tlLeft;

    auto* tlArea = new QWidget(this);
    tlArea->setFixedWidth(tlAreaW);

    auto makeBtn = [&](QColor color, int x) -> QPushButton* {
        auto* btn = new QPushButton(tlArea);
        btn->setFixedSize(tlSize, tlSize);
        btn->move(x, (TE::titleBarHeight - tlSize) / 2);
        btn->setStyleSheet(QString(
            "QPushButton {"
            "  background-color: %1;"
            "  border-radius: %2px;"
            "  border: 0.5px solid rgba(0,0,0,20);"
            "}"
            "QPushButton:hover { background-color: %3; }"
            "QPushButton:pressed { background-color: %4; }"
        ).arg(color.name())
         .arg(tlSize / 2)
         .arg(color.darker(115).name())
         .arg(color.darker(130).name()));
        btn->setVisible(false); // hidden until hover
        btn->raise();
        return btn;
    };

    int x = tlLeft;
    m_btnClose    = makeBtn(TE::trafficRed(),    x); x += tlSize + tlGap;
    m_btnMinimize = makeBtn(TE::trafficYellow(), x); x += tlSize + tlGap;
    m_btnMaximize = makeBtn(TE::trafficGreen(),  x);

    // Connect to signals AND to exact macOS behavior
    connect(m_btnClose, &QPushButton::clicked, this, [this] {
        emit closeClicked();
        if (window()) window()->close(); // Standard behavior: close the window, let Qt handle app exit if last window.
    });
    connect(m_btnMinimize, &QPushButton::clicked, this, [this] {
        emit minimizeClicked();
        if (window()) window()->showMinimized();
    });
    connect(m_btnMaximize, &QPushButton::clicked, this, [this] {
        emit maximizeClicked();
        if (!window()) return;
        if (window()->isMaximized())
            window()->showNormal();
        else
            window()->showMaximized();
    });

    root->addWidget(tlArea);

    // ── Left slot (Logo) ──
    m_leftSlot = new QWidget(this);
    auto* leftLayout = new QHBoxLayout(m_leftSlot);
    leftLayout->setContentsMargins(15, 0, 0, 0);
    leftLayout->setSpacing(0);
    
    QLabel* logoLabel = new QLabel(this);
    logoLabel->setFixedSize(20, 20);
    logoLabel->setPixmap(QPixmap(":/assets/butterfly.png").scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    leftLayout->addWidget(logoLabel);
    
    m_leftSlot->setVisible(true);
    root->addWidget(m_leftSlot);

    // ── Center title (expands to fill remaining space) ──
    m_titleLabel = new QLabel(this);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setFont(TE::fontBody());
    m_titleLabel->setStyleSheet(
        QString("color: %1; background: transparent; font-weight: 600; font-size: 13px;")
        .arg(TE::onSurface().name())
    );
    root->addWidget(m_titleLabel, 1); // stretch = 1 → takes remaining space

    // ── Right action buttons ──
    m_rightArea = new QWidget(this);
    m_rightArea->setFixedWidth(tlAreaW); // symmetric with traffic-light area
    auto* rightLayout = new QHBoxLayout(m_rightArea);
    rightLayout->setContentsMargins(4, 0, 8, 0);
    rightLayout->setSpacing(2);
    rightLayout->addStretch();

    for (int i = 0; i < 3; ++i) {
        auto* btn = new QPushButton(m_rightArea);
        btn->setFixedSize(26, 26);
        btn->setVisible(false);
        btn->setStyleSheet(R"(
            QPushButton {
                background: transparent;
                border: none;
                border-radius: 13px;
            }
            QPushButton:hover {
                background: rgba(0,0,0,8);
            }
            QPushButton:pressed {
                background: rgba(0,0,0,15);
            }
        )");
        rightLayout->addWidget(btn);
        m_rightBtns[i] = btn;
    }

    root->addWidget(m_rightArea);
}

// ─── Painting ────────────────────────────────────────────────────────────────

void TitleBar::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Background fill
    p.fillRect(rect(), m_bgColor);

    // Bottom divider line
    p.setPen(QPen(TE::divider(), 1));
    p.drawLine(0, height() - 1, width(), height() - 1);
}

// ─── Drag to move ────────────────────────────────────────────────────────────

void TitleBar::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragStartPos = event->globalPosition().toPoint() - window()->frameGeometry().topLeft();
        event->accept();
    }
}

void TitleBar::mouseMoveEvent(QMouseEvent* event) {
    if (m_dragging && (event->buttons() & Qt::LeftButton) && window()) {
        window()->move(event->globalPosition().toPoint() - m_dragStartPos);
        event->accept();
    }
}

void TitleBar::mouseReleaseEvent(QMouseEvent* event) {
    m_dragging = false;
    event->accept();
}

// ─── Traffic-light hover reveal (authentic macOS) ────────────────────────────

void TitleBar::enterEvent(QEnterEvent*) {
    updateTrafficLightVisibility(true);
}

void TitleBar::leaveEvent(QEvent*) {
    updateTrafficLightVisibility(false);
}

void TitleBar::updateTrafficLightVisibility(bool visible) {
    if (m_trafficLightVisible == visible) return;
    m_trafficLightVisible = visible;
    if (m_btnClose)    m_btnClose->setVisible(visible);
    if (m_btnMinimize) m_btnMinimize->setVisible(visible);
    if (m_btnMaximize) m_btnMaximize->setVisible(visible);
}
