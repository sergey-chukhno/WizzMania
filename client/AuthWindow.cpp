#include "AuthWindow.h"
#include "../common/Packet.h"
#include "NetworkManager.h"
#include <QGraphicsDropShadowEffect>
#include <QMessageBox>
#include <QPaintEvent>
#include <QTimer>

AuthWindow::AuthWindow(QWidget *parent) : QWidget(parent) {
  setWindowTitle("Wizz Mania");
  setFixedSize(1024, 768);

  // Load background image
  m_backgroundPixmap = QPixmap(":/assets/login_bg.png");

  setupUI();
}

void AuthWindow::paintEvent(QPaintEvent *event) {
  QPainter painter(this);

  // Draw background scaled to fill window
  if (!m_backgroundPixmap.isNull()) {
    painter.drawPixmap(rect(), m_backgroundPixmap.scaled(
                                   size(), Qt::KeepAspectRatioByExpanding,
                                   Qt::SmoothTransformation));
  }

  QWidget::paintEvent(event);
}

QPixmap AuthWindow::processTransparentImage(const QString &path, int size) {
  QPixmap pix(path);
  if (pix.isNull())
    return QPixmap();
  return pix.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

void AuthWindow::setupUI() {
  // Set background
  setStyleSheet(R"(
        AuthWindow {
            background-image: url(:/assets/login_bg.png);
            background-position: center;
            background-repeat: no-repeat;
        }
    )");

  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);

  // Create stacked widget for smooth page transitions
  m_stack = new QStackedWidget(this);
  m_stack->setStyleSheet("background: transparent;");
  m_stack->setAttribute(Qt::WA_TranslucentBackground);

  // Create Login Page
  m_loginPage = new QWidget();
  m_loginPage->setStyleSheet("background: transparent;");
  m_loginPage->setAttribute(Qt::WA_TranslucentBackground);
  QVBoxLayout *loginLayout = new QVBoxLayout(m_loginPage);
  loginLayout->setAlignment(Qt::AlignCenter);

  // Create Register Page
  m_registerPage = new QWidget();
  m_registerPage->setStyleSheet("background: transparent;");
  m_registerPage->setAttribute(Qt::WA_TranslucentBackground);
  QVBoxLayout *registerLayout = new QVBoxLayout(m_registerPage);
  registerLayout->setAlignment(Qt::AlignCenter);

  // --- Create Login Content ---
  QFrame *loginCard = createLoginCard();
  loginLayout->addWidget(loginCard);

  // --- Create Register Content ---
  QFrame *registerCard = createRegisterCard();
  registerLayout->addWidget(registerCard);

  // Add pages to stack
  m_stack->addWidget(m_loginPage);    // index 0
  m_stack->addWidget(m_registerPage); // index 1

  mainLayout->addWidget(m_stack);

  // --- Decorative Mascots (shared, placed on top) ---
  QLabel *alienGreen = new QLabel(this);
  alienGreen->setPixmap(
      processTransparentImage(":/assets/alien_green.png", 140));
  alienGreen->setFixedSize(140, 140);
  alienGreen->move(140, 540);
  alienGreen->setStyleSheet("background: transparent;");
  alienGreen->raise();

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
  ufoGold->raise();

  QGraphicsDropShadowEffect *goldShadow = new QGraphicsDropShadowEffect(this);
  goldShadow->setBlurRadius(25);
  goldShadow->setColor(QColor(0, 0, 0, 80));
  goldShadow->setOffset(0, 12);
  ufoGold->setGraphicsEffect(goldShadow);
}

