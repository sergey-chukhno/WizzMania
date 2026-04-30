#pragma once

#include "../theme/ThemeEngine.h"
#include <QStyledItemDelegate>
#include <QPainter>
#include <QPainterPath>
#include <QVariantAnimation>
#include <QEvent>
#include <QMouseEvent>

// We need ContactInfo from MainWindow.h, but to avoid circular dependencies,
// we define a minimal struct here for the delegate, or we can just #include MainWindow.h
// Since MainWindow.h defines ContactInfo, we'll include it.
#include "../MainWindow.h"

namespace wizz::ui {

/**
 * @brief ContactDelegate — High-performance 60fps rendering for contact rows.
 *
 * Paints:
 * - Circular avatar (with fallback colored initials)
 * - Name (bold)
 * - Sub-status (Rich Presence or Last Message)
 * - Online pulse dot (animated)
 * - Unread badge pill (red, right-aligned)
 * - Hover background tint (animated ease-in)
 */
class ContactDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    explicit ContactDelegate(QObject* parent = nullptr);
    ~ContactDelegate() override;

    // Standard Qt override
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    // Animation triggers
    void startHoverAnimation(const QModelIndex& index);
    void stopHoverAnimation(const QModelIndex& index);

protected:
    bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) override;

private:
    // Helper painting methods
    void paintBackground(QPainter* p, const QStyleOptionViewItem& opt, const QModelIndex& idx, bool isHovered) const;
    void paintAvatar(QPainter* p, const QRect& rect, const ContactInfo& info) const;
    void paintStatusDot(QPainter* p, const QRect& avatarRect, ::UserStatus status) const;
    void paintText(QPainter* p, const QRect& rect, const ContactInfo& info) const;
    void paintBadge(QPainter* p, const QRect& rect, int count) const;

    // Animation state
    // Maps row index to current hover progress (0.0 to 1.0)
    mutable QMap<int, qreal> m_hoverProgress;
    QMap<int, QVariantAnimation*> m_animations;

    // Global pulse animation for Online status dots
    QVariantAnimation* m_pulseAnim;
    qreal m_pulseScale = 1.0;

private slots:
    void onPulseUpdated(const QVariant& value);
};

} // namespace wizz::ui
