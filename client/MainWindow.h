#pragma once

#include "../common/NativeSharedMemory.h"
#include "../common/Packet.h"
#include "../common/TicTacToeIPC.h"
#include <QComboBox>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPainter>
#include <QPointer>
#include <QProcess>
#include <QPushButton>
#include <QScrollArea>
#include <QTimer>
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

  // Game state
  bool isPlayingGame = false;
  QString currentGameName;
  uint32_t currentGameScore = 0;
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
  explicit MainWindow(const QString &username,
                      const QPoint &initialPos = QPoint(),
                      QWidget *parent = nullptr);
  virtual ~MainWindow();

  void setContacts(const QList<ContactInfo> &contacts);
  void updateContactStatus(const QString &username, UserStatus status,
                           const QString &statusMessage = "");
  void updateContactAvatar(const QString &username, const QPixmap &avatar);
  void updateContactGameStatus(const QString &username, const QString &gameName,
                               uint32_t score);

signals:
  void contactDoubleClicked(const QString &username);
  void logoutRequested();
  void statusChanged(UserStatus status, const QString &statusMessage);

protected:
  // Paint Event for background
  void paintEvent(QPaintEvent *event) override;

  // Event Filter for hover effects
  bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
  void onStatusChanged(int index);
  void onStatusMessageSubmitted();
  void onSendMessage();
  void onAddFriendClicked();
  void onRemoveFriendClicked();
  void onChatWindowClosed(const QString &partnerName);
  void onContactDoubleClicked(const QString &username);

  // Avatar Slots
  void onAvatarClicked();

  // GameBridge Slots
  void onLocalGameStatusChanged(bool isPlaying, const QString& gameName, uint32_t score);
  void onLocalMoveMade(const QString& roomId, uint8_t cellIndex);
  void onTicTacToeFinished();

private:
  void setupUI();
  void populateContactList();
  QColor getStatusColor(UserStatus status);
  QString getStatusText(UserStatus status);
  ChatWindow *openChatWindow(const QString &username);

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

  // Game Panel
  QFrame *m_gamePanelFrame;
  QHBoxLayout *m_gamesLayout;
  void setupGamePanel(QVBoxLayout *parentLayout);
  void addGameIcon(const QString &name, const QString &iconPath);

  // Dialogs
  AddFriendDialog *m_addFriendDialog = nullptr;

  // Active Chats
  QMap<QString, ChatWindow *> m_openChats;

  class GameBridge* m_gameBridge = nullptr;
};
