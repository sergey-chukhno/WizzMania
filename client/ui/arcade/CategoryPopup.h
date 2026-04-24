#pragma once

#include "ArcadeCategory.h"
#include <QDialog>
#include <QLineEdit>

namespace wizz::ui::arcade {

/**
 * @brief CategoryPopup — Modal popup when a category is clicked.
 * Allows the user to input a status string and either share it to a specific chat
 * or broadcast it as Rich Presence.
 */
class CategoryPopup : public QDialog {
    Q_OBJECT

public:
    explicit CategoryPopup(const ArcadeCategory& category, const QList<QString>& onlineContacts, QWidget* parent = nullptr);

signals:
    void broadcastRichPresenceRequested(const ArcadeCategory& category, const QString& text);
    void shareToChatRequested(const ArcadeCategory& category, const QString& text, const QString& targetUser);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    ArcadeCategory m_category;
    QList<QString> m_onlineContacts;
    QLineEdit* m_inputField;
    
    void setupUI();
};

} // namespace wizz::ui::arcade
