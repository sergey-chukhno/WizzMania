#include "RegisterDialog.h"
#include "../common/Packet.h"
#include "NetworkManager.h"
#include <QGraphicsDropShadowEffect>
#include <QImage>
#include <QMessageBox>
#include <QTimer>

RegisterDialog::RegisterDialog(QWidget *parent) : QDialog(parent) {
  setupUI();
  applyStyles();

  connect(&NetworkManager::instance(), &NetworkManager::connected, this,
          [this]() {
            m_statusLabel->setText("Registering...");
            m_statusLabel->setStyleSheet(
                "color: #00f2fe; font-weight: bold; background: transparent;");

            wizz::Packet regPkt(wizz::PacketType::Register);
            regPkt.writeString(m_usernameInput->text().toStdString());
            regPkt.writeString(m_passwordInput->text().toStdString());
            NetworkManager::instance().sendPacket(regPkt);
          });

  connect(&NetworkManager::instance(), &NetworkManager::packetReceived, this,
          [this](const wizz::Packet &constPkt) {
            wizz::Packet pkt = constPkt;
            if (pkt.type() == wizz::PacketType::RegisterSuccess) {
              onRegisterSuccess();
            } else if (pkt.type() == wizz::PacketType::RegisterFailed) {
              std::string reason = pkt.readString();
              onRegisterFailed(QString::fromStdString(reason));
            } else if (pkt.type() == wizz::PacketType::Error) {
              std::string err = pkt.readString();
              onRegisterFailed("Error: " + QString::fromStdString(err));
            }
          });

  connect(&NetworkManager::instance(), &NetworkManager::errorOccurred, this,
          &RegisterDialog::onConnectionError);
}

