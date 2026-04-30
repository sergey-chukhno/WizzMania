#include "TypingBubble.h"
#include "../theme/ThemeEngine.h"
#include <QPainter>
#include <QPainterPath>

namespace wizz::ui {

TypingBubble::TypingBubble(QWidget* parent) : QWidget(parent) {
    setFixedSize(60, 36);
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, [this]() {
        m_step = (m_step + 1) % 30; // 30 steps per cycle
        update();
    });
}

TypingBubble::~TypingBubble() {}

void TypingBubble::startAnimation() {
    m_step = 0;
    m_timer->start(33); // ~30 FPS
    show();
}

void TypingBubble::stopAnimation() {
    m_timer->stop();
    hide();
}

void TypingBubble::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Background bubble
    QPainterPath path;
    path.addRoundedRect(rect(), 18, 18);
    p.fillPath(path, ThemeEngine::surfaceHigh());
    p.setPen(QPen(QColor(255,255,255,20), 1));
    p.drawPath(path);

    // 3 Bouncing Dots
    p.setPen(Qt::NoPen);
    p.setBrush(ThemeEngine::onSurface2());

    int dotRadius = 3;
    int spacing = 8;
    int startX = (width() - (3 * dotRadius * 2 + 2 * spacing)) / 2;
    int baseY = height() / 2;

    for (int i = 0; i < 3; ++i) {
        // Calculate bounce offset using sine wave
        // Offset phases: dot 0: 0, dot 1: 10, dot 2: 20
        int phaseOffset = i * 10;
        int localStep = (m_step + 30 - phaseOffset) % 30;
        
        float offsetY = 0;
        if (localStep < 15) { // First half of cycle = bounce up and down
            // 0 to PI
            offsetY = -6.0f * qSin((localStep / 15.0f) * M_PI);
        }

        p.drawEllipse(QPointF(startX + dotRadius + i * (dotRadius * 2 + spacing), baseY + offsetY), dotRadius, dotRadius);
    }
}

} // namespace wizz::ui
