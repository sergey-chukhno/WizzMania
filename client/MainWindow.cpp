#include "MainWindow.h"
#include "AddFriendDialog.h"
#include "AvatarManager.h"
#include "ChatWindow.h"
#include "GameLauncher.h"
#include "NetworkManager.h"
#include <QBuffer>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QGraphicsDropShadowEffect>
#include <QMessageBox>
#include <QPaintEvent>
#include <QPainterPath>
#include <QPair> // Include QPair
#include <QPropertyAnimation>
#include <QTimer>
MainWindow::MainWindow(const QString &username, const QPoint &initialPos,
                       QWidget *parent)
    : QWidget(parent), m_username(username), m_statusMessageInput(nullptr) {
  setWindowTitle("Wizz Mania - " + username);
  setMinimumSize(350, 500);
  resize(400, 600);

  if (!initialPos.isNull()) {
    move(initialPos);
  }

  // Load background
  m_backgroundPixmap = QPixmap(":/assets/login_bg.png");

  // Connect to NetworkManager for contact updates
  connect(&NetworkManager::instance(), &NetworkManager::contactListReceived,
          this, [this](const QList<QPair<QString, int>> &friends) {
            QList<ContactInfo> newContacts;
            for (const auto &pair : friends) {
              QString name = pair.first;
              int statusInt = pair.second;
              UserStatus status = static_cast<UserStatus>(statusInt);
              if (statusInt > 3)
                status = UserStatus::Offline; // Fail-safe

              newContacts.append({name, status, "", QPixmap()});
            }
            setContacts(newContacts);

            if (m_addFriendDialog && m_addFriendDialog->isVisible()) {
              m_addFriendDialog->clearInput();
              m_addFriendDialog->hide();
            }
          });

  // Connect Status Change
  connect(&NetworkManager::instance(), &NetworkManager::contactStatusChanged,
          this, [this](const QString &username, int status) {
            updateContactStatus(username, static_cast<UserStatus>(status), "");
          });

  // Connect Error
  connect(&NetworkManager::instance(), &NetworkManager::errorOccurred, this,
          [this](const QString &msg) {
            if (m_addFriendDialog && m_addFriendDialog->isVisible()) {
              m_addFriendDialog->showError(msg);
            } else {
              QMessageBox::warning(this, "Error", msg);
            }
          });

  // Connect Message Received (Mediator)
  connect(&NetworkManager::instance(), &NetworkManager::messageReceived, this,
          [this](const QString &sender, const QString &text) {
            if (!m_openChats.contains(sender)) {
              onContactDoubleClicked(sender);
            }
            if (m_openChats.contains(sender)) {
              m_openChats[sender]->addMessage(sender, text, false);
              m_openChats[sender]->show();
              m_openChats[sender]->activateWindow();
            }
          });

  // Connect Nudge Received
  connect(&NetworkManager::instance(), &NetworkManager::nudgeReceived, this,
          [this](const QString &sender) {
            if (!m_openChats.contains(sender)) {
              onContactDoubleClicked(sender); // Open window
            }
            if (m_openChats.contains(sender)) {
              m_openChats[sender]->addMessage(sender, sender + " sent a Wizz!",
                                              false);
              m_openChats[sender]->shake();
              m_openChats[sender]->show();
              m_openChats[sender]->activateWindow();
            }
          });

  // Connect Voice Message Received
  connect(
      &NetworkManager::instance(), &NetworkManager::voiceMessageReceived, this,
      [this](const QString &sender, uint16_t duration,
             const std::vector<uint8_t> &data) {
        if (!m_openChats.contains(sender)) {
          onContactDoubleClicked(sender); // Open window
        }
        if (m_openChats.contains(sender)) {
          m_openChats[sender]->addVoiceMessage(sender, duration, data, false);
          m_openChats[sender]->show();
          m_openChats[sender]->activateWindow();
        }
      });

  // Connect Avatar Updated
  connect(&AvatarManager::instance(), &AvatarManager::avatarUpdated, this,
          &MainWindow::updateContactAvatar);

  // Initialize Dialogs
  m_addFriendDialog = new AddFriendDialog(this);
  connect(
      m_addFriendDialog, &AddFriendDialog::addRequested, this,
      [this](const QString &username) {
        // Check if user is already in friends list
        for (const ContactInfo &contact : m_contacts) {
          if (contact.username.compare(username, Qt::CaseInsensitive) == 0) {
            m_addFriendDialog->showError("User is already in your friend list");
            return;
          }
        }

        wizz::Packet pkt(wizz::PacketType::AddContact);
        pkt.writeString(username.toStdString());
        NetworkManager::instance().sendPacket(pkt);
      });

  setupUI();

  // Request my own avatar immediately
  QTimer::singleShot(
      500, [this]() { AvatarManager::instance().getAvatar(m_username, 50); });
}
void MainWindow::paintEvent(QPaintEvent *event) {
  QPainter painter(this);

  if (!m_backgroundPixmap.isNull()) {
    painter.drawPixmap(rect(), m_backgroundPixmap.scaled(
                                   size(), Qt::KeepAspectRatioByExpanding,
                                   Qt::SmoothTransformation));
  }

  QWidget::paintEvent(event);
}