QFrame *AuthWindow::createLoginCard() {
  QFrame *glassCard = new QFrame();
  glassCard->setObjectName("glassCard");
  glassCard->setFixedSize(500, 520);
  glassCard->setStyleSheet(R"(
        #glassCard {
            background-color: rgba(255, 255, 255, 35);
            border: 2px solid rgba(255, 255, 255, 120);
            border-top: 2px solid rgba(255, 255, 255, 180);
            border-bottom: 2px solid rgba(255, 255, 255, 80);
            border-radius: 35px;
        }
    )");

  QGraphicsDropShadowEffect *cardShadow = new QGraphicsDropShadowEffect(this);
  cardShadow->setBlurRadius(60);
  cardShadow->setColor(QColor(0, 60, 120, 100));
  cardShadow->setOffset(0, 15);
  glassCard->setGraphicsEffect(cardShadow);

  QVBoxLayout *cardLayout = new QVBoxLayout(glassCard);
  cardLayout->setContentsMargins(40, 30, 40, 35);
  cardLayout->setSpacing(12);

  // Header with Title and Butterfly
  QHBoxLayout *headerLayout = new QHBoxLayout();

  QLabel *titleLabel = new QLabel("Wizz Mania", glassCard);
  titleLabel->setStyleSheet("font-size: 34px; font-weight: 700; color: "
                            "#1a2530; background: transparent;");

  QLabel *butterflyIcon = new QLabel(glassCard);
  butterflyIcon->setPixmap(
      processTransparentImage(":/assets/butterfly.png", 70));
  butterflyIcon->setFixedSize(70, 70);
  butterflyIcon->setStyleSheet("background: transparent;");

  headerLayout->addWidget(titleLabel);
  headerLayout->addStretch();
  headerLayout->addWidget(butterflyIcon);
  cardLayout->addLayout(headerLayout);

  // Tagline
  QLabel *taglineLabel =
      new QLabel("It's not 2003, but you can still log in.", glassCard);
  taglineLabel->setStyleSheet(
      "font-size: 14px; color: #4a5568; background: transparent;");
  cardLayout->addWidget(taglineLabel);
  cardLayout->addSpacing(20);

  // Inner Glass Frame
  QFrame *innerFrame = new QFrame(glassCard);
  innerFrame->setFixedSize(420, 280);
  innerFrame->setStyleSheet(R"(
        background-color: rgba(255, 255, 255, 25);
        border: 2px solid rgba(200, 230, 255, 150);
        border-radius: 20px;
    )");

  QVBoxLayout *innerLayout = new QVBoxLayout(innerFrame);
  innerLayout->setContentsMargins(25, 25, 25, 25);
  innerLayout->setSpacing(15);

  // Username Input
  QHBoxLayout *userRow = new QHBoxLayout();
  QLabel *userIcon = new QLabel(innerFrame);
  userIcon->setPixmap(processTransparentImage(":/assets/icon_user.png", 24));
  userIcon->setFixedSize(24, 24);
  userIcon->setStyleSheet("background: transparent;");

  m_loginUsername = new QLineEdit(innerFrame);
  m_loginUsername->setPlaceholderText("Username");
  m_loginUsername->setStyleSheet(R"(
        background-color: rgba(255, 255, 255, 50);
        border: 1px solid rgba(255, 255, 255, 150);
        border-radius: 20px;
        padding: 12px 18px;
        font-size: 15px;
        color: #2d3748;
    )");

  userRow->addWidget(userIcon, 0, Qt::AlignVCenter);
  userRow->addWidget(m_loginUsername);
  innerLayout->addLayout(userRow);

  // Password Input
  QHBoxLayout *passRow = new QHBoxLayout();
  QLabel *lockIcon = new QLabel(innerFrame);
  lockIcon->setPixmap(processTransparentImage(":/assets/icon_lock.png", 24));
  lockIcon->setFixedSize(24, 24);
  lockIcon->setStyleSheet("background: transparent;");

  m_loginPassword = new QLineEdit(innerFrame);
  m_loginPassword->setPlaceholderText("Password");
  m_loginPassword->setEchoMode(QLineEdit::Password);
  m_loginPassword->setStyleSheet(m_loginUsername->styleSheet());

  passRow->addWidget(lockIcon, 0, Qt::AlignVCenter);
  passRow->addWidget(m_loginPassword);
  innerLayout->addLayout(passRow);

  innerLayout->addSpacing(10);

  // Sign In Button
  m_loginButton = new QPushButton("Sign In >", innerFrame);
  m_loginButton->setCursor(Qt::PointingHandCursor);
  m_loginButton->setFixedHeight(48);
  m_loginButton->setStyleSheet(R"(
        QPushButton {
            background-color: rgba(80, 180, 255, 120);
            border: 2px solid rgba(150, 220, 255, 200);
            border-radius: 24px;
            font-size: 17px;
            font-weight: bold;
            color: white;
        }
        QPushButton:hover {
            background-color: rgba(100, 200, 255, 180);
            border: 2px solid rgba(180, 240, 255, 255);
        }
        QPushButton:pressed {
            background-color: rgba(60, 160, 240, 200);
        }
    )");
  innerLayout->addWidget(m_loginButton);

  // Secondary Buttons
  QHBoxLayout *secondaryRow = new QHBoxLayout();

  QPushButton *createAcc = new QPushButton("Create account", innerFrame);
  createAcc->setCursor(Qt::PointingHandCursor);
  createAcc->setStyleSheet(R"(
        QPushButton {
            background: transparent;
            border: none;
            font-size: 13px;
            color: rgba(60, 80, 100, 180);
        }
        QPushButton:hover {
            color: rgb(0, 120, 200);
        }
    )");

  QLabel *separator = new QLabel("|", innerFrame);
  separator->setStyleSheet(
      "color: rgba(100, 120, 140, 150); background: transparent;");

  QPushButton *offlineMode = new QPushButton("Offline mode", innerFrame);
  offlineMode->setCursor(Qt::PointingHandCursor);
  offlineMode->setStyleSheet(R"(
        QPushButton {
            background: transparent;
            border: none;
            font-size: 13px;
            color: rgba(60, 80, 100, 180);
        }
        QPushButton:hover {
            color: rgb(0, 120, 200);
        }
    )");

  secondaryRow->addStretch();
  secondaryRow->addWidget(createAcc);
  secondaryRow->addWidget(separator);
  secondaryRow->addWidget(offlineMode);
  secondaryRow->addStretch();

  innerLayout->addLayout(secondaryRow);

  cardLayout->addWidget(innerFrame, 0, Qt::AlignCenter);

  // Status Label
  m_loginStatus = new QLabel("", glassCard);
  m_loginStatus->setAlignment(Qt::AlignCenter);
  m_loginStatus->setStyleSheet("background: transparent;");
  cardLayout->addWidget(m_loginStatus);

  // Connect signals
  connect(m_loginButton, &QPushButton::clicked, this,
          &AuthWindow::onLoginClicked);
  connect(createAcc, &QPushButton::clicked, this, [this]() {
    m_stack->setCurrentIndex(1); // Switch to register page
  });

  return glassCard;
}

