#include "LoginDialog.h"
#include "../common/Packet.h"
#include "NetworkManager.h"
#include <QGraphicsDropShadowEffect>
#include <QGraphicsOpacityEffect>
#include <QImage>
#include <QMessageBox>
#include <QPainter>

LoginDialog::LoginDialog(QWidget *parent) : QDialog(parent) {
  setupUI();
  applyStyles();

  connect(&NetworkManager::instance(), &NetworkManager::connected, this,
          [this]() {
            m_statusLabel->setText("Verifying...");
            m_statusLabel->setStyleSheet(
                "color: #00f2fe; font-weight: bold; background: transparent;");

            wizz::Packet loginPkt(wizz::PacketType::Login);
            loginPkt.writeString(m_usernameInput->text().toStdString());
            loginPkt.writeString(m_passwordInput->text().toStdString());
            NetworkManager::instance().sendPacket(loginPkt);
          });

  connect(&NetworkManager::instance(), &NetworkManager::packetReceived, this,
          [this](const wizz::Packet &constPkt) {
            wizz::Packet pkt = constPkt;
            if (pkt.type() == wizz::PacketType::LoginSuccess) {
              onLoginSuccess();
            } else if (pkt.type() == wizz::PacketType::LoginFailed) {
              std::string reason = pkt.readString();
              onLoginFailed(QString::fromStdString(reason));
            } else if (pkt.type() == wizz::PacketType::Error) {
              std::string err = pkt.readString();
              onLoginFailed("Error: " + QString::fromStdString(err));
            }
          });

  connect(&NetworkManager::instance(), &NetworkManager::errorOccurred, this,
          &LoginDialog::onConnectionError);
}

