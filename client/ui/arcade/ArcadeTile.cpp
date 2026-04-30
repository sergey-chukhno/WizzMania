#include "ArcadeTile.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QEvent>
#include <QEnterEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QGraphicsDropShadowEffect>

namespace wizz::ui::arcade {

using TE = ThemeEngine;

ArcadeTile::ArcadeTile(const ArcadeCategory& category, QWidget* parent)
    : QPushButton(parent)
    , m_category(category)
{
    setFixedSize(84, 106);
    setCursor(Qt::PointingHandCursor);
    
    // Transparent button background to avoid clash with custom painting
    setAttribute(Qt::WA_StyledBackground, false);
    setStyleSheet("QPushButton { background: transparent; border: none; }");

    m_liftAnim = new QPropertyAnimation(this, "lift", this);
    m_liftAnim->setDuration(250);
    m_liftAnim->setEasingCurve(QEasingCurve::OutBack); // Sophisticated bounce

    connect(this, &QPushButton::clicked, this, [this]() {
        emit tileClicked(m_category);
    });
}

void ArcadeTile::setLift(int lift) {
    m_liftValue = lift;
    update();
}

void ArcadeTile::enterEvent(QEnterEvent* event) {
    animateHover(true);
    QPushButton::enterEvent(event);
}

void ArcadeTile::leaveEvent(QEvent* event) {
    animateHover(false);
    QPushButton::leaveEvent(event);
}

void ArcadeTile::mousePressEvent(QMouseEvent* event) {
    m_isPressed = true;
    update();
    QPushButton::mousePressEvent(event);
}

void ArcadeTile::mouseReleaseEvent(QMouseEvent* event) {
    m_isPressed = false;
    update();
    QPushButton::mouseReleaseEvent(event);
}

void ArcadeTile::animateHover(bool hover) {
    m_liftAnim->stop();
    m_liftAnim->setStartValue(m_liftValue);
    m_liftAnim->setEndValue(hover ? 100 : 0); // 0 to 100 percent
    m_liftAnim->start();
}

void ArcadeTile::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    float progress = m_liftValue / 100.0f;
    float scale = 1.0f + (progress * 0.08f);
    if (m_isPressed) scale *= 0.94f; // Subtle premium "click" compression
    
    float liftY = progress * 6.0f;

    QRectF baseRect = rect().adjusted(6, 6, -6, -6);
    
    p.save();
    p.translate(width() / 2, height() / 2);
    p.scale(scale, scale);
    p.translate(-width() / 2, -height() / 2 - liftY);

    // 1. Draw Glow
    if (progress > 0.01f) {
        QRadialGradient glow(width() / 2, height() / 2, width() / 2);
        QColor glowColor = QColor(m_category.colorHex);
        glowColor.setAlpha(static_cast<int>(60 * progress));
        glow.setColorAt(0, glowColor);
        glow.setColorAt(1, Qt::transparent);
        p.fillRect(rect(), glow);
    }

    // 2. Draw Glass Card
    QPainterPath cardPath;
    cardPath.addRoundedRect(baseRect, 16, 16);
    
    QLinearGradient cardGrad(0, 0, 0, height());
    cardGrad.setColorAt(0, QColor(255, 255, 255, static_cast<int>(20 + (progress * 15))));
    cardGrad.setColorAt(1, QColor(255, 255, 255, static_cast<int>(10 + (progress * 5))));
    p.fillPath(cardPath, cardGrad);

    QPen borderPen(QColor(255, 255, 255, static_cast<int>(30 + (progress * 50))), 1.0 + (progress * 0.5));
    p.setPen(borderPen);
    p.drawPath(cardPath);

    // 3. Draw Icon
    QPixmap pix(m_category.icon);
    if (!pix.isNull()) {
        // Color-key transparency for black backgrounds
        QImage img = pix.toImage().convertToFormat(QImage::Format_ARGB32);
        for (int y = 0; y < img.height(); ++y) {
            for (int x = 0; x < img.width(); ++x) {
                QRgb pixel = img.pixel(x, y);
                if (qRed(pixel) < 30 && qGreen(pixel) < 30 && qBlue(pixel) < 30) {
                    img.setPixel(x, y, qRgba(0, 0, 0, 0));
                }
            }
        }
        pix = QPixmap::fromImage(img);

        float iconSize = 44.0f;
        QRectF iconRect(width() / 2 - iconSize / 2, 16, iconSize, iconSize);
        p.drawPixmap(iconRect.toRect(), pix);
    }

    // 4. Draw Label
    p.setPen(Qt::white);
    QFont font = TE::fontCaption();
    font.setBold(true);
    font.setPointSizeF(font.pointSizeF() + 0.5);
    p.setFont(font);

    // Text Shadow
    p.save();
    p.setPen(QColor(0, 0, 0, 180));
    p.drawText(rect().adjusted(0, 68, 0, 0), Qt::AlignCenter, m_category.name);
    p.restore();

    p.drawText(rect().adjusted(0, 67, 0, -1), Qt::AlignCenter, m_category.name);

    p.restore();
}

} // namespace wizz::ui::arcade