QColor MainWindow::getStatusColor(UserStatus status) {
  switch (status) {
  case UserStatus::Online:
    return QColor("#4CAF50"); // Green
  case UserStatus::Away:
    return QColor("#FF9800"); // Orange
  case UserStatus::Busy:
    return QColor("#F44336"); // Red
  case UserStatus::Offline:
    return QColor("#9E9E9E"); // Gray
  }
  return QColor("#9E9E9E");
}

QString MainWindow::getStatusText(UserStatus status) {
  switch (status) {
  case UserStatus::Online:
    return "Online";
  case UserStatus::Away:
    return "Away";
  case UserStatus::Busy:
    return "Busy";
  case UserStatus::Offline:
    return "Appear Offline";
  }
  return "Offline";
}

void MainWindow::setupUI() {
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(15, 15, 15, 15);
  mainLayout->setSpacing(10);

  // --- Main Glass Card ---
  QFrame *glassCard = new QFrame(this);
  glassCard->setObjectName("mainGlassCard");
  glassCard->setStyleSheet(R"(
        #mainGlassCard {
            background-color: rgba(255, 255, 255, 45);
            border: 2px solid rgba(255, 255, 255, 150);
            border-radius: 25px;
        }
    )");

  QGraphicsDropShadowEffect *cardShadow = new QGraphicsDropShadowEffect(this);
  cardShadow->setBlurRadius(40);
  cardShadow->setColor(QColor(0, 60, 120, 80));
  cardShadow->setOffset(0, 10);
  glassCard->setGraphicsEffect(cardShadow);

  QVBoxLayout *cardLayout = new QVBoxLayout(glassCard);
  cardLayout->setContentsMargins(20, 20, 20, 15);
  cardLayout->setSpacing(12);

  // --- Header: Butterfly + Title ---
  QHBoxLayout *headerLayout = new QHBoxLayout();

  QLabel *butterflyIcon = new QLabel(glassCard);
  QPixmap butterfly(":/assets/butterfly.png");
  butterflyIcon->setPixmap(
      butterfly.scaled(40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation));
  butterflyIcon->setFixedSize(40, 40);
  butterflyIcon->setStyleSheet("background: transparent;");

  QLabel *titleLabel = new QLabel("Wizz Mania", glassCard);
  titleLabel->setStyleSheet("font-size: 22px; font-weight: 700; color: "
                            "#1a2530; background: transparent;");

  QVBoxLayout *titleLayout = new QVBoxLayout();
  titleLayout->setSpacing(0);
  titleLayout->addWidget(titleLabel);

  QLabel *subtitleLabel =
      new QLabel("Undefined Behaviour Included for Free", glassCard);
  subtitleLabel->setStyleSheet(
      "font-size: 12px; font-weight: 500; color: #5a6b7c; "
      "font-style: italic; background: transparent; padding-left: 2px;");
  titleLayout->addWidget(subtitleLabel);

  headerLayout->addWidget(butterflyIcon);
  headerLayout->addLayout(titleLayout);
  headerLayout->addStretch();
  cardLayout->addLayout(headerLayout);

  // --- User Profile Section ---
  QFrame *profileFrame = new QFrame(glassCard);
  profileFrame->setStyleSheet(R"(
        background-color: rgba(255, 255, 255, 30);
        border: 1px solid rgba(200, 230, 255, 150);
        border-radius: 15px;
    )");

  QHBoxLayout *profileLayout = new QHBoxLayout(profileFrame);
  profileLayout->setContentsMargins(12, 10, 12, 10);

  // Avatar
  m_avatarLabel = new QLabel(profileFrame);
  m_avatarLabel->setPixmap(AvatarManager::instance().getAvatar(m_username, 50));
  m_avatarLabel->setFixedSize(50, 50);
  m_avatarLabel->setStyleSheet("background: transparent;");
  m_avatarLabel->setCursor(Qt::PointingHandCursor);

  // Make Avatar Clickable using Event Filter or lambda
  // Simple way: wrap in a QPushButton with icon? Or make a transparent button
  // on top? Let's use a transparent button on top for simplicity
  QPushButton *avatarBtn = new QPushButton(m_avatarLabel);
  avatarBtn->setFixedSize(50, 50);
  avatarBtn->setStyleSheet("background: transparent; border: none;");
  avatarBtn->setCursor(Qt::PointingHandCursor);
  connect(avatarBtn, &QPushButton::clicked, this, &MainWindow::onAvatarClicked);

  // User info
  QVBoxLayout *userInfoLayout = new QVBoxLayout();
  userInfoLayout->setSpacing(4);

  m_usernameLabel = new QLabel(m_username, profileFrame);
  m_usernameLabel->setStyleSheet("font-size: 16px; font-weight: 600; color: "
                                 "#1a2530; background: transparent;");

  // Status dropdown
  m_statusCombo = new QComboBox(profileFrame);
  m_statusCombo->addItem("🟢 Online");
  m_statusCombo->addItem("🟠 Away");
  m_statusCombo->addItem("🔴 Busy");
  m_statusCombo->addItem("⚫ Appear Offline");
  m_statusCombo->setStyleSheet(R"(
        QComboBox {
            background-color: rgba(255, 255, 255, 80);
            border: 1px solid rgba(200, 220, 240, 150);
            border-radius: 10px;
            padding: 4px 10px;
            font-size: 12px;
            color: #2d3748;
        }
        QComboBox:hover {
            border: 1px solid rgba(100, 180, 255, 200);
        }
        QComboBox::drop-down {
            border: none;
        }
    )");
  m_statusCombo->setFixedWidth(160);
  connect(m_statusCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &MainWindow::onStatusChanged);

  userInfoLayout->addWidget(m_usernameLabel);
  userInfoLayout->addWidget(m_statusCombo);

  profileLayout->addWidget(m_avatarLabel);
  profileLayout->addLayout(userInfoLayout);
  profileLayout->addStretch();

  cardLayout->addWidget(profileFrame);

  // --- Friends Header Row ---
  QWidget *friendHeader = new QWidget(glassCard);
  QHBoxLayout *friendHeaderLayout = new QHBoxLayout(friendHeader);
  friendHeaderLayout->setContentsMargins(0, 5, 0, 5);

  QLabel *friendsLabel = new QLabel("Friends", friendHeader);
  friendsLabel->setStyleSheet("font-size: 14px; font-weight: 600; color: "
                              "#4a5568; background: transparent;");

  QPushButton *addFriendBtn = new QPushButton("+", friendHeader);
  addFriendBtn->setFixedSize(24, 24);
  addFriendBtn->setCursor(Qt::PointingHandCursor);
  addFriendBtn->setStyleSheet(R"(
        QPushButton {
            background-color: rgba(80, 180, 255, 40);
            border: 1px solid rgba(80, 180, 255, 100);
            border-radius: 12px;
            color: #2d3748;
            font-weight: bold;
            padding-bottom: 2px;
        }
        QPushButton:hover {
            background-color: rgba(80, 180, 255, 80);
        }
    )");
  connect(addFriendBtn, &QPushButton::clicked, this,
          &MainWindow::onAddFriendClicked);

  QPushButton *removeFriendBtn = new QPushButton("-", friendHeader);
  removeFriendBtn->setFixedSize(24, 24);
  removeFriendBtn->setCursor(Qt::PointingHandCursor);
  removeFriendBtn->setStyleSheet(R"(
        QPushButton {
            background-color: rgba(255, 80, 80, 40);
            border: 1px solid rgba(255, 80, 80, 100);
            border-radius: 12px;
            color: #2d3748;
            font-weight: bold;
            padding-bottom: 2px;
        }
        QPushButton:hover {
            background-color: rgba(255, 80, 80, 80);
        }
    )");
  connect(removeFriendBtn, &QPushButton::clicked, this,
          &MainWindow::onRemoveFriendClicked);

  friendHeaderLayout->addWidget(friendsLabel);
  friendHeaderLayout->addStretch();
  friendHeaderLayout->addWidget(addFriendBtn);
  friendHeaderLayout->addWidget(removeFriendBtn);

  cardLayout->addWidget(friendHeader);

  // --- Contact List ---
  m_contactList = new QListWidget(glassCard);
  m_contactList->setStyleSheet(R"(
        QListWidget {
            background-color: rgba(255, 255, 255, 30);
            border: 1px solid rgba(200, 230, 255, 120);
            border-radius: 12px;
        }
        QListWidget::item {
            border-bottom: 1px solid rgba(200, 220, 240, 80);
        }
        QListWidget::item:hover {
            background-color: rgba(100, 180, 255, 40);
        }
        QListWidget::item:selected {
            background-color: rgba(80, 160, 255, 80);
        }
    )");
  m_contactList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  m_contactList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  connect(m_contactList, &QListWidget::itemDoubleClicked, this,
          [this](QListWidgetItem *item) {
            onContactDoubleClicked(item->data(Qt::UserRole).toString());
          });

  cardLayout->addWidget(m_contactList, 1); // Stretch

  // --- Game Panel ---
  setupGamePanel(cardLayout);

  mainLayout->addWidget(glassCard);

  // Add sample contacts for testing
  // Remove hardcoded sample data
  // Contacts will be loaded from NetworkManager signal
  // QList<ContactInfo> sampleContacts = ...
  // setContacts(sampleContacts);
}