QFrame *AuthWindow::createRegisterCard() {
  QFrame *glassCard = new QFrame();
  glassCard->setObjectName("glassCard");
  glassCard->setFixedSize(500, 540);
  glassCard->setStyleSheet(R"(
        #glassCard {
            background-color: rgba(255, 255, 255, 35);
            border: 2px solid rgba(255, 255, 255, 120);
            border-top: 2px solid rgba(255, 255, 255, 180);
            border-bottom: 2px solid rgba(255, 255, 255, 80);
            border-radius: 35px;
        }
    )");

  QGraphicsDropShadowEffect *cardShadow = new QGraphicsDropShadowEffect(this);
  cardShadow->setBlurRadius(60);
  cardShadow->setColor(QColor(0, 60, 120, 100));
  cardShadow->setOffset(0, 15);
  glassCard->setGraphicsEffect(cardShadow);

  QVBoxLayout *cardLayout = new QVBoxLayout(glassCard);
  cardLayout->setContentsMargins(40, 30, 40, 35);
  cardLayout->setSpacing(12);

  // Header
  QHBoxLayout *headerLayout = new QHBoxLayout();

  QLabel *titleLabel = new QLabel("Wizz Mania", glassCard);
  titleLabel->setStyleSheet("font-size: 34px; font-weight: 700; color: "
                            "#1a2530; background: transparent;");

  QLabel *butterflyIcon = new QLabel(glassCard);
  butterflyIcon->setPixmap(
      processTransparentImage(":/assets/butterfly.png", 70));
  butterflyIcon->setFixedSize(70, 70);
  butterflyIcon->setStyleSheet("background: transparent;");

  headerLayout->addWidget(titleLabel);
  headerLayout->addStretch();
  headerLayout->addWidget(butterflyIcon);
  cardLayout->addLayout(headerLayout);

  // Tagline
  QLabel *taglineLabel = new QLabel("Create your account", glassCard);
  taglineLabel->setStyleSheet(
      "font-size: 14px; color: #4a5568; background: transparent;");
  cardLayout->addWidget(taglineLabel);
  cardLayout->addSpacing(15);

  // Inner Glass Frame
  QFrame *innerFrame = new QFrame(glassCard);
  innerFrame->setFixedSize(420, 310);
  innerFrame->setStyleSheet(R"(
        background-color: rgba(255, 255, 255, 25);
        border: 2px solid rgba(200, 230, 255, 150);
        border-radius: 20px;
    )");

  QVBoxLayout *innerLayout = new QVBoxLayout(innerFrame);
  innerLayout->setContentsMargins(25, 25, 25, 25);
  innerLayout->setSpacing(12);

  QString inputStyle = R"(
        background-color: rgba(255, 255, 255, 50);
        border: 1px solid rgba(255, 255, 255, 150);
        border-radius: 20px;
        padding: 12px 18px;
        font-size: 15px;
        color: #2d3748;
    )";

  // Username
  QHBoxLayout *userRow = new QHBoxLayout();
  QLabel *userIcon = new QLabel(innerFrame);
  userIcon->setPixmap(processTransparentImage(":/assets/icon_user.png", 24));
  userIcon->setFixedSize(24, 24);
  userIcon->setStyleSheet("background: transparent;");

  m_regUsername = new QLineEdit(innerFrame);
  m_regUsername->setPlaceholderText("Username");
  m_regUsername->setStyleSheet(inputStyle);

  userRow->addWidget(userIcon, 0, Qt::AlignVCenter);
  userRow->addWidget(m_regUsername);
  innerLayout->addLayout(userRow);

  // Password
  QHBoxLayout *passRow = new QHBoxLayout();
  QLabel *lockIcon = new QLabel(innerFrame);
  lockIcon->setPixmap(processTransparentImage(":/assets/icon_lock.png", 24));
  lockIcon->setFixedSize(24, 24);
  lockIcon->setStyleSheet("background: transparent;");

  m_regPassword = new QLineEdit(innerFrame);
  m_regPassword->setPlaceholderText("Password");
  m_regPassword->setEchoMode(QLineEdit::Password);
  m_regPassword->setStyleSheet(inputStyle);

  passRow->addWidget(lockIcon, 0, Qt::AlignVCenter);
  passRow->addWidget(m_regPassword);
  innerLayout->addLayout(passRow);

  // Confirm Password
  QHBoxLayout *confirmRow = new QHBoxLayout();
  QLabel *confirmIcon = new QLabel(innerFrame);
  confirmIcon->setPixmap(processTransparentImage(":/assets/icon_lock.png", 24));
  confirmIcon->setFixedSize(24, 24);
  confirmIcon->setStyleSheet("background: transparent;");

  m_regConfirmPassword = new QLineEdit(innerFrame);
  m_regConfirmPassword->setPlaceholderText("Confirm Password");
  m_regConfirmPassword->setEchoMode(QLineEdit::Password);
  m_regConfirmPassword->setStyleSheet(inputStyle);

  confirmRow->addWidget(confirmIcon, 0, Qt::AlignVCenter);
  confirmRow->addWidget(m_regConfirmPassword);
  innerLayout->addLayout(confirmRow);

  innerLayout->addSpacing(8);

  // Register Button
  m_registerButton = new QPushButton("Register >", innerFrame);
  m_registerButton->setCursor(Qt::PointingHandCursor);
  m_registerButton->setFixedHeight(48);
  m_registerButton->setStyleSheet(R"(
        QPushButton {
            background-color: rgba(80, 180, 255, 120);
            border: 2px solid rgba(150, 220, 255, 200);
            border-radius: 24px;
            font-size: 17px;
            font-weight: bold;
            color: white;
        }
        QPushButton:hover {
            background-color: rgba(100, 200, 255, 180);
            border: 2px solid rgba(180, 240, 255, 255);
        }
        QPushButton:pressed {
            background-color: rgba(60, 160, 240, 200);
        }
    )");
  innerLayout->addWidget(m_registerButton);

  // Back to Login
  QHBoxLayout *backRow = new QHBoxLayout();
  QPushButton *backToLogin = new QPushButton("Back to login", innerFrame);
  backToLogin->setCursor(Qt::PointingHandCursor);
  backToLogin->setStyleSheet(R"(
        QPushButton {
            background: transparent;
            border: none;
            font-size: 13px;
            color: rgba(60, 80, 100, 180);
        }
        QPushButton:hover {
            color: rgb(0, 120, 200);
        }
    )");

  backRow->addStretch();
  backRow->addWidget(backToLogin);
  backRow->addStretch();

  innerLayout->addLayout(backRow);

  cardLayout->addWidget(innerFrame, 0, Qt::AlignCenter);

  // Status Label
  m_regStatus = new QLabel("", glassCard);
  m_regStatus->setAlignment(Qt::AlignCenter);
  m_regStatus->setStyleSheet("background: transparent;");
  cardLayout->addWidget(m_regStatus);

  // Connect signals
  connect(m_registerButton, &QPushButton::clicked, this,
          &AuthWindow::onRegisterClicked);
  connect(backToLogin, &QPushButton::clicked, this, [this]() {
    m_stack->setCurrentIndex(0); // Switch to login page
    m_regStatus->clear();
  });

  return glassCard;
}

