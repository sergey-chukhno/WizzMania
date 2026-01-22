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

// Load PNG with transparency preserved (no white removal needed for proper
// PNGs)
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

  // --- Glass Card ---
  QFrame *glassCard = new QFrame(this);
  glassCard->setObjectName("glassCard");
  glassCard->setFixedSize(480, 580);

  // Card shadow for depth
  QGraphicsDropShadowEffect *cardShadow = new QGraphicsDropShadowEffect(this);
  cardShadow->setBlurRadius(60);
  cardShadow->setColor(QColor(0, 60, 120, 100));
  cardShadow->setOffset(0, 15);
  glassCard->setGraphicsEffect(cardShadow);

  QVBoxLayout *cardLayout = new QVBoxLayout(glassCard);
  cardLayout->setContentsMargins(45, 35, 45, 45);
  cardLayout->setSpacing(18);

  // --- Header with Butterfly ---
  QHBoxLayout *headerTop = new QHBoxLayout();
  headerTop->addStretch();

  QLabel *butterflyIcon = new QLabel(glassCard);
  butterflyIcon->setPixmap(
      processTransparentImage(":/assets/butterfly.png", 80));
  butterflyIcon->setFixedSize(80, 80);
  butterflyIcon->setStyleSheet("background: transparent;");

  QGraphicsDropShadowEffect *flyShadow = new QGraphicsDropShadowEffect(this);
  flyShadow->setBlurRadius(15);
  flyShadow->setColor(QColor(0, 0, 0, 60));
  flyShadow->setOffset(3, 5);
  butterflyIcon->setGraphicsEffect(flyShadow);

  headerTop->addWidget(butterflyIcon);
  cardLayout->addLayout(headerTop);

  // Title
  QLabel *titleLabel = new QLabel("Wizz Mania", glassCard);
  titleLabel->setAlignment(Qt::AlignCenter);
  titleLabel->setObjectName("titleLabel");

  QLabel *taglineLabel =
      new QLabel("It's not 2003, but you can still log in.", glassCard);
  taglineLabel->setAlignment(Qt::AlignCenter);
  taglineLabel->setObjectName("taglineLabel");

  cardLayout->addWidget(titleLabel);
  cardLayout->addWidget(taglineLabel);
  cardLayout->addSpacing(20);

  // --- Inputs ---
  m_hostInput = new QLineEdit("127.0.0.1", glassCard);
  m_hostInput->setPlaceholderText("Server IP");
  m_hostInput->setObjectName("glassInput");

  m_portInput = new QLineEdit("8080", glassCard);
  m_portInput->setPlaceholderText("Port");
  m_portInput->setObjectName("glassInput");

  m_usernameInput = new QLineEdit(glassCard);
  m_usernameInput->setPlaceholderText("Username");
  m_usernameInput->setObjectName("glassInput");

  m_passwordInput = new QLineEdit(glassCard);
  m_passwordInput->setPlaceholderText("Password");
  m_passwordInput->setEchoMode(QLineEdit::Password);
  m_passwordInput->setObjectName("glassInput");

  cardLayout->addWidget(m_hostInput);
  cardLayout->addWidget(m_portInput);
  cardLayout->addWidget(m_usernameInput);
  cardLayout->addWidget(m_passwordInput);
  cardLayout->addSpacing(18);

  // --- Primary Button ---
  m_loginButton = new QPushButton("Sign In >", glassCard);
  m_loginButton->setCursor(Qt::PointingHandCursor);
  m_loginButton->setFixedHeight(52);
  m_loginButton->setObjectName("primaryBtn");

  QGraphicsDropShadowEffect *btnShadow = new QGraphicsDropShadowEffect(this);
  btnShadow->setBlurRadius(25);
  btnShadow->setColor(QColor(0, 150, 255, 120));
  btnShadow->setOffset(0, 6);
  m_loginButton->setGraphicsEffect(btnShadow);

  cardLayout->addWidget(m_loginButton);

  // --- Secondary Buttons ---
  QHBoxLayout *footerLayout = new QHBoxLayout();

  QPushButton *createAcc = new QPushButton("Create account", glassCard);
  createAcc->setCursor(Qt::PointingHandCursor);
  createAcc->setObjectName("secondaryBtn");

  QPushButton *offlineMode = new QPushButton("Offline mode", glassCard);
  offlineMode->setCursor(Qt::PointingHandCursor);
  offlineMode->setObjectName("secondaryBtn");

  footerLayout->addWidget(createAcc);
  footerLayout->addStretch();
  footerLayout->addWidget(offlineMode);

  cardLayout->addLayout(footerLayout);

  m_statusLabel = new QLabel("", glassCard);
  m_statusLabel->setAlignment(Qt::AlignCenter);
  cardLayout->addWidget(m_statusLabel);

  mainLayout->addWidget(glassCard);

  // --- Decorative Elements ---
  QLabel *alienGreen = new QLabel(this);
  alienGreen->setPixmap(
      processTransparentImage(":/assets/alien_green.png", 130));
  alienGreen->setFixedSize(130, 130);
  alienGreen->move(170, 560);
  alienGreen->setStyleSheet("background: transparent;");

  QGraphicsDropShadowEffect *greenShadow = new QGraphicsDropShadowEffect(this);
  greenShadow->setBlurRadius(25);
  greenShadow->setColor(QColor(0, 0, 0, 80));
  greenShadow->setOffset(0, 12);
  alienGreen->setGraphicsEffect(greenShadow);

  QLabel *ufoGold = new QLabel(this);
  ufoGold->setPixmap(processTransparentImage(":/assets/alien_gold.png", 140));
  ufoGold->setFixedSize(140, 140);
  ufoGold->move(720, 550);
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

        /* Glass Card */
        #glassCard {
            background-color: rgba(255, 255, 255, 45);
            border: 2px solid rgba(255, 255, 255, 150);
            border-top: 2px solid rgba(255, 255, 255, 200);
            border-bottom: 2px solid rgba(255, 255, 255, 100);
            border-radius: 32px;
        }

        #titleLabel {
            font-family: 'Segoe UI', 'SF Pro Display', sans-serif;
            font-size: 36px;
            font-weight: 700;
            color: #1a2530; 
        }
        
        #taglineLabel {
            font-family: 'Segoe UI', 'SF Pro Display', sans-serif;
            font-size: 14px;
            color: #4a5568;
        }

        /* Glass Inputs - Proper height with padding */
        #glassInput {
            background-color: rgb(245, 250, 255);
            border: 1px solid rgba(255, 255, 255, 200);
            border-top: 1px solid rgba(180, 200, 220, 100);
            border-bottom: 2px solid rgba(255, 255, 255, 255);
            border-radius: 22px;
            padding: 16px 24px;
            font-size: 16px;
            color: #2d3748;
        }
        #glassInput:focus {
            background-color: rgb(255, 255, 255);
            border: 2px solid rgb(80, 180, 255);
        }

        /* Primary Gel Button - Smooth Aqua like Maquette */
        #primaryBtn {
            background: qlineargradient(
                x1:0, y1:0, x2:0, y2:1,
                stop:0.0 rgb(80, 200, 255),
                stop:0.5 rgb(40, 180, 250),
                stop:1.0 rgb(20, 150, 230)
            );
            color: white;
            border: 2px solid rgba(120, 220, 255, 200);
            border-radius: 26px;
            font-size: 18px;
            font-weight: bold;
            padding: 14px 30px;
        }
        #primaryBtn:hover {
            background: qlineargradient(
                x1:0, y1:0, x2:0, y2:1,
                stop:0.0 rgb(100, 220, 255),
                stop:0.5 rgb(60, 200, 255),
                stop:1.0 rgb(40, 170, 245)
            );
        }
        #primaryBtn:pressed {
            background: rgb(30, 160, 230);
        }

        /* Secondary Glass Buttons */
        #secondaryBtn {
            background-color: rgba(255, 255, 255, 70);
            border: 1px solid rgba(255, 255, 255, 160);
            border-radius: 14px;
            padding: 8px 16px;
            font-size: 13px;
            font-weight: 600;
            color: #2d3748;
        }
        #secondaryBtn:hover {
            background-color: rgba(255, 255, 255, 120);
            border: 1px solid rgba(255, 255, 255, 220);
            color: rgb(0, 120, 200);
        }
    )");
}

void LoginDialog::onLoginClicked() {
  QString host = m_hostInput->text();
  quint16 port = m_portInput->text().toUShort();

  if (host.isEmpty() || port == 0) {
    m_statusLabel->setText("Invalid Host/Port");
    m_statusLabel->setStyleSheet("color: #e74c3c; background: transparent;");
    return;
  }

  m_statusLabel->setText("Connecting...");
  m_statusLabel->setStyleSheet(
      "color: #00a8ff; background: transparent; font-weight: bold;");
  m_loginButton->setEnabled(false);

  NetworkManager::instance().connectToHost(host, port);
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