void MainWindow::setContacts(const QList<ContactInfo> &contacts) {
  // Merge new contacts with existing ones to preserve avatars if already
  // loaded But since setContacts usually replaces the list, we should check
  // if we need to re-request avatars

  // For MVP: Just replace list, but request avatars for everyone
  // Optimization: In real app, check if we already have it.

  m_contacts = contacts;
  populateContactList();

  // Initialize fetching avatars for all contacts
  for (const auto &contact : m_contacts) {
    AvatarManager::instance().getAvatar(contact.username, 36);
  }

  // Also request my own avatar to ensure it's up to date
  AvatarManager::instance().getAvatar(m_username, 50);
}

void MainWindow::populateContactList() {
  m_contactList->clear();

  // Sort: Online first, then Away, then Busy, then Offline
  QList<ContactInfo> sorted = m_contacts;
  std::sort(sorted.begin(), sorted.end(),
            [](const ContactInfo &a, const ContactInfo &b) {
              return static_cast<int>(a.status) < static_cast<int>(b.status);
            });

  for (const ContactInfo &contact : sorted) {
    QWidget *itemWidget = new QWidget();
    QHBoxLayout *itemLayout = new QHBoxLayout(itemWidget);
    itemLayout->setContentsMargins(15, 12, 15, 12);
    itemLayout->setSpacing(15);

    // Avatar
    QLabel *avatar = new QLabel();
    if (contact.avatar.isNull()) {
      avatar->setPixmap(
          AvatarManager::instance().getAvatar(contact.username, 36));
    } else {
      avatar->setPixmap(contact.avatar.scaled(36, 36, Qt::KeepAspectRatio,
                                              Qt::SmoothTransformation));
    }
    avatar->setFixedSize(36, 36);
    avatar->setStyleSheet("background: transparent;");

    // Status indicator
    QLabel *statusDot = new QLabel();
    statusDot->setFixedSize(10, 10);
    statusDot->setStyleSheet(
        QString("background-color: %1; border-radius: 5px;")
            .arg(getStatusColor(contact.status).name()));

    // Name and status message
    QVBoxLayout *textLayout = new QVBoxLayout();
    textLayout->setSpacing(2);

    QLabel *nameLabel = new QLabel(contact.username);
    nameLabel->setStyleSheet("font-size: 13px; font-weight: 600; color: "
                             "#1a2530; background: transparent;");

    QString statusText = contact.statusMessage.isEmpty()
                             ? getStatusText(contact.status)
                             : contact.statusMessage;
    QLabel *statusLabel = new QLabel(statusText);
    statusLabel->setStyleSheet(
        "font-size: 11px; color: #718096; background: transparent;");

    textLayout->addWidget(nameLabel);
    textLayout->addWidget(statusLabel);

    itemLayout->addWidget(avatar);
    itemLayout->addWidget(statusDot, 0, Qt::AlignVCenter);
    itemLayout->addLayout(textLayout);
    itemLayout->addStretch();

    QListWidgetItem *item = new QListWidgetItem(m_contactList);
    item->setData(Qt::UserRole, contact.username);
    item->setSizeHint(itemWidget->sizeHint());
    m_contactList->setItemWidget(item, itemWidget);
  }
}