QPixmap RegisterDialog::processTransparentImage(const QString &path, int size) {
  QPixmap pix(path);
  if (pix.isNull())
    return QPixmap();
  return pix.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

void RegisterDialog::setupUI() {
  setWindowTitle("Wizz Mania - Register");
  resize(1024, 768);
  setMinimumSize(1024, 768);

  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setAlignment(Qt::AlignCenter);

  // --- Outer Glass Card ---
  QFrame *glassCard = new QFrame(this);
  glassCard->setObjectName("glassCard");
  glassCard->setFixedSize(500, 540);

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
  QLabel *taglineLabel = new QLabel("Create your account", glassCard);
  taglineLabel->setObjectName("taglineLabel");
  cardLayout->addWidget(taglineLabel);
  cardLayout->addSpacing(15);

  // --- Inner Glass Frame ---
  QFrame *innerFrame = new QFrame(glassCard);
  innerFrame->setObjectName("innerFrame");
  innerFrame->setFixedSize(420, 310);

  QGraphicsDropShadowEffect *innerShadow = new QGraphicsDropShadowEffect(this);
  innerShadow->setBlurRadius(20);
  innerShadow->setColor(QColor(150, 200, 255, 80));
  innerShadow->setOffset(0, 0);
  innerFrame->setGraphicsEffect(innerShadow);

  QVBoxLayout *innerLayout = new QVBoxLayout(innerFrame);
  innerLayout->setContentsMargins(25, 25, 25, 25);
  innerLayout->setSpacing(12);

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

  // Confirm Password Input with Icon
  QHBoxLayout *confirmRow = new QHBoxLayout();
  QLabel *confirmIcon = new QLabel(innerFrame);
  confirmIcon->setPixmap(processTransparentImage(":/assets/icon_lock.png", 24));
  confirmIcon->setFixedSize(24, 24);
  confirmIcon->setStyleSheet("background: transparent;");

  m_confirmPasswordInput = new QLineEdit(innerFrame);
  m_confirmPasswordInput->setPlaceholderText("Confirm Password");
  m_confirmPasswordInput->setEchoMode(QLineEdit::Password);
  m_confirmPasswordInput->setObjectName("glassInput");

  confirmRow->addWidget(confirmIcon);
  confirmRow->addWidget(m_confirmPasswordInput);
  innerLayout->addLayout(confirmRow);

  innerLayout->addSpacing(8);

  // Register Button (Glass style)
  m_registerButton = new QPushButton("Register >", innerFrame);
  m_registerButton->setCursor(Qt::PointingHandCursor);
  m_registerButton->setFixedHeight(48);
  m_registerButton->setObjectName("glassSignInBtn");

  QGraphicsDropShadowEffect *btnShadow = new QGraphicsDropShadowEffect(this);
  btnShadow->setBlurRadius(20);
  btnShadow->setColor(QColor(100, 180, 255, 100));
  btnShadow->setOffset(0, 4);
  m_registerButton->setGraphicsEffect(btnShadow);

  innerLayout->addWidget(m_registerButton);

  // Back to Login Link
  QHBoxLayout *backRow = new QHBoxLayout();

  QPushButton *backToLogin = new QPushButton("Back to login", innerFrame);
  backToLogin->setCursor(Qt::PointingHandCursor);
  backToLogin->setObjectName("secondaryBtn");

  backRow->addStretch();
  backRow->addWidget(backToLogin);
  backRow->addStretch();

  innerLayout->addLayout(backRow);

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

  // Connect signals
  connect(m_registerButton, &QPushButton::clicked, this,
          &RegisterDialog::onRegisterClicked);
  connect(backToLogin, &QPushButton::clicked, this, [this]() {
    emit backToLoginRequested();
    reject();
  });
}

void RegisterDialog::applyStyles() {
  this->setStyleSheet(R"(
        RegisterDialog {
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

void RegisterDialog::onRegisterClicked() {
  QString username = m_usernameInput->text().trimmed();
  QString password = m_passwordInput->text();
  QString confirm = m_confirmPasswordInput->text();

  // Validate inputs
  if (username.isEmpty()) {
    m_statusLabel->setText("Please enter a username");
    m_statusLabel->setStyleSheet("color: #e74c3c; background: transparent;");
    return;
  }

  if (password.isEmpty()) {
    m_statusLabel->setText("Please enter a password");
    m_statusLabel->setStyleSheet("color: #e74c3c; background: transparent;");
    return;
  }

  if (password != confirm) {
    m_statusLabel->setText("Passwords do not match");
    m_statusLabel->setStyleSheet("color: #e74c3c; background: transparent;");
    return;
  }

  if (password.length() < 4) {
    m_statusLabel->setText("Password must be at least 4 characters");
    m_statusLabel->setStyleSheet("color: #e74c3c; background: transparent;");
    return;
  }

  m_statusLabel->setText("Connecting...");
  m_statusLabel->setStyleSheet(
      "color: #00a8ff; background: transparent; font-weight: bold;");
  m_registerButton->setEnabled(false);

  NetworkManager::instance().connectToHost(m_defaultHost, m_defaultPort);
}

void RegisterDialog::onRegisterSuccess() {
  m_statusLabel->setText("Account created! Returning to login...");
  m_statusLabel->setStyleSheet(
      "color: #27ae60; font-weight: bold; background: transparent;");

  // Disconnect from server and return to login after short delay
  NetworkManager::instance().disconnect();

  QTimer::singleShot(1500, this, [this]() {
    emit backToLoginRequested();
    accept();
  });
}

void RegisterDialog::onRegisterFailed(const QString &reason) {
  m_statusLabel->setText(reason);
  m_statusLabel->setStyleSheet(
      "color: #e74c3c; font-weight: bold; background: transparent;");
  m_registerButton->setEnabled(true);
  NetworkManager::instance().disconnect();
}

void RegisterDialog::onConnectionError(const QString &error) {
  m_statusLabel->setText("Connection error: " + error);
  m_statusLabel->setStyleSheet("color: #e74c3c; background: transparent;");
  m_registerButton->setEnabled(true);
}