void AuthWindow::onLoginClicked() {
  if (m_loginUsername->text().isEmpty()) {
    m_loginStatus->setText("Please enter a username");
    m_loginStatus->setStyleSheet("color: #e74c3c; background: transparent;");
    return;
  }

  m_loginStatus->setText("Connecting...");
  m_loginStatus->setStyleSheet(
      "color: #00a8ff; background: transparent; font-weight: bold;");
  m_loginButton->setEnabled(false);

  // Disconnect any existing connections first
  NetworkManager::instance().disconnect();

  // Connect signals for login
  connect(&NetworkManager::instance(), &NetworkManager::connected, this,
          &AuthWindow::onLoginConnected, Qt::UniqueConnection);
  connect(&NetworkManager::instance(), &NetworkManager::packetReceived, this,
          &AuthWindow::onLoginPacketReceived, Qt::UniqueConnection);
  connect(&NetworkManager::instance(), &NetworkManager::errorOccurred, this,
          &AuthWindow::onLoginError, Qt::UniqueConnection);

  NetworkManager::instance().connectToHost("127.0.0.1", 8080);
}

void AuthWindow::onLoginConnected() {
  m_loginStatus->setText("Verifying...");

  wizz::Packet loginPkt(wizz::PacketType::Login);
  loginPkt.writeString(m_loginUsername->text().toStdString());
  loginPkt.writeString(m_loginPassword->text().toStdString());
  NetworkManager::instance().sendPacket(loginPkt);
}

