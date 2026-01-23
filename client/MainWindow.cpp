#include "MainWindow.h"
#include "AddFriendDialog.h"
#include "ChatWindow.h"
#include "NetworkManager.h"
#include <QGraphicsDropShadowEffect>
#include <QMessageBox>
#include <QPaintEvent>
#include <QPair> // Include QPair
#include <QTimer>

MainWindow::MainWindow(const QString &username, QWidget *parent)
    : QWidget(parent), m_username(username) {
  setWindowTitle("Wizz Mania - " + username);
  setMinimumSize(350, 500);
  resize(400, 600);

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

QPixmap MainWindow::createAvatarWithInitials(const QString &name, int size) {
  QPixmap avatar(size, size);
  avatar.fill(Qt::transparent);

  QPainter painter(&avatar);
  painter.setRenderHint(QPainter::Antialiasing);

  // Generate color from name hash
  uint hash = qHash(name);
  QColor bgColor = QColor::fromHsl(hash % 360, 150, 120);

  // Draw circle
  painter.setBrush(bgColor);
  painter.setPen(Qt::NoPen);
  painter.drawEllipse(0, 0, size, size);

  // Draw initials
  QString initials;
  QStringList parts = name.split('_');
  if (parts.isEmpty())
    parts = name.split(' ');
  for (const QString &part : parts) {
    if (!part.isEmpty())
      initials += part[0].toUpper();
    if (initials.length() >= 2)
      break;
  }
  if (initials.isEmpty() && !name.isEmpty()) {
    initials = name.left(2).toUpper();
  }

  painter.setPen(Qt::white);
  QFont font("SF Pro Display", size / 3, QFont::Bold);
  painter.setFont(font);
  painter.drawText(QRect(0, 0, size, size), Qt::AlignCenter, initials);

  return avatar;
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

  headerLayout->addWidget(butterflyIcon);
  headerLayout->addWidget(titleLabel);
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
  m_avatarLabel->setPixmap(createAvatarWithInitials(m_username, 50));
  m_avatarLabel->setFixedSize(50, 50);
  m_avatarLabel->setStyleSheet("background: transparent;");
  m_avatarLabel->setCursor(Qt::PointingHandCursor);

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

  // --- Chat Preview Panel ---
  m_chatPreviewFrame = new QFrame(glassCard);
  m_chatPreviewFrame->setStyleSheet(R"(
        background-color: rgba(255, 255, 255, 40);
        border: 1px solid rgba(200, 230, 255, 150);
        border-radius: 12px;
    )");

  QVBoxLayout *chatPreviewLayout = new QVBoxLayout(m_chatPreviewFrame);
  chatPreviewLayout->setContentsMargins(12, 10, 12, 10);
  chatPreviewLayout->setSpacing(8);

  m_chatPreviewLabel =
      new QLabel("Select a friend to start chatting", m_chatPreviewFrame);
  m_chatPreviewLabel->setStyleSheet(
      "font-size: 12px; color: #718096; background: transparent;");
  m_chatPreviewLabel->setAlignment(Qt::AlignCenter);
  chatPreviewLayout->addWidget(m_chatPreviewLabel);

  // Message input row
  QHBoxLayout *inputRow = new QHBoxLayout();

  m_messageInput = new QLineEdit(m_chatPreviewFrame);
  m_messageInput->setPlaceholderText("Send a message...");
  m_messageInput->setStyleSheet(R"(
        background-color: rgba(255, 255, 255, 60);
        border: 1px solid rgba(200, 220, 240, 150);
        border-radius: 15px;
        padding: 8px 15px;
        font-size: 13px;
        color: #2d3748;
    )");

  m_sendButton = new QPushButton("▶", m_chatPreviewFrame);
  m_sendButton->setFixedSize(36, 36);
  m_sendButton->setCursor(Qt::PointingHandCursor);
  m_sendButton->setStyleSheet(R"(
        QPushButton {
            background-color: rgba(80, 180, 255, 150);
            border: 2px solid rgba(150, 220, 255, 200);
            border-radius: 18px;
            font-size: 14px;
            color: white;
        }
        QPushButton:hover {
            background-color: rgba(100, 200, 255, 200);
        }
    )");
  connect(m_sendButton, &QPushButton::clicked, this,
          &MainWindow::onSendMessage);
  connect(m_messageInput, &QLineEdit::returnPressed, this,
          &MainWindow::onSendMessage);

  inputRow->addWidget(m_messageInput);
  inputRow->addWidget(m_sendButton);
  chatPreviewLayout->addLayout(inputRow);

  cardLayout->addWidget(m_chatPreviewFrame);

  mainLayout->addWidget(glassCard);

  // Add sample contacts for testing
  // Remove hardcoded sample data
  // Contacts will be loaded from NetworkManager signal
  // QList<ContactInfo> sampleContacts = ...
  // setContacts(sampleContacts);
}

void MainWindow::setContacts(const QList<ContactInfo> &contacts) {
  m_contacts = contacts;
  populateContactList();
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
      avatar->setPixmap(createAvatarWithInitials(contact.username, 36));
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

void MainWindow::onStatusChanged(int index) {
  m_currentStatus = static_cast<UserStatus>(index);
  emit statusChanged(m_currentStatus,
                     m_statusMessageInput ? m_statusMessageInput->text() : "");

  // Inform NetworkManager
  // We need to implement sendStatusChange in NetworkManager first, but for now
  // we can manually construct packet or assume server knows from heartbeat?
  // Protocol says: ContactStatusChange(203). We should send this.
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
  QString message = m_messageInput->text().trimmed();
  if (message.isEmpty())
    return;

  // Get selected contact
  QListWidgetItem *selected = m_contactList->currentItem();
  if (selected) {
    QString recipient = selected->data(Qt::UserRole).toString();
    // TODO: Send message via NetworkManager
    m_messageInput->clear();
    m_chatPreviewLabel->setText("You: " + message);
  }
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

  w->show();
  m_openChats.insert(username, w);
}

void MainWindow::onChatWindowClosed(const QString &partnerName) {
  m_openChats.remove(partnerName);
}