QPixmap LoginDialog::processTransparentImage(const QString &path, int size) {
  QPixmap pix(path);
  if (pix.isNull())
    return QPixmap();
  return pix.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

void LoginDialog::setupUI() {
  setWindowTitle("Wizz Mania");
  setFixedSize(1024, 768);

  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setAlignment(Qt::AlignCenter);

  // --- Outer Glass Card ---
  QFrame *glassCard = new QFrame(this);
  glassCard->setObjectName("glassCard");
  glassCard->setFixedSize(500, 520);

  QGraphicsDropShadowEffect *cardShadow = new QGraphicsDropShadowEffect(this);
  cardShadow->setBlurRadius(60);
  cardShadow->setColor(QColor(0, 60, 120, 100));
  cardShadow->setOffset(0, 15);
  glassCard->setGraphicsEffect(cardShadow);

  QVBoxLayout *cardLayout = new QVBoxLayout(glassCard);
  cardLayout->setContentsMargins(40, 30, 40, 35);
  cardLayout->setSpacing(12);

  // --- Header with Title and Butterfly ---
  QHBoxLayout *headerLayout = new QHBoxLayout();

  QLabel *titleLabel = new QLabel("Wizz Mania", glassCard);
  titleLabel->setObjectName("titleLabel");

  QLabel *butterflyIcon = new QLabel(glassCard);
  butterflyIcon->setPixmap(
      processTransparentImage(":/assets/butterfly.png", 70));
  butterflyIcon->setFixedSize(70, 70);
  butterflyIcon->setStyleSheet("background: transparent;");

  QGraphicsDropShadowEffect *flyShadow = new QGraphicsDropShadowEffect(this);
  flyShadow->setBlurRadius(12);
  flyShadow->setColor(QColor(0, 0, 0, 50));
  flyShadow->setOffset(2, 4);
  butterflyIcon->setGraphicsEffect(flyShadow);

  headerLayout->addWidget(titleLabel);
  headerLayout->addStretch();
  headerLayout->addWidget(butterflyIcon);
  cardLayout->addLayout(headerLayout);

  // Tagline
  QLabel *taglineLabel =
      new QLabel("It's not 2003, but you can still log in.", glassCard);
  taglineLabel->setObjectName("taglineLabel");
  cardLayout->addWidget(taglineLabel);
  cardLayout->addSpacing(20);

  // --- Inner Glass Frame (contains inputs + buttons) ---
  QFrame *innerFrame = new QFrame(glassCard);
  innerFrame->setObjectName("innerFrame");
  innerFrame->setFixedSize(420, 280);

  QGraphicsDropShadowEffect *innerShadow = new QGraphicsDropShadowEffect(this);
  innerShadow->setBlurRadius(20);
  innerShadow->setColor(QColor(150, 200, 255, 80));
  innerShadow->setOffset(0, 0);
  innerFrame->setGraphicsEffect(innerShadow);

  QVBoxLayout *innerLayout = new QVBoxLayout(innerFrame);
  innerLayout->setContentsMargins(25, 25, 25, 25);
  innerLayout->setSpacing(15);

  // Username Input with Icon
  QHBoxLayout *userRow = new QHBoxLayout();
  QLabel *userIcon = new QLabel(innerFrame);
  userIcon->setPixmap(processTransparentImage(":/assets/icon_user.png", 24));
  userIcon->setFixedSize(24, 24);
  userIcon->setStyleSheet("background: transparent;");

  m_usernameInput = new QLineEdit(innerFrame);
  m_usernameInput->setPlaceholderText("Username");
  m_usernameInput->setObjectName("glassInput");

  userRow->addWidget(userIcon);
  userRow->addWidget(m_usernameInput);
  innerLayout->addLayout(userRow);

  // Password Input with Icon
  QHBoxLayout *passRow = new QHBoxLayout();
  QLabel *lockIcon = new QLabel(innerFrame);
  lockIcon->setPixmap(processTransparentImage(":/assets/icon_lock.png", 24));
  lockIcon->setFixedSize(24, 24);
  lockIcon->setStyleSheet("background: transparent;");

  m_passwordInput = new QLineEdit(innerFrame);
  m_passwordInput->setPlaceholderText("Password");
  m_passwordInput->setEchoMode(QLineEdit::Password);
  m_passwordInput->setObjectName("glassInput");

  passRow->addWidget(lockIcon);
  passRow->addWidget(m_passwordInput);
  innerLayout->addLayout(passRow);

  innerLayout->addSpacing(10);

  // Sign In Button (Glass style)
  m_loginButton = new QPushButton("Sign In >", innerFrame);
  m_loginButton->setCursor(Qt::PointingHandCursor);
  m_loginButton->setFixedHeight(48);
  m_loginButton->setObjectName("glassSignInBtn");

  QGraphicsDropShadowEffect *btnShadow = new QGraphicsDropShadowEffect(this);
  btnShadow->setBlurRadius(20);
  btnShadow->setColor(QColor(100, 180, 255, 100));
  btnShadow->setOffset(0, 4);
  m_loginButton->setGraphicsEffect(btnShadow);

  innerLayout->addWidget(m_loginButton);

  // Secondary Buttons Row
  QHBoxLayout *secondaryRow = new QHBoxLayout();

  QPushButton *createAcc = new QPushButton("Create account", innerFrame);
  createAcc->setCursor(Qt::PointingHandCursor);
  createAcc->setObjectName("secondaryBtn");

  QLabel *separator = new QLabel("|", innerFrame);
  separator->setStyleSheet(
      "color: rgba(100, 120, 140, 150); background: transparent;");

  QPushButton *offlineMode = new QPushButton("Offline mode", innerFrame);
  offlineMode->setCursor(Qt::PointingHandCursor);
  offlineMode->setObjectName("secondaryBtn");

  secondaryRow->addStretch();
  secondaryRow->addWidget(createAcc);
  secondaryRow->addWidget(separator);
  secondaryRow->addWidget(offlineMode);
  secondaryRow->addStretch();

  innerLayout->addLayout(secondaryRow);

  cardLayout->addWidget(innerFrame, 0, Qt::AlignCenter);

  // Status Label
  m_statusLabel = new QLabel("", glassCard);
  m_statusLabel->setAlignment(Qt::AlignCenter);
  m_statusLabel->setStyleSheet("background: transparent;");
  cardLayout->addWidget(m_statusLabel);

  mainLayout->addWidget(glassCard);

  // --- Decorative Mascots ---
  QLabel *alienGreen = new QLabel(this);
  alienGreen->setPixmap(
      processTransparentImage(":/assets/alien_green.png", 140));
  alienGreen->setFixedSize(140, 140);
  alienGreen->move(140, 540);
  alienGreen->setStyleSheet("background: transparent;");

  QGraphicsDropShadowEffect *greenShadow = new QGraphicsDropShadowEffect(this);
  greenShadow->setBlurRadius(25);
  greenShadow->setColor(QColor(0, 0, 0, 80));
  greenShadow->setOffset(0, 12);
  alienGreen->setGraphicsEffect(greenShadow);

  QLabel *ufoGold = new QLabel(this);
  ufoGold->setPixmap(processTransparentImage(":/assets/alien_gold.png", 150));
  ufoGold->setFixedSize(150, 150);
  ufoGold->move(730, 530);
  ufoGold->setStyleSheet("background: transparent;");

  QGraphicsDropShadowEffect *goldShadow = new QGraphicsDropShadowEffect(this);
  goldShadow->setBlurRadius(25);
  goldShadow->setColor(QColor(0, 0, 0, 80));
  goldShadow->setOffset(0, 12);
  ufoGold->setGraphicsEffect(goldShadow);

  connect(m_loginButton, &QPushButton::clicked, this,
          &LoginDialog::onLoginClicked);
}

void LoginDialog::applyStyles() {
  this->setStyleSheet(R"(
        LoginDialog {
            background-image: url(:/assets/login_bg.png);
            background-position: center;
            background-repeat: no-repeat;
        }

        /* Outer Glass Card */
        #glassCard {
            background-color: rgba(255, 255, 255, 35);
            border: 2px solid rgba(255, 255, 255, 120);
            border-top: 2px solid rgba(255, 255, 255, 180);
            border-bottom: 2px solid rgba(255, 255, 255, 80);
            border-radius: 35px;
        }

        /* Inner Glass Frame with glowing edges */
        #innerFrame {
            background-color: rgba(255, 255, 255, 25);
            border: 2px solid rgba(200, 230, 255, 150);
            border-radius: 20px;
        }

        #titleLabel {
            font-family: 'Segoe UI', 'SF Pro Display', sans-serif;
            font-size: 34px;
            font-weight: 700;
            color: #1a2530;
            background: transparent;
        }
        
        #taglineLabel {
            font-family: 'Segoe UI', 'SF Pro Display', sans-serif;
            font-size: 14px;
            color: #4a5568;
            background: transparent;
        }

        /* Glass Inputs - Transparent rounded */
        #glassInput {
            background-color: rgba(255, 255, 255, 50);
            border: 1px solid rgba(255, 255, 255, 150);
            border-radius: 20px;
            padding: 12px 18px;
            font-size: 15px;
            color: #2d3748;
        }
        #glassInput:focus {
            background-color: rgba(255, 255, 255, 100);
            border: 2px solid rgba(100, 180, 255, 180);
        }

        /* Glass Sign In Button */
        #glassSignInBtn {
            background-color: rgba(80, 180, 255, 120);
            border: 2px solid rgba(150, 220, 255, 200);
            border-radius: 24px;
            font-size: 17px;
            font-weight: bold;
            color: white;
        }
        #glassSignInBtn:hover {
            background-color: rgba(100, 200, 255, 160);
            border: 2px solid rgba(180, 240, 255, 220);
        }
        #glassSignInBtn:pressed {
            background-color: rgba(60, 160, 240, 180);
        }

        /* Secondary Text Buttons */
        #secondaryBtn {
            background-color: transparent;
            border: none;
            font-size: 13px;
            font-weight: 500;
            color: rgba(60, 80, 100, 180);
        }
        #secondaryBtn:hover {
            color: rgb(0, 120, 200);
        }
    )");
}

void LoginDialog::onLoginClicked() {
  if (m_usernameInput->text().isEmpty()) {
    m_statusLabel->setText("Please enter a username");
    m_statusLabel->setStyleSheet("color: #e74c3c; background: transparent;");
    return;
  }

  m_statusLabel->setText("Connecting...");
  m_statusLabel->setStyleSheet(
      "color: #00a8ff; background: transparent; font-weight: bold;");
  m_loginButton->setEnabled(false);

  NetworkManager::instance().connectToHost(m_defaultHost, m_defaultPort);
}

void LoginDialog::onLoginSuccess() { accept(); }

void LoginDialog::onLoginFailed(const QString &reason) {
  m_statusLabel->setText(reason);
  m_statusLabel->setStyleSheet(
      "color: #e74c3c; font-weight: bold; background: transparent;");
  m_loginButton->setEnabled(true);
}

void LoginDialog::onConnectionError(const QString &error) {
  m_statusLabel->setText("Error: " + error);
  m_statusLabel->setStyleSheet("color: #e74c3c; background: transparent;");
  m_loginButton->setEnabled(true);
}