void AuthWindow::onLoginPacketReceived(const wizz::Packet &constPkt) {
  wizz::Packet pkt = constPkt;

  if (pkt.type() == wizz::PacketType::LoginSuccess) {
    m_loginStatus->setText("Login successful!");
    m_loginStatus->setStyleSheet(
        "color: #27ae60; font-weight: bold; background: transparent;");
    emit loginSuccessful();
  } else if (pkt.type() == wizz::PacketType::LoginFailed) {
    std::string reason = pkt.readString();
    m_loginStatus->setText(QString::fromStdString(reason));
    m_loginStatus->setStyleSheet(
        "color: #e74c3c; font-weight: bold; background: transparent;");
    m_loginButton->setEnabled(true);
    NetworkManager::instance().disconnect();
  }
}

void AuthWindow::onLoginError(const QString &error) {
  m_loginStatus->setText("Error: " + error);
  m_loginStatus->setStyleSheet("color: #e74c3c; background: transparent;");
  m_loginButton->setEnabled(true);
}

void AuthWindow::onRegisterClicked() {
  QString username = m_regUsername->text().trimmed();
  QString password = m_regPassword->text();
  QString confirm = m_regConfirmPassword->text();

  if (username.isEmpty()) {
    m_regStatus->setText("Please enter a username");
    m_regStatus->setStyleSheet("color: #e74c3c; background: transparent;");
    return;
  }
  if (password.isEmpty()) {
    m_regStatus->setText("Please enter a password");
    m_regStatus->setStyleSheet("color: #e74c3c; background: transparent;");
    return;
  }
  if (password != confirm) {
    m_regStatus->setText("Passwords do not match");
    m_regStatus->setStyleSheet("color: #e74c3c; background: transparent;");
    return;
  }
  if (password.length() < 4) {
    m_regStatus->setText("Password must be at least 4 characters");
    m_regStatus->setStyleSheet("color: #e74c3c; background: transparent;");
    return;
  }

  m_regStatus->setText("Connecting...");
  m_regStatus->setStyleSheet(
      "color: #00a8ff; background: transparent; font-weight: bold;");
  m_registerButton->setEnabled(false);

  // Disconnect previous and connect for register
  NetworkManager::instance().disconnect();

  connect(&NetworkManager::instance(), &NetworkManager::connected, this,
          &AuthWindow::onRegisterConnected, Qt::UniqueConnection);
  connect(&NetworkManager::instance(), &NetworkManager::packetReceived, this,
          &AuthWindow::onRegisterPacketReceived, Qt::UniqueConnection);
  connect(&NetworkManager::instance(), &NetworkManager::errorOccurred, this,
          &AuthWindow::onRegisterError, Qt::UniqueConnection);

  NetworkManager::instance().connectToHost("127.0.0.1", 8080);
}