void MainWindow::updateContactStatus(const QString &username, UserStatus status,
                                     const QString &statusMessage) {
  for (ContactInfo &contact : m_contacts) {
    if (contact.username == username) {
      contact.status = status;
      if (!statusMessage.isEmpty()) {
        contact.statusMessage = statusMessage;
      }
      break;
    }
  }
  populateContactList();
}

void MainWindow::updateContactAvatar(const QString &username,
                                     const QPixmap &avatar) {
  if (username == m_username) {
    // Update my own avatar
    m_avatarLabel->setPixmap(avatar.scaled(
        50, 50, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));

    // Apply circular mask again just in case
    QPixmap circular(50, 50);
    circular.fill(Qt::transparent);
    QPainter painter(&circular);
    painter.setRenderHint(QPainter::Antialiasing);
    QPainterPath path;
    path.addEllipse(0, 0, 50, 50);
    painter.setClipPath(path);
    painter.drawPixmap(0, 0,
                       avatar.scaled(50, 50, Qt::KeepAspectRatioByExpanding,
                                     Qt::SmoothTransformation));
    m_avatarLabel->setPixmap(circular);
    return;
  }

  // Update friend's avatar
  bool found = false;
  for (ContactInfo &contact : m_contacts) {
    if (contact.username == username) {
      contact.avatar = avatar;
      found = true;
      break;
    }
  }

  if (found) {
    populateContactList();
  }
}

