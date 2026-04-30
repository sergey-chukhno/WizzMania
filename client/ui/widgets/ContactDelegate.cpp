#include "ContactDelegate.h"
#include <QListWidget>

namespace wizz::ui {

using TE = ThemeEngine;

ContactDelegate::ContactDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
    // Global pulse animation for Online dots (scales 1.0 -> 1.3 -> 1.0)
    m_pulseAnim = new QVariantAnimation(this);
    m_pulseAnim->setStartValue(1.0);
    m_pulseAnim->setKeyValueAt(0.5, 1.3);
    m_pulseAnim->setEndValue(1.0);
    m_pulseAnim->setDuration(2000);
    m_pulseAnim->setLoopCount(-1); // Infinite loop
    connect(m_pulseAnim, &QVariantAnimation::valueChanged, this, &ContactDelegate::onPulseUpdated);
    m_pulseAnim->start();
}

ContactDelegate::~ContactDelegate() {
    // Animations are children, automatically deleted
}

QSize ContactDelegate::sizeHint(const QStyleOptionViewItem& /*option*/, const QModelIndex& /*index*/) const {
    return QSize(0, TE::contactRowHeight); // Fixed height row, width handled by ListView
}

void ContactDelegate::onPulseUpdated(const QVariant& value) {
    m_pulseScale = value.toReal();
    
    // Trigger a repaint of the parent view to show the pulse
    if (auto* view = qobject_cast<QAbstractItemView*>(parent())) {
        view->viewport()->update();
    }
}

// ─── Painting ────────────────────────────────────────────────────────────────

void ContactDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    // Extract data from index (we assume the ListWidget stores ContactInfo ptr or struct in UserRole)
    // Actually, Qt models store QVariant. We need to register ContactInfo as a metatype, 
    // or just pass a pointer. Let's assume UserRole contains a pointer or we fetch from MainWindow.
    // For safety, let's assume UserRole stores the username, and we query MainWindow (our parent).
    
    auto* parentWidget = qobject_cast<QWidget*>(parent());
    // Since we need ContactInfo and it's not a standard QVariant yet, we'll assume 
    // we get a pointer via Qt::UserRole + 1. We'll update MainWindow to set this.
    QVariant dataVar = index.data(Qt::UserRole + 1);
    if (!dataVar.isValid() || !dataVar.canConvert<void*>()) {
        painter->restore();
        return;
    }
    
    const ContactInfo* info = static_cast<const ContactInfo*>(dataVar.value<void*>());
    if (!info) {
        painter->restore();
        return;
    }

    bool isHovered = option.state & QStyle::State_MouseOver;
    bool isSelected = option.state & QStyle::State_Selected;

    // 1. Background (Glassmorphic)
    if (isSelected) {
        QPainterPath path;
        path.addRoundedRect(option.rect.adjusted(4, 2, -4, -2), TE::rMd, TE::rMd);
        QColor selColor = TE::accent();
        selColor.setAlpha(60); // 25% opacity glass accent
        painter->fillPath(path, selColor);
        painter->setPen(QPen(TE::accent(), 1));
        painter->drawPath(path);
    } else {
        paintBackground(painter, option, index, isHovered);
    }

    // 2. Avatar
    paintAvatar(painter, option.rect, *info);

    // 3. Status Dot
    paintStatusDot(painter, option.rect, info->status);

    // 4. Text (Name + Subtitle)
    paintText(painter, option.rect, *info);

    // 5. Unread Badge
    if (info->unreadCount > 0) {
        paintBadge(painter, option.rect, info->unreadCount);
    }

    painter->restore();
}