void AuthWindow::onRegisterConnected() {
  m_regStatus->setText("Registering...");

  wizz::Packet regPkt(wizz::PacketType::Register);
  regPkt.writeString(m_regUsername->text().toStdString());
  regPkt.writeString(m_regPassword->text().toStdString());
  NetworkManager::instance().sendPacket(regPkt);
}

void AuthWindow::onRegisterPacketReceived(const wizz::Packet &constPkt) {
  wizz::Packet pkt = constPkt;

  if (pkt.type() == wizz::PacketType::RegisterSuccess) {
    m_regStatus->setText("Account created! Returning to login...");
    m_regStatus->setStyleSheet(
        "color: #27ae60; font-weight: bold; background: transparent;");
    NetworkManager::instance().disconnect();

    QTimer::singleShot(1500, this, [this]() {
      m_stack->setCurrentIndex(0); // Back to login
      m_regStatus->clear();
      m_regUsername->clear();
      m_regPassword->clear();
      m_regConfirmPassword->clear();
      m_registerButton->setEnabled(true);
    });
  } else if (pkt.type() == wizz::PacketType::RegisterFailed) {
    std::string reason = pkt.readString();
    m_regStatus->setText(QString::fromStdString(reason));
    m_regStatus->setStyleSheet(
        "color: #e74c3c; font-weight: bold; background: transparent;");
    m_registerButton->setEnabled(true);
    NetworkManager::instance().disconnect();
  }
}

void AuthWindow::onRegisterError(const QString &error) {
  m_regStatus->setText("Error: " + error);
  m_regStatus->setStyleSheet("color: #e74c3c; background: transparent;");
  m_registerButton->setEnabled(true);
}