void MainWindow::onStatusChanged(int index) {
  m_currentStatus = static_cast<UserStatus>(index);
  emit statusChanged(m_currentStatus,
                     m_statusMessageInput ? m_statusMessageInput->text() : "");

  // Inform NetworkManager
  // We need to implement sendStatusChange in NetworkManager first, but for
  // now we can manually construct packet or assume server knows from
  // heartbeat? Protocol says: ContactStatusChange(203). We should send
  // this.
  wizz::Packet statusPkt(wizz::PacketType::ContactStatusChange);
  statusPkt.writeInt(static_cast<uint32_t>(m_currentStatus));
  statusPkt.writeString(""); // Optional message
  NetworkManager::instance().sendPacket(statusPkt);
}

void MainWindow::onAddFriendClicked() {
  m_addFriendDialog->clearInput();
  m_addFriendDialog->show();
  m_addFriendDialog->raise();
  m_addFriendDialog->activateWindow();
}

void MainWindow::onRemoveFriendClicked() {
  QListWidgetItem *selected = m_contactList->currentItem();
  if (!selected) {
    QMessageBox::information(this, "Select Contact",
                             "Please select a friend to remove.");
    return;
  }

  QString username = selected->data(Qt::UserRole).toString();
  auto reply = QMessageBox::question(
      this, "Remove Friend",
      QString("Are you sure you want to remove %1?").arg(username),
      QMessageBox::Yes | QMessageBox::No);

  if (reply == QMessageBox::Yes) {
    wizz::Packet pkt(wizz::PacketType::RemoveContact);
    pkt.writeString(username.toStdString());
    NetworkManager::instance().sendPacket(pkt);
  }
}

