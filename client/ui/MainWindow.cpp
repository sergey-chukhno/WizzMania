#include "MainWindow.h"
#include "../../common/GameIPC.h"
#include "../../common/TicTacToeIPC.h"
#include "../logic/AvatarManager.h"
#include "../logic/GameBridge.h"
#include "../logic/GameLauncher.h"
#include "../network/NetworkManager.h"
#include "AddFriendDialog.h"
#include "ChatWindow.h"
#include "arcade/CategoryPopup.h"
#include "arcade/GameSelectionPopup.h"
#include "theme/ThemeEngine.h"
#include "widgets/ContactDelegate.h"
#include "widgets/SearchBar.h"
#include "widgets/TitleBar.h"
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QBuffer>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QGraphicsDropShadowEffect>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QMessageBox>
#include <QPaintEvent>
#include <QPainterPath>
#include <QPair> // Include QPair
#include <QPropertyAnimation>
#include <QThread>
#include <QTimer>
#include <iostream>

MainWindow::MainWindow(const QString &username, const QPoint &initialPos,
                       QWidget *parent)
    : QWidget(parent), m_username(username), m_statusMessageInput(nullptr) {
  std::cout << "[MainWindow] Constructing for user: " << username.toStdString()
            << std::endl;
  setWindowTitle("Wizz Mania - " + username);
  setMinimumSize(350, 500);
  resize(350, 700);
  setAttribute(Qt::WA_DeleteOnClose);
  // Frameless window — TitleBar handles dragging and chrome
  setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
  setAttribute(Qt::WA_TranslucentBackground, false);
  setMouseTracking(true); // Required for resize cursor updates

  // Global style now applied via QApplication in main.cpp

  m_gameBridge = new GameBridge(m_username, this);
  connect(m_gameBridge, &GameBridge::localGameStatusChanged, this,
          &MainWindow::onLocalGameStatusChanged);
  connect(m_gameBridge, &GameBridge::localMoveMade, this,
          &MainWindow::onLocalMoveMade);
  connect(m_gameBridge, &GameBridge::ticTacToeFinished, this,
          &MainWindow::onTicTacToeFinished);
  connect(m_gameBridge, &GameBridge::rematchRequested, this,
          [this](const QString &opponent) {
            // Player pressed "Play Again" inside the game — send a fresh
            // TicTacToe invite
            NetworkManager::instance().sendGameInvite(opponent, "TicTacToe");
          });
  m_gameBridge->startGameIPC();

  std::cout << "[MainWindow] GameBridge Setup complete" << std::endl;

  if (!initialPos.isNull()) {
    move(initialPos);
  }

  // Load background
  m_backgroundPixmap = QPixmap(":/assets/login_bg.png");

  // Connect to NetworkManager for contact updates
  connect(&NetworkManager::instance(), &NetworkManager::contactListReceived,
          this,
          [this](const QList<std::tuple<QString, int, QString>> &friends) {
            std::cout << "[MainWindow] Received contact list update, count: "
                      << friends.size() << std::endl;
            QList<ContactInfo> newContacts;
            for (const auto &tup : friends) {
              QString name = std::get<0>(tup);
              int statusInt = std::get<1>(tup);
              QString statusMsg = std::get<2>(tup);

              UserStatus status = static_cast<UserStatus>(statusInt);
              if (statusInt > 3)
                status = UserStatus::Offline; // Fail-safe

              newContacts.append(
                  {name, status, statusMsg, QPixmap(), false, "", 0});
            }
            setContacts(newContacts);

            if (m_addFriendDialog && m_addFriendDialog->isVisible()) {
              m_addFriendDialog->clearInput();
              m_addFriendDialog->hide();
            }
          });

  // Connect Status Change
  connect(
      &NetworkManager::instance(), &NetworkManager::contactStatusChanged, this,
      [this](const QString &username, int status, const QString &statusMsg) {
        std::cout << "[MainWindow] Status update for: "
                  << username.toStdString() << " to " << status << std::endl;
        updateContactStatus(username, static_cast<UserStatus>(status),
                            statusMsg);
      });

  // Connect Error
  connect(&NetworkManager::instance(), &NetworkManager::errorOccurred, this,
          [this](const QString &msg) {
            if (m_addFriendDialog && m_addFriendDialog->isVisible()) {
              m_addFriendDialog->showError(msg);
            } else {
              QMessageBox::warning(this, "Error", msg);
            }
          }); // Connect Message Received (Mediator)
  connect(&NetworkManager::instance(), &NetworkManager::messageReceived, this,
          [this](const QString &sender, const QString &text) {
            QString lowerSender = sender.toLower();
            if (!m_openChats.contains(lowerSender)) {
              onContactDoubleClicked(sender);
            }
            if (m_openChats.contains(lowerSender)) {
              if (text.startsWith("{\"type\":\"arcade_share\"")) {
                QJsonDocument doc = QJsonDocument::fromJson(text.toUtf8());
                if (!doc.isNull() && doc.isObject()) {
                  QJsonObject obj = doc.object();
                  QString cat = obj["category"].toString();
                  QString txt = obj["text"].toString();
                  m_openChats[lowerSender]->addRichMessage(sender, cat, txt,
                                                           false);
                } else {
                  m_openChats[lowerSender]->addMessage(sender, text, false);
                }
              } else {
                m_openChats[lowerSender]->addMessage(sender, text, false);
              }
              m_openChats[lowerSender]->show();
              m_openChats[lowerSender]->activateWindow();
            }
          });

  // Connect Nudge Received
  connect(&NetworkManager::instance(), &NetworkManager::nudgeReceived, this,
          [this](const QString &sender) {
            QString lowerSender = sender.toLower();
            if (!m_openChats.contains(lowerSender)) {
              onContactDoubleClicked(sender); // Open window
            }
            if (m_openChats.contains(lowerSender)) {
              m_openChats[lowerSender]->addMessage(
                  sender, sender + " sent a Wizz!", false);
              m_openChats[lowerSender]->shake();
              m_openChats[lowerSender]->show();
              m_openChats[lowerSender]->activateWindow();
            }
          });

  // Connect Voice Message Received
  connect(&NetworkManager::instance(), &NetworkManager::voiceMessageReceived,
          this,
          [this](const QString &sender, uint16_t duration,
                 const std::vector<uint8_t> &data) {
            QString lowerSender = sender.toLower();
            if (!m_openChats.contains(lowerSender)) {
              onContactDoubleClicked(sender); // Open window
            }
            if (m_openChats.contains(lowerSender)) {
              m_openChats[lowerSender]->addVoiceMessage(sender, duration, data,
                                                        false);
              m_openChats[lowerSender]->show();
              m_openChats[lowerSender]->activateWindow();
            }
          });
  ;

  // Connect Avatar Updated
  connect(&AvatarManager::instance(), &AvatarManager::avatarUpdated, this,
          &MainWindow::updateContactAvatar);

  // Handle Game Status
  connect(
      &NetworkManager::instance(), &NetworkManager::gameStatusChanged, this,
      [this](const QString &username, const QString &gameName, uint32_t score) {
        updateContactGameStatus(username, gameName, score);
      });

  // Handle Rich Presence
  connect(&NetworkManager::instance(), &NetworkManager::richPresenceReceived,
          this,
          [this](const QString &username, int type, const QString &name,
                 const QString &detail) {
            // For now, we reuse the game status logic but we could expand it
            // later
            if (type == 1) { // Gaming
              updateContactGameStatus(username, name,
                                      0); // Detail could be parsed for score
            } else if (type == 0) {       // None
              updateContactGameStatus(username, "", 0);
            }
          });

  // Handle Game Invites
  connect(&NetworkManager::instance(), &NetworkManager::gameInviteReceived,
          this, [this](const QString &sender, const QString &gameName) {
            ChatWindow *chatW = openChatWindow(sender);
            chatW->addGameInvite(sender, gameName);
            chatW->shake(); // alert the user visually
            chatW->show();
            chatW->activateWindow();
          });

  // Handle Game Invite Responses
  connect(
      &NetworkManager::instance(), &NetworkManager::gameInviteResponseReceived,
      this,
      [this](const QString &target, const QString &gameName, bool accepted) {
        ChatWindow *chatW = openChatWindow(target);
        if (accepted) {
          chatW->addMessage("System",
                            target + " accepted your " + gameName +
                                " challenge! Waiting for server...",
                            false);
        } else {
          chatW->addMessage(
              "System", target + " declined your " + gameName + " challenge.",
              false);
        }
        chatW->show();
        chatW->activateWindow();
      });

  // Handle Game Starts
  connect(&NetworkManager::instance(), &NetworkManager::gameStartReceived, this,
          [this](const QString &gameName, const QString &roomId, char symbol,
                 const QString &opponent) {
            if (gameName == "TicTacToe") {
              QPixmap avatar =
                  AvatarManager::instance().getAvatar(opponent, 64);
              m_gameBridge->startTicTacToe(m_username, roomId, opponent, symbol,
                                           avatar);
            }
          });

  // Relay incoming opponent moves into shared memory
  connect(&NetworkManager::instance(), &NetworkManager::gameMoveReceived, this,
          [this](const QString &roomId, uint8_t cellIndex) {
            m_gameBridge->receiveNetworkMove(cellIndex);
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

        std::string normalizedUser = username.trimmed().toLower().toStdString();
        wizz::Packet pkt(wizz::PacketType::AddContact);
        pkt.writeString(normalizedUser);
        NetworkManager::instance().sendPacket(pkt);
      });

  setupUI();
  std::cout << "[MainWindow] UI Setup complete" << std::endl;

  // Initialize with any cached contacts that arrived before we connected
  QList<ContactInfo> initialContacts;
  for (const auto &c : NetworkManager::instance().getContacts()) {
    ContactInfo info;
    info.username = std::get<0>(c);
    info.status = static_cast<UserStatus>(std::get<1>(c));
    info.statusMessage = std::get<2>(c);
    initialContacts.append(info);
  }
  setContacts(initialContacts);

  // Request my own avatar immediately
  QTimer::singleShot(
      500, [this]() { AvatarManager::instance().getAvatar(m_username, 50); });
}