void ContactDelegate::paintBackground(QPainter* p, const QStyleOptionViewItem& opt, const QModelIndex& idx, bool isHovered) const {
    int row = idx.row();
    qreal progress = m_hoverProgress.value(row, 0.0);

    // If suddenly hovered but no animation, jump to 1.0 (failsafe)
    if (isHovered && !m_animations.contains(row)) progress = 1.0;
    if (!isHovered && !m_animations.contains(row)) progress = 0.0;

    if (progress > 0.0) {
        QColor hoverColor = TE::surfaceHigh();
        hoverColor.setAlphaF(hoverColor.alphaF() * progress);
        
        QPainterPath path;
        path.addRoundedRect(opt.rect.adjusted(4, 2, -4, -2), TE::rMd, TE::rMd);
        p->fillPath(path, hoverColor);
    }
}

void ContactDelegate::paintAvatar(QPainter* p, const QRect& rect, const ContactInfo& info) const {
    const int size = TE::contactAvatarSize;
    const int margin = (TE::contactRowHeight - size) / 2;
    QRect avatarRect(rect.left() + TE::sp3, rect.top() + margin, size, size);

    QPainterPath path;
    path.addEllipse(avatarRect);
    p->setClipPath(path);

    if (!info.avatar.isNull()) {
        p->drawPixmap(avatarRect, info.avatar.scaled(size, size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    } else {
        // Fallback: colored circle with initials
        QColor bgColor = TE::accent(); // Or generate based on name hash
        p->fillRect(avatarRect, bgColor);
        
        p->setPen(Qt::white);
        p->setFont(TE::fontTitle());
        QString initial = info.username.isEmpty() ? "?" : info.username.left(1).toUpper();
        p->drawText(avatarRect, Qt::AlignCenter, initial);
    }
    p->setClipping(false);
}

void ContactDelegate::paintStatusDot(QPainter* p, const QRect& rect, ::UserStatus status) const {
    const int size = TE::contactAvatarSize;
    const int margin = (TE::contactRowHeight - size) / 2;
    const int dotSize = 12;
    
    // Position at bottom-right of avatar
    int cx = rect.left() + TE::sp3 + size - 4;
    int cy = rect.top() + margin + size - 4;

    QColor color;
    switch (status) {
        case ::UserStatus::Online: color = TE::success(); break;
        case ::UserStatus::Away:   color = TE::warning(); break;
        case ::UserStatus::Busy:   color = TE::danger(); break;
        case ::UserStatus::Offline: color = TE::onSurface3(); break;
    }

    qreal scale = (status == ::UserStatus::Online) ? m_pulseScale : 1.0;
    int scaledSize = static_cast<int>(dotSize * scale);
    int offset = (scaledSize - dotSize) / 2;

    QRectF dotRect(cx - offset - dotSize/2, cy - offset - dotSize/2, scaledSize, scaledSize);

    // Draw outer stroke (to separate from avatar)
    p->setPen(QPen(TE::surface(), 2));
    p->setBrush(color);
    p->drawEllipse(dotRect);
}

void ContactDelegate::paintText(QPainter* p, const QRect& rect, const ContactInfo& info) const {
    int textX = rect.left() + TE::sp3 + TE::contactAvatarSize + TE::sp3;
    int textWidth = rect.width() - textX - TE::sp4; // Leave room for badge

    // Name
    p->setFont(TE::fontTitle());
    p->setPen(TE::onSurface());
    QRect nameRect(textX, rect.top() + 14, textWidth, 20);
    p->drawText(nameRect, Qt::AlignLeft | Qt::AlignVCenter, info.username);

    // Subtitle
    QString subText = info.statusMessage;
    if (info.isPlayingGame) {
        subText = QString("🎮 Playing %1").arg(info.currentGameName);
        p->setPen(TE::accent()); // Highlight game activity
    } else {
        p->setPen(TE::onSurface2());
    }

    if (subText.isEmpty()) {
    ::UserStatus status = info.status;
    switch(status) {
        case ::UserStatus::Online: subText = "Online"; break;
        case ::UserStatus::Away: subText = "Away"; break;
        case ::UserStatus::Busy: subText = "Busy"; break;
        case ::UserStatus::Offline: subText = "Offline"; break;
    }
    }

    p->setFont(TE::fontCaption());
    QRect subRect(textX, rect.top() + 34, textWidth, 16);
    // Elide text if too long
    QString elided = p->fontMetrics().elidedText(subText, Qt::ElideRight, textWidth);
    p->drawText(subRect, Qt::AlignLeft | Qt::AlignVCenter, elided);
}

void ContactDelegate::paintBadge(QPainter* p, const QRect& rect, int count) const {
    if (count <= 0) return;

    QString txt = QString::number(count);
    if (count > 99) txt = "99+";

    p->setFont(TE::fontCaption());
    QFontMetrics fm(p->font());
    int txtW = fm.horizontalAdvance(txt);
    int padding = 6;
    int badgeW = qMax(18, txtW + padding * 2);
    int badgeH = 18;

    int bx = rect.right() - TE::sp3 - badgeW;
    int by = rect.top() + (TE::contactRowHeight - badgeH) / 2;

    QRect badgeRect(bx, by, badgeW, badgeH);

    QPainterPath path;
    path.addRoundedRect(badgeRect, badgeH/2, badgeH/2);

    p->fillPath(path, TE::danger());
    p->setPen(Qt::white);
    p->drawText(badgeRect, Qt::AlignCenter, txt);
}

// ─── Hover Animations ────────────────────────────────────────────────────────

bool ContactDelegate::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) {
    if (event->type() == QEvent::HoverEnter || event->type() == QEvent::MouseMove) {
        startHoverAnimation(index);
    } else if (event->type() == QEvent::HoverLeave) {
        stopHoverAnimation(index);
    }
    return QStyledItemDelegate::editorEvent(event, model, option, index);
}

void ContactDelegate::startHoverAnimation(const QModelIndex& index) {
    int row = index.row();
    if (m_hoverProgress.value(row, 0.0) >= 1.0) return;

    if (m_animations.contains(row)) {
        m_animations[row]->stop();
        m_animations[row]->deleteLater();
    }

    auto* anim = new QVariantAnimation(this);
    anim->setStartValue(m_hoverProgress.value(row, 0.0));
    anim->setEndValue(1.0);
    anim->setDuration(120);
    anim->setEasingCurve(QEasingCurve::OutCubic);

    connect(anim, &QVariantAnimation::valueChanged, this, [this, row](const QVariant& val) {
        m_hoverProgress[row] = val.toReal();
        if (auto* view = qobject_cast<QAbstractItemView*>(parent())) {
            // Repaint just this row
            if (view->model()) {
                QModelIndex idx = view->model()->index(row, 0);
                view->update(idx);
            }
        }
    });
    
    connect(anim, &QVariantAnimation::finished, this, [this, row]() {
        m_animations.remove(row);
    });

    m_animations[row] = anim;
    anim->start();
}

void ContactDelegate::stopHoverAnimation(const QModelIndex& index) {
    int row = index.row();
    if (m_hoverProgress.value(row, 0.0) <= 0.0) return;

    if (m_animations.contains(row)) {
        m_animations[row]->stop();
        m_animations[row]->deleteLater();
    }

    auto* anim = new QVariantAnimation(this);
    anim->setStartValue(m_hoverProgress.value(row, 0.0));
    anim->setEndValue(0.0);
    anim->setDuration(200);
    anim->setEasingCurve(QEasingCurve::InCubic);

    connect(anim, &QVariantAnimation::valueChanged, this, [this, row](const QVariant& val) {
        m_hoverProgress[row] = val.toReal();
        if (auto* view = qobject_cast<QAbstractItemView*>(parent())) {
            if (view->model()) {
                QModelIndex idx = view->model()->index(row, 0);
                view->update(idx);
            }
        }
    });

    connect(anim, &QVariantAnimation::finished, this, [this, row]() {
        m_animations.remove(row);
    });

    m_animations[row] = anim;
    anim->start();
}

} // namespace wizz::ui