void MainWindow::onSendMessage() {
  // Deprecated: Chat Preview removed in favor of Game Panel
}

void MainWindow::onContactDoubleClicked(const QString &username) {
  if (m_openChats.contains(username)) {
    ChatWindow *w = m_openChats[username];
    w->show();
    w->raise();
    w->activateWindow();
    return;
  }

  // Position relative to Main Window (Right side)
  QPoint startPos;
  if (this->isVisible()) {
    startPos = this->geometry().topRight() + QPoint(20, 0); // 20px offset
  }

  ChatWindow *w = new ChatWindow(username, startPos);
  connect(w, &ChatWindow::windowClosed, this, &MainWindow::onChatWindowClosed);
  connect(w, &ChatWindow::sendMessage, this, [username](const QString &text) {
    wizz::Packet pkt(wizz::PacketType::DirectMessage);
    pkt.writeString(username.toStdString()); // Target
    pkt.writeString(text.toStdString());     // Message
    NetworkManager::instance().sendPacket(pkt);
  });
  connect(w, &ChatWindow::sendNudge, this, [username]() {
    wizz::Packet pkt(wizz::PacketType::Nudge);
    pkt.writeString(username.toStdString()); // Target
    NetworkManager::instance().sendPacket(pkt);
  });

  connect(w, &ChatWindow::sendVoiceMessage, this,
          [username](uint16_t duration, const std::vector<uint8_t> &data) {
            NetworkManager::instance().sendVoiceMessage(username, duration,
                                                        data);
          });

  w->show();
  m_openChats.insert(username, w);
}

void MainWindow::onChatWindowClosed(const QString &partnerName) {
  m_openChats.remove(partnerName);
}

void MainWindow::onAvatarClicked() {
  QString fileName = QFileDialog::getOpenFileName(
      this, "Change Avatar", "", "Images (*.png *.jpg *.jpeg)");
  if (fileName.isEmpty())
    return;

  QPixmap pix(fileName);
  if (pix.isNull()) {
    QMessageBox::warning(this, "Error", "Failed to load image");
    return;
  }

  // Immediate local update for responsiveness
  updateContactAvatar(m_username, pix);

  // Send to server
  QByteArray bytes;
  QBuffer buffer(&bytes);
  buffer.open(QIODevice::WriteOnly);
  pix.save(&buffer, "PNG");

  NetworkManager::instance().sendUpdateAvatar(bytes);
}

// --- Game Panel Implementation ---