MainWindow::~MainWindow() {
  std::cout << "[MainWindow] Destructor starting..." << std::endl;

  for (auto chatW : m_openChats) {
    if (chatW) {
      chatW->close();
    }
  }
  m_openChats.clear();

  std::cout << "[MainWindow] Destructor complete." << std::endl;
}

void MainWindow::paintEvent(QPaintEvent *event) {
  Q_UNUSED(event);
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

  // Rounded clip — no manual shadow (mask handles corner clipping)
  QPainterPath path;
  path.addRoundedRect(rect(), 30, 30);
  painter.setClipPath(path);

  if (!m_backgroundPixmap.isNull()) {
    painter.drawPixmap(rect(), m_backgroundPixmap.scaled(
                                   size(), Qt::KeepAspectRatioByExpanding,
                                   Qt::SmoothTransformation));
  }
}

QColor MainWindow::getStatusColor(UserStatus status) {
  switch (status) {
  case UserStatus::Online:
    return wizz::ui::ThemeEngine::success();
  case UserStatus::Away:
    return wizz::ui::ThemeEngine::warning();
  case UserStatus::Busy:
    return wizz::ui::ThemeEngine::danger();
  case UserStatus::Offline:
    return wizz::ui::ThemeEngine::onSurface3();
  }
  return wizz::ui::ThemeEngine::onSurface3();
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

void MainWindow::resizeEvent(QResizeEvent *event) {
  QWidget::resizeEvent(event);
  
  // Apply Rounded Mask to remove black corners
  QPainterPath path;
  path.addRoundedRect(rect(), 30, 30);
  this->setMask(path.toFillPolygon().toPolygon());
}

void MainWindow::setupUI() {
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(0);

  // ── TitleBar ──────────────────────────────────────────────────────────────
  auto *titleBar = new TitleBar("WizzMania Pro", this);
  mainLayout->addWidget(titleBar);

  // ── Inner content area (with padding) ────────────────────────────────────
  auto *contentWidget = new QWidget(this);
  auto *contentLayout = new QVBoxLayout(contentWidget);
  contentLayout->setContentsMargins(16, 12, 16, 12);
  contentLayout->setSpacing(12);
  mainLayout->addWidget(contentWidget, 1);

  // Re-point mainLayout for all code below this point
  // (all child widgets will be added to contentLayout)
  QVBoxLayout *&innerLayout = contentLayout;
  Q_UNUSED(innerLayout) // used as alias below

  // ── Profile card ──────────────────────────────────────────────────────────
  // --- User Profile Section (Subtle Glass) ---
  QFrame *profileFrame = new QFrame(this);
  profileFrame->setObjectName("userProfileFrame");
  profileFrame->setStyleSheet(R"(
        #userProfileFrame {
            background-color: rgba(255, 255, 255, 30);
            border: 1px solid rgba(255, 255, 255, 20);
            border-radius: 28px;
        }
    )");

  profileFrame->setGraphicsEffect(
      wizz::ui::ThemeEngine::shadow(1, profileFrame));

  QHBoxLayout *profileLayout = new QHBoxLayout(profileFrame);
  profileLayout->setContentsMargins(15, 15, 15, 15);
  profileLayout->setSpacing(18);

  // Avatar 2.0
  m_avatarLabel = new QLabel(profileFrame);
  m_avatarLabel->setFixedSize(80, 80);
  m_avatarLabel->setStyleSheet("background: transparent;");

  QGraphicsDropShadowEffect *avatarGlow = new QGraphicsDropShadowEffect(this);
  avatarGlow->setBlurRadius(25);
  avatarGlow->setColor(wizz::ui::ThemeEngine::accent());
  avatarGlow->setOffset(0, 0);
  m_avatarLabel->setGraphicsEffect(avatarGlow);

  // Pulse Animation
  QPropertyAnimation *pulse = new QPropertyAnimation(avatarGlow, "blurRadius", this);
  pulse->setDuration(2000);
  pulse->setStartValue(15.0);
  pulse->setEndValue(35.0);
  pulse->setEasingCurve(QEasingCurve::InOutSine);
  pulse->setLoopCount(-1); // Infinite
  pulse->start();

  QPushButton *avatarBtn = new QPushButton(m_avatarLabel);
  avatarBtn->setFixedSize(80, 80);
  avatarBtn->setStyleSheet("background: transparent; border: none;");
  avatarBtn->setCursor(Qt::PointingHandCursor);
  connect(avatarBtn, &QPushButton::clicked, this, &MainWindow::onAvatarClicked);

  // User info
  QVBoxLayout *userInfoLayout = new QVBoxLayout();
  userInfoLayout->setSpacing(6);

  QHBoxLayout *nameRow = new QHBoxLayout();
  m_usernameLabel = new QLabel(m_username, profileFrame);
  m_usernameLabel->setStyleSheet(
      QString("font-size: 20px; font-weight: 700; color: %1; background: "
              "transparent;")
          .arg(wizz::ui::ThemeEngine::onSurface().name()));

  // SOVEREIGN BADGE — use emoji instead of PNG to avoid black background artifact
  QLabel *sovereignBadge = new QLabel("🔒", profileFrame);
  sovereignBadge->setToolTip("Sovereign Identity Verified (Ed25519)");
  sovereignBadge->setStyleSheet(
      "font-size: 14px; background: transparent; border: none; padding: 2px;");

  nameRow->addWidget(m_usernameLabel);
  nameRow->addWidget(sovereignBadge);
  nameRow->addStretch();

  // Status dropdown
  m_statusCombo = new QComboBox(profileFrame);
  m_statusCombo->addItem("🟢 Online");
  m_statusCombo->addItem("🟠 Away");
  m_statusCombo->addItem("🔴 Busy");
  m_statusCombo->addItem("⚫ Offline");
  m_statusCombo->setFixedWidth(150);
  // Use a dark glass palette for the popup so Qt doesn't override with system white
  QPalette comboPalette = m_statusCombo->palette();
  comboPalette.setColor(QPalette::Base, QColor(30, 35, 50, 220));     // dark glass
  comboPalette.setColor(QPalette::Text, Qt::white);
  comboPalette.setColor(QPalette::ButtonText, Qt::white);
  comboPalette.setColor(QPalette::WindowText, Qt::white);
  comboPalette.setColor(QPalette::Highlight, wizz::ui::ThemeEngine::accent());
  comboPalette.setColor(QPalette::HighlightedText, Qt::white);
  m_statusCombo->setPalette(comboPalette);
  m_statusCombo->setStyleSheet(R"(
      QComboBox {
          background-color: rgba(255, 255, 255, 15);
          border: 1px solid rgba(255, 255, 255, 20);
          border-radius: 8px;
          padding: 4px 10px;
          color: white;
          font-weight: 600;
      }
      QComboBox::drop-down { border: none; }
      QComboBox QAbstractItemView {
          background-color: rgba(30, 35, 50, 220);
          border: 1px solid rgba(255, 255, 255, 20);
          outline: none;
          color: white;
          selection-background-color: rgba(64, 153, 255, 180);
          selection-color: white;
          padding: 4px;
      }
  )");
  connect(m_statusCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &MainWindow::onStatusChanged);

  m_statusMessageInput = new QLineEdit(profileFrame);
  m_statusMessageInput->setPlaceholderText("Broadcast a thought...");
  m_statusMessageInput->setFixedWidth(200);
  m_statusMessageInput->setAttribute(Qt::WA_MacShowFocusRect, false);
  m_statusMessageInput->setStyleSheet(QString(R"(
      QLineEdit {
          background-color: rgba(255, 255, 255, 8);
          border: 1px solid rgba(255, 255, 255, 15);
          border-radius: 6px;
          padding: 6px 10px;
          color: %1;
          font-size: 11px;
      }
      QLineEdit:focus {
          border: 1px solid %2;
          background-color: rgba(255, 255, 255, 15);
      }
  )").arg(wizz::ui::ThemeEngine::onSurface().name()).arg(wizz::ui::ThemeEngine::accent().name()));
  connect(m_statusMessageInput, &QLineEdit::returnPressed, this,
          &MainWindow::onStatusMessageSubmitted);

  userInfoLayout->addLayout(nameRow);
  userInfoLayout->addWidget(m_statusCombo);
  userInfoLayout->addWidget(m_statusMessageInput);

  profileLayout->addWidget(m_avatarLabel);
  profileLayout->addLayout(userInfoLayout);
  profileLayout->addStretch();

  contentLayout->addWidget(profileFrame);

  // --- Friends Section Header ---
  QHBoxLayout *friendHeaderLayout = new QHBoxLayout();
  friendHeaderLayout->setContentsMargins(5, 5, 5, 0);

  QLabel *friendsLabel = new QLabel("CITIZENS", this);
  friendsLabel->setStyleSheet(
      QString(
          "font-size: 11px; font-weight: 800; color: %1; letter-spacing: 2px;")
          .arg(wizz::ui::ThemeEngine::onSurface3().name()));

  QPushButton *addFriendBtn = new QPushButton("+", this);
  addFriendBtn->setFixedSize(28, 28);
  addFriendBtn->setCursor(Qt::PointingHandCursor);
  addFriendBtn->setStyleSheet(QString(R"(
        QPushButton {
            background-color: transparent;
            border: 1px solid rgba(255, 255, 255, 30);
            border-radius: 14px;
            color: %1;
            font-weight: bold;
            font-size: 16px;
        }
        QPushButton:hover {
            background-color: rgba(0, 0, 0, 80);
            border: 1px solid %1;
        }
    )")
                                  .arg(wizz::ui::ThemeEngine::accent().name()));
  connect(addFriendBtn, &QPushButton::clicked, this,
          &MainWindow::onAddFriendClicked);

  friendHeaderLayout->addWidget(friendsLabel);
  friendHeaderLayout->addStretch();
  friendHeaderLayout->addWidget(addFriendBtn);
  contentLayout->addLayout(friendHeaderLayout);

  // --- Search Bar ---
  m_searchBar = new wizz::ui::SearchBar(this);
  connect(m_searchBar, &wizz::ui::SearchBar::textChanged, this,
          &MainWindow::onSearchTextChanged);
  contentLayout->addWidget(m_searchBar);

  // --- Contact List ---
  m_contactList = new QListWidget(this);
  m_contactList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  m_contactList->setSpacing(8);
  m_contactList->setItemDelegate(new wizz::ui::ContactDelegate(m_contactList));
  m_contactList->setContextMenuPolicy(Qt::CustomContextMenu);
  m_contactList->setStyleSheet("background: transparent !important; border: none;");
  m_contactList->viewport()->setStyleSheet("background: transparent !important;");
  m_contactList->setFrameShape(QFrame::NoFrame);
  m_contactList->setAttribute(Qt::WA_MacShowFocusRect, false);
  m_contactList->setAttribute(Qt::WA_OpaquePaintEvent, false);
  m_contactList->setAttribute(Qt::WA_NoSystemBackground, true);

  connect(m_contactList, &QListWidget::itemDoubleClicked, this,
          [this](QListWidgetItem *item) {
            onContactDoubleClicked(item->data(Qt::UserRole).toString());
          });
  connect(m_contactList, &QListWidget::customContextMenuRequested, this,
          &MainWindow::showContactContextMenu);

  contentLayout->addWidget(m_contactList, 1);

  // --- Social Arcade ---
  m_socialArcade = new wizz::ui::arcade::SocialArcade(this);
  connect(m_socialArcade, &wizz::ui::arcade::SocialArcade::categorySelected,
          this, &MainWindow::onCategorySelected);
  contentLayout->addWidget(m_socialArcade);

  // --- System Tray ---
  setupSystemTray();
}

void MainWindow::setContacts(const QList<ContactInfo> &contacts) {
  // Merge new contacts with existing ones to preserve game states and avatars
  QList<ContactInfo> mergedContacts = contacts;
  for (int i = 0; i < mergedContacts.size(); ++i) {
    for (const auto &existing : m_contacts) {
      if (mergedContacts[i].username.compare(existing.username,
                                             Qt::CaseInsensitive) == 0) {
        // If the new list doesn't have an avatar but the existing one does,
        // keep it
        if (mergedContacts[i].avatar.isNull() && !existing.avatar.isNull()) {
          mergedContacts[i].avatar = existing.avatar;
        }
        // Preserve game states
        mergedContacts[i].isPlayingGame = existing.isPlayingGame;
        mergedContacts[i].currentGameName = existing.currentGameName;
        mergedContacts[i].currentGameScore = existing.currentGameScore;
        break;
      }
    }
  }

  m_contacts = mergedContacts;
  populateContactList();

  // Initialize fetching avatars for all contacts
  for (const auto &contact : m_contacts) {
    if (contact.avatar.isNull()) {
      AvatarManager::instance().getAvatar(contact.username, 36);
    }
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

  // Build a map of username -> index in m_contacts so we can safely store
  // pointers
  QMap<QString, int> contactIndices;
  for (int i = 0; i < m_contacts.size(); ++i) {
    contactIndices[m_contacts[i].username] = i;
  }

  for (const ContactInfo &contact : sorted) {
    QListWidgetItem *item = new QListWidgetItem(m_contactList);
    item->setData(Qt::UserRole, contact.username);

    // Store pointer to the ACTUAL ContactInfo struct in m_contacts, not the
    // sorted copy
    int realIndex = contactIndices[contact.username];
    void *ptr =
        static_cast<void *>(const_cast<ContactInfo *>(&m_contacts[realIndex]));
    item->setData(Qt::UserRole + 1, QVariant::fromValue(ptr));
  }
}

void MainWindow::updateContactStatus(const QString &username, UserStatus status,
                                     const QString &statusMessage) {
  std::cout << "[MainWindow] updateContactStatus signal for: "
            << username.toStdString() << " Status: " << static_cast<int>(status)
            << std::endl;
  bool found = false;
  for (ContactInfo &contact : m_contacts) {
    if (contact.username.compare(username, Qt::CaseInsensitive) == 0) {
      contact.status = status;
      if (status == UserStatus::Offline) {
        contact.statusMessage = "";
      } else if (!statusMessage.isEmpty()) {
        contact.statusMessage = statusMessage;
      }
      found = true;
      break;
    }
  }
  if (!found) {
    std::cout << "[MainWindow] WARNING: Contact not found in list for update: "
              << username.toStdString() << std::endl;
  }
  populateContactList();
}

void MainWindow::updateContactGameStatus(const QString &username,
                                         const QString &gameName,
                                         uint32_t score) {
  for (ContactInfo &contact : m_contacts) {
    if (contact.username.compare(username, Qt::CaseInsensitive) == 0) {
      if (gameName.isEmpty()) {
        contact.isPlayingGame = false;
        contact.currentGameName = "";
        contact.currentGameScore = 0;
      } else {
        contact.isPlayingGame = true;
        contact.currentGameName = gameName;
        contact.currentGameScore = score;
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
    if (contact.username.compare(username, Qt::CaseInsensitive) == 0) {
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
  QString statusMessage =
      m_statusMessageInput ? m_statusMessageInput->text() : "";
  emit statusChanged(m_currentStatus, statusMessage);

  // Inform NetworkManager
  NetworkManager::instance().sendStatusChange(static_cast<int>(m_currentStatus),
                                              statusMessage);
  std::cout << "[MainWindow] Direct Status change to index: " << index
            << std::endl;
}

void MainWindow::onStatusMessageSubmitted() {
  QString msg = m_statusMessageInput->text();
  NetworkManager::instance().sendUpdateStatus(msg);
  m_statusMessageInput->clearFocus();
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
  QString lowerName = username.toLower();
  if (m_openChats.contains(lowerName)) {
    ChatWindow *w = m_openChats[lowerName];
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
  connect(w, &ChatWindow::sendMessage, this,
          [username, w](const QString &text) {
            NetworkManager::instance().sendEncryptedMessage(username, text);
            // Since we now have rich messages, let's just make sure local echo
            // works for plain text. Actually, local echo is done inside
            // ChatWindow::onSendClicked, so we just send here.
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
  m_openChats.insert(lowerName, w);
}

void MainWindow::onChatWindowClosed(const QString &partnerName) {
  m_openChats.remove(partnerName.toLower());
}

ChatWindow *MainWindow::openChatWindow(const QString &username) {
  QString lowerName = username.toLower();
  if (!m_openChats.contains(lowerName)) {
    onContactDoubleClicked(username);
  }
  return m_openChats.value(lowerName, nullptr);
}

void MainWindow::onSearchTextChanged(const QString &text) {
  QString lowerText = text.toLower();
  for (int i = 0; i < m_contactList->count(); ++i) {
    QListWidgetItem *item = m_contactList->item(i);
    QString username = item->data(Qt::UserRole).toString().toLower();
    item->setHidden(!username.contains(lowerText));
  }
}

void MainWindow::showContactContextMenu(const QPoint &pos) {
  QListWidgetItem *item = m_contactList->itemAt(pos);
  if (!item)
    return;

  QString username = item->data(Qt::UserRole).toString();

  // Find contact info to check status
  const ContactInfo *contact = nullptr;
  for (const auto &c : m_contacts) {
    if (c.username == username) {
      contact = &c;
      break;
    }
  }
  if (!contact)
    return;

  QMenu menu(this);
  QPalette menuPalette = menu.palette();
  menuPalette.setColor(QPalette::Base, QColor(30, 35, 50, 220));
  menuPalette.setColor(QPalette::Text, Qt::white);
  menuPalette.setColor(QPalette::ButtonText, Qt::white);
  menuPalette.setColor(QPalette::WindowText, Qt::white);
  menuPalette.setColor(QPalette::Highlight, wizz::ui::ThemeEngine::accent());
  menuPalette.setColor(QPalette::HighlightedText, Qt::white);
  menu.setPalette(menuPalette);
  
  menu.setStyleSheet(QString(R"(
    QMenu {
      background-color: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 rgba(30, 35, 50, 240), stop:1 rgba(15, 20, 30, 240));
      border: 1px solid rgba(255, 255, 255, 20);
      border-radius: 12px;
      padding: 6px;
    }
    QMenu::item {
      padding: 8px 24px 8px 12px;
      border-radius: 6px;
      color: rgba(255, 255, 255, 230);
      font-size: 14px;
      font-weight: 500;
      margin: 2px 4px;
    }
    QMenu::item:selected {
      background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 rgba(64, 153, 255, 200), stop:1 rgba(64, 153, 255, 80));
      color: white;
      font-weight: 600;
    }
    QMenu::separator {
      height: 1px;
      background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 rgba(255,255,255,0), stop:0.5 rgba(255,255,255,30), stop:1 rgba(255,255,255,0));
      margin: 6px 12px;
    }
  )"));

  QAction *openChatAction = menu.addAction("💬 Open Chat");
  QAction *nudgeAction = nullptr;
  QMenu *gameMenu = nullptr;
  QAction *ticTacToeAction = nullptr;

  if (contact->status == UserStatus::Online) {
    nudgeAction = menu.addAction("⚡ Send Nudge");

    // Sub-menu for games
    gameMenu = menu.addMenu("🎮 Invite to Game...");
    
    // Set matching palette for submenu
    gameMenu->setPalette(menuPalette);
    gameMenu->setStyleSheet(menu.styleSheet());

    ticTacToeAction = gameMenu->addAction("❎🅾️ TicTacToe");

    connect(ticTacToeAction, &QAction::triggered, this, [this, username]() {
      NetworkManager::instance().sendGameInvite(username, "TicTacToe");
    });
  }

  menu.addSeparator();
  QAction *removeAction = menu.addAction("🗑️ Remove Contact");

  QAction *selectedAction = menu.exec(m_contactList->mapToGlobal(pos));
  if (selectedAction == openChatAction) {
    onContactDoubleClicked(username);
  } else if (nudgeAction && selectedAction == nudgeAction) {
    wizz::Packet pkt(wizz::PacketType::Nudge);
    pkt.writeString(username.toStdString());
    NetworkManager::instance().sendPacket(pkt);
  } else if (selectedAction == removeAction) {
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Remove Contact",
        "Are you sure you want to remove " + username + "?",
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
      wizz::Packet pkt(wizz::PacketType::RemoveContact);
      pkt.writeString(username.toStdString());
      NetworkManager::instance().sendPacket(pkt);
    }
  }
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

// --- Social Arcade Implementation ---

void MainWindow::onCategorySelected(
    const wizz::ui::arcade::ArcadeCategory &category) {
  
  if (category.id == "games") {
    auto* popup = new wizz::ui::arcade::GameSelectionPopup(category, this);
    connect(popup, &wizz::ui::arcade::GameSelectionPopup::gameSelected, this, [this](const QString& gameId) {
        GameLauncher::launchGame(gameId, m_username);
        NetworkManager::instance().sendRichPresence(1, gameId, "Playing " + gameId);
    });
    popup->show();
    return;
  }

  // Gather online contacts for the share dialog
  QList<QString> onlineUsers;
  for (const auto &contact : m_contacts) {
    if (contact.status == UserStatus::Online &&
        contact.username != m_username) {
      onlineUsers.append(contact.username);
    }
  }

  auto *popup =
      new wizz::ui::arcade::CategoryPopup(category, onlineUsers, this);

  connect(popup,
          &wizz::ui::arcade::CategoryPopup::broadcastRichPresenceRequested,
          this,
          [](const wizz::ui::arcade::ArcadeCategory &cat, const QString &text) {
            // Assuming type 2 for generic arcade activities
            NetworkManager::instance().sendRichPresence(2, cat.name, text);
          });

  connect(popup, &wizz::ui::arcade::CategoryPopup::shareToChatRequested, this,
          [](const wizz::ui::arcade::ArcadeCategory &cat, const QString &text,
             const QString &targetUser) {
            // Send a rich card message
            QString richMsg = QString("{\"type\":\"arcade_share\",\"category\":"
                                      "\"%1\",\"text\":\"%2\"}")
                                  .arg(cat.name)
                                  .arg(text);
            NetworkManager::instance().sendEncryptedMessage(targetUser,
                                                            richMsg);
          });

  // Center popup over main window
  popup->move(geometry().center() - popup->rect().center());
  popup->exec();
}

void MainWindow::onLocalGameStatusChanged(bool isPlaying,
                                          const QString &gameName,
                                          uint32_t score) {
  if (isPlaying) {
    NetworkManager::instance().sendRichPresence(
        1, gameName, QString("Score: %1").arg(score));
  } else {
    NetworkManager::instance().sendRichPresence(0, "", "");
  }

  if (m_statusMessageInput) {
    if (isPlaying) {
      m_statusMessageInput->setText(
          QString("🎮 Playing %1 (Score: %2)").arg(gameName).arg(score));
      m_statusMessageInput->setReadOnly(true);
    } else {
      m_statusMessageInput->setText("");
      m_statusMessageInput->setReadOnly(false);
      m_statusMessageInput->setPlaceholderText("Broadcast a thought...");
    }
  }
}

void MainWindow::onLocalMoveMade(const QString &roomId, uint8_t cellIndex) {
  NetworkManager::instance().sendGameMove(roomId, cellIndex);
}

void MainWindow::onTicTacToeFinished() {
  NetworkManager::instance().sendGameStatus("", 0);
  if (m_statusMessageInput) {
    m_statusMessageInput->setText("");
    m_statusMessageInput->setReadOnly(false);
    m_statusMessageInput->setPlaceholderText("Share a quick thought...");
  }
  populateContactList();
}

// ── System Tray & Badges ──────────────────────────────────────────────────

void MainWindow::setupSystemTray() {
  if (!QSystemTrayIcon::isSystemTrayAvailable())
    return;

  m_trayIcon = new QSystemTrayIcon(this);
  m_trayIcon->setIcon(QIcon(":/assets/butterfly.png"));
  m_trayIcon->setToolTip("WizzMania Pro");

  connect(m_trayIcon, &QSystemTrayIcon::activated, this,
          [this](QSystemTrayIcon::ActivationReason reason) {
            if (reason == QSystemTrayIcon::Trigger ||
                reason == QSystemTrayIcon::DoubleClick) {
              this->showNormal();
              this->activateWindow();
            }
          });

  m_trayIcon->show();
}

void MainWindow::updateGlobalUnreadCount() {
  int totalUnread = 0;
  for (const auto &contact : m_contacts) {
    totalUnread += contact.unreadCount;
  }

  if (m_trayIcon) {
    if (totalUnread > 0) {
      m_trayIcon->setToolTip(
          QString("WizzMania Pro (%1 Unread)").arg(totalUnread));
      // Optionally show a macOS notification
      // m_trayIcon->showMessage("New Messages", QString("You have %1 unread
      // messages").arg(totalUnread), QSystemTrayIcon::Information, 3000);
    } else {
      m_trayIcon->setToolTip("WizzMania Pro");
    }
  }

  // Also update the TitleBar if we want
  // auto* titleBar = findChild<TitleBar*>();
  // if (titleBar) {
  //   titleBar->setTitle(totalUnread > 0 ? QString("WizzMania Pro
  //   (%1)").arg(totalUnread) : "WizzMania Pro");
  // }
}

// ── Frameless Resize Implementation ─────────────────────────────────────────

MainWindow::ResizeEdge MainWindow::edgeAtPoint(const QPoint &p) const {
  const int r = RESIZE_MARGIN;
  const int w = width();
  const int h = height();

  bool left = p.x() <= r;
  bool right = p.x() >= w - r;
  bool top = p.y() <= r;
  bool bottom = p.y() >= h - r;

  if (top && left)
    return ResizeEdge::TopLeft;
  if (top && right)
    return ResizeEdge::TopRight;
  if (bottom && left)
    return ResizeEdge::BottomLeft;
  if (bottom && right)
    return ResizeEdge::BottomRight;
  if (left)
    return ResizeEdge::Left;
  if (right)
    return ResizeEdge::Right;
  if (top)
    return ResizeEdge::Top;
  if (bottom)
    return ResizeEdge::Bottom;

  return ResizeEdge::None;
}

void MainWindow::applyCursorForEdge(ResizeEdge edge) {
  switch (edge) {
  case ResizeEdge::Left:
  case ResizeEdge::Right:
    setCursor(Qt::SizeHorCursor);
    break;
  case ResizeEdge::Top:
  case ResizeEdge::Bottom:
    setCursor(Qt::SizeVerCursor);
    break;
  case ResizeEdge::TopLeft:
  case ResizeEdge::BottomRight:
    setCursor(Qt::SizeFDiagCursor);
    break;
  case ResizeEdge::TopRight:
  case ResizeEdge::BottomLeft:
    setCursor(Qt::SizeBDiagCursor);
    break;
  case ResizeEdge::None:
    unsetCursor();
    break;
  }
}

void MainWindow::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    m_resizeEdge = edgeAtPoint(event->pos());
    if (m_resizeEdge != ResizeEdge::None) {
      m_resizing = true;
      m_resizeStart = event->globalPosition().toPoint();
      m_resizeStartGeom = geometry();
      event->accept();
      return;
    }
  }
  QWidget::mousePressEvent(event);
}

void MainWindow::mouseMoveEvent(QMouseEvent *event) {
  if (m_resizing) {
    QPoint diff = event->globalPosition().toPoint() - m_resizeStart;
    QRect newGeom = m_resizeStartGeom;

    switch (m_resizeEdge) {
    case ResizeEdge::Left:
      newGeom.setLeft(
          qMin(newGeom.right() - minimumWidth(), newGeom.left() + diff.x()));
      break;
    case ResizeEdge::Right:
      newGeom.setRight(
          qMax(newGeom.left() + minimumWidth(), newGeom.right() + diff.x()));
      break;
    case ResizeEdge::Top:
      newGeom.setTop(
          qMin(newGeom.bottom() - minimumHeight(), newGeom.top() + diff.y()));
      break;
    case ResizeEdge::Bottom:
      newGeom.setBottom(
          qMax(newGeom.top() + minimumHeight(), newGeom.bottom() + diff.y()));
      break;
    case ResizeEdge::TopLeft:
      newGeom.setLeft(
          qMin(newGeom.right() - minimumWidth(), newGeom.left() + diff.x()));
      newGeom.setTop(
          qMin(newGeom.bottom() - minimumHeight(), newGeom.top() + diff.y()));
      break;
    case ResizeEdge::TopRight:
      newGeom.setRight(
          qMax(newGeom.left() + minimumWidth(), newGeom.right() + diff.x()));
      newGeom.setTop(
          qMin(newGeom.bottom() - minimumHeight(), newGeom.top() + diff.y()));
      break;
    case ResizeEdge::BottomLeft:
      newGeom.setLeft(
          qMin(newGeom.right() - minimumWidth(), newGeom.left() + diff.x()));
      newGeom.setBottom(
          qMax(newGeom.top() + minimumHeight(), newGeom.bottom() + diff.y()));
      break;
    case ResizeEdge::BottomRight:
      newGeom.setRight(
          qMax(newGeom.left() + minimumWidth(), newGeom.right() + diff.x()));
      newGeom.setBottom(
          qMax(newGeom.top() + minimumHeight(), newGeom.bottom() + diff.y()));
      break;
    case ResizeEdge::None:
      break;
    }
    setGeometry(newGeom);
    event->accept();
    return;
  }

  // Not resizing, update cursor on hover
  applyCursorForEdge(edgeAtPoint(event->pos()));
  QWidget::mouseMoveEvent(event);
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event) {
  if (m_resizing && event->button() == Qt::LeftButton) {
    m_resizing = false;
    m_resizeEdge = ResizeEdge::None;
    applyCursorForEdge(edgeAtPoint(event->pos()));
    event->accept();
    return;
  }
  QWidget::mouseReleaseEvent(event);
}
