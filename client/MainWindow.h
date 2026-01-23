#pragma once

#include "../common/Packet.h"
#include <QComboBox>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPainter>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>

// User status enum
enum class UserStatus { Online = 0, Away, Busy, Offline };

// Contact info structure
struct ContactInfo {
  QString username;
  UserStatus status;
  QString statusMessage;
  QPixmap avatar; // Empty = use initials
};

class AddFriendDialog; // Forward declaration
class ChatWindow;      // Forward declaration

/**
 * @brief MainWindow - Contact list window (MSN-style buddy list)
 * Displays user's contacts with online/offline status
 */
class MainWindow : public QWidget {
  Q_OBJECT

public:
  explicit MainWindow(const QString &username, QWidget *parent = nullptr);

  void setContacts(const QList<ContactInfo> &contacts);
  void updateContactStatus(const QString &username, UserStatus status,
                           const QString &statusMessage = "");

signals:
  void contactDoubleClicked(const QString &username);
  void logoutRequested();
  void statusChanged(UserStatus status, const QString &statusMessage);

protected:
  void paintEvent(QPaintEvent *event) override;

private slots:
  void onStatusChanged(int index);
  void onSendMessage();
  void onAddFriendClicked();
  void onRemoveFriendClicked();
  void onChatWindowClosed(const QString &partnerName);
  void onContactDoubleClicked(const QString &username);

private:
  void setupUI();
  void populateContactList();
  QPixmap createAvatarWithInitials(const QString &name, int size);
  QColor getStatusColor(UserStatus status);
  QString getStatusText(UserStatus status);

  QString m_username;
  UserStatus m_currentStatus = UserStatus::Online;
  QList<ContactInfo> m_contacts;

  QPixmap m_backgroundPixmap;

  // UI Elements
  QLabel *m_avatarLabel;
  QLabel *m_usernameLabel;
  QComboBox *m_statusCombo;
  QLineEdit *m_statusMessageInput;
  QListWidget *m_contactList;

  // Chat preview
  QFrame *m_chatPreviewFrame;
  QLabel *m_chatPreviewLabel;
  QLineEdit *m_messageInput;
  QPushButton *m_sendButton;

  // Dialogs
  AddFriendDialog *m_addFriendDialog = nullptr;

  // Active Chats
  QMap<QString, ChatWindow *> m_openChats;
};