void MainWindow::setupGamePanel(QVBoxLayout *parentLayout) {
  m_gamePanelFrame = new QFrame(this);
  m_gamePanelFrame->setStyleSheet(R"(
        background-color: rgba(255, 255, 255, 40);
        border: 1px solid rgba(200, 230, 255, 150);
        border-radius: 12px;
    )");

  QVBoxLayout *panelLayout = new QVBoxLayout(m_gamePanelFrame);
  panelLayout->setContentsMargins(12, 10, 12, 10);
  panelLayout->setSpacing(8);

  QLabel *titleLabel = new QLabel("SELECT A GAME TO PLAY", m_gamePanelFrame);
  titleLabel->setStyleSheet("font-size: 11px; font-weight: 800; color: "
                            "#4a5568; background: transparent; "
                            "letter-spacing: 1px; text-transform: uppercase;");
  titleLabel->setAlignment(Qt::AlignCenter);
  panelLayout->addWidget(titleLabel);

  // Horizontal layout for game icons
  m_gamesLayout = new QHBoxLayout();
  m_gamesLayout->setSpacing(15);
  m_gamesLayout->addStretch();

  QString tileTwisterDir = GameLauncher::resolveWorkingDir("TileTwister");
  QString tileTwisterIcon =
      QDir(tileTwisterDir).absoluteFilePath("assets/logo.png");

  QString brickBreakerDir = GameLauncher::resolveWorkingDir("BrickBreaker");
  QString brickBreakerIcon =
      QDir(brickBreakerDir).absoluteFilePath("assets/logo.png");

  // Placeholder for where games will be added
  addGameIcon("TileTwister", tileTwisterIcon);
  addGameIcon("BrickBreaker", brickBreakerIcon);

  m_gamesLayout->addStretch();
  panelLayout->addLayout(m_gamesLayout);

  parentLayout->addWidget(m_gamePanelFrame);
}

void MainWindow::addGameIcon(const QString &name, const QString &iconPath) {
  if (!m_gamesLayout)
    return;

  // Create Launch Button
  QPushButton *gameBtn = new QPushButton(m_gamePanelFrame);
  gameBtn->setFixedSize(64, 64);
  gameBtn->setCursor(Qt::PointingHandCursor);
  gameBtn->setToolTip("Play " + name);

  // Install Event Filter for Hover Animation
  gameBtn->installEventFilter(this);

  // Load Icon
  QPixmap pix(iconPath);
  if (!iconPath.isEmpty() && !pix.isNull()) {
    gameBtn->setIcon(QIcon(pix));
    gameBtn->setIconSize(QSize(48, 48));
  } else {
    // Fallback: Colorful Text Icon
    gameBtn->setText(name.left(1).toUpper());
    QFont font = gameBtn->font();
    font.setPixelSize(32);
    font.setBold(true);
    gameBtn->setFont(font);
  }

  gameBtn->setStyleSheet(R"(
        QPushButton {
            background-color: rgba(255, 255, 255, 60);
            border: 2px solid rgba(200, 230, 255, 150);
            border-radius: 32px;
            padding: 0px;
            color: #2d3748;
        }
        QPushButton:hover {
            background-color: rgba(100, 200, 255, 80);
            border: 2px solid rgba(100, 200, 255, 200);
        }
        QPushButton:pressed {
            background-color: rgba(80, 180, 255, 120);
        }
    )");

  // Connect click to launch
  connect(gameBtn, &QPushButton::clicked, this,
          [name]() { GameLauncher::launchGame(name); });

  // Insert before the last stretch
  // m_gamesLayout has: stretch, [games...], stretch
  // count is at least 2. insert at count-1
  m_gamesLayout->insertWidget(m_gamesLayout->count() - 1, gameBtn);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
  if (QPushButton *btn = qobject_cast<QPushButton *>(obj)) {
    if (event->type() == QEvent::Enter) {
      // Hover Enter: Scale Up Icon
      QPropertyAnimation *anim = new QPropertyAnimation(btn, "iconSize");
      anim->setDuration(150);
      anim->setStartValue(btn->iconSize());
      anim->setEndValue(QSize(56, 56));
      anim->start(QAbstractAnimation::DeleteWhenStopped);

      // Move Up slightly via margin (Simulated with geometry check or
      // stylesheet? Stylesheet is safer for layout but complicates animation.
      // Let's stick to icon size + cursor for now as "professional" inner
      // scale) Or animate geometry: btn->move(btn->x(), btn->y() - 2); // risky
      // with layout
      return true;
    } else if (event->type() == QEvent::Leave) {
      // Hover Leave: Scale Down Icon
      QPropertyAnimation *anim = new QPropertyAnimation(btn, "iconSize");
      anim->setDuration(150);
      anim->setStartValue(btn->iconSize());
      anim->setEndValue(QSize(48, 48));
      anim->start(QAbstractAnimation::DeleteWhenStopped);
      return true;
    }
  }
  return QWidget::eventFilter(obj, event);
}
