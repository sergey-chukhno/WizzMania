#include "AddFriendDialog.h"
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QVBoxLayout>
#include <QTimer>

AddFriendDialog::AddFriendDialog(QWidget *parent) : QDialog(parent) {
  setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
  setAttribute(Qt::WA_TranslucentBackground);
  setFixedSize(320, 240);
  
  m_watchdog = new QTimer(this);
  m_watchdog->setSingleShot(true);
  connect(m_watchdog, &QTimer::timeout, this, &AddFriendDialog::onWatchdogTimeout);

  setupUI();
}

QString AddFriendDialog::getUsername() const { return m_usernameInput->text(); }

void AddFriendDialog::clearInput() {
  m_usernameInput->clear();
  m_errorLabel->clear();
  setLoading(false);
}

void AddFriendDialog::showError(const QString &message) {
  m_errorLabel->setText(message);
  setLoading(false);
}

void AddFriendDialog::setLoading(bool loading) {
  m_addBtn->setEnabled(!loading);
  m_cancelBtn->setEnabled(!loading);
  m_usernameInput->setEnabled(!loading);
  if (loading) {
    m_addBtn->setText("Adding...");
  } else {
    m_addBtn->setText("Add");
    m_watchdog->stop();
  }
}

void AddFriendDialog::onAddClicked() {
  QString text = m_usernameInput->text().trimmed();
  if (!text.isEmpty()) {
    m_errorLabel->clear();
    setLoading(true);
    m_watchdog->start(10000); // 10s Timeout
    emit addRequested(text);
  }
}

void AddFriendDialog::onWatchdogTimeout() {
  if (m_addBtn->text() == "Adding...") {
    showError("Request timed out. Please try again.");
  }
}

void AddFriendDialog::setupUI() {
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(10, 10, 10, 10);

  // Glass Frame
  QFrame *glassFrame = new QFrame(this);
  glassFrame->setObjectName("dialogFrame");
  glassFrame->setStyleSheet(R"(
        #dialogFrame {
            background-color: rgba(255, 255, 255, 20);
            border: 2px solid rgba(255, 255, 255, 180);
            border-radius: 20px;
        }
    )");

  QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
  shadow->setBlurRadius(20);
  shadow->setColor(QColor(0, 0, 0, 40));
  shadow->setOffset(0, 5);
  glassFrame->setGraphicsEffect(shadow);

  QVBoxLayout *frameLayout = new QVBoxLayout(glassFrame);
  frameLayout->setContentsMargins(25, 25, 25, 25);
  frameLayout->setSpacing(15);

  // Title
  QLabel *titleLabel = new QLabel("Add Friend", glassFrame);
  titleLabel->setStyleSheet(
      "font-size: 20px; font-weight: 700; color: #1a2530; "
      "background: transparent;");
  titleLabel->setAlignment(Qt::AlignCenter);
  frameLayout->addWidget(titleLabel);

  // Error Label
  m_errorLabel = new QLabel("", glassFrame);
  m_errorLabel->setStyleSheet(
      "font-size: 12px; color: #e53e3e; font-weight: 600; "
      "background: transparent;");
  m_errorLabel->setAlignment(Qt::AlignCenter);
  m_errorLabel->setMinimumWidth(300);
  m_errorLabel->setFixedHeight(20);
  frameLayout->addWidget(m_errorLabel);

  // Input
  m_usernameInput = new QLineEdit(glassFrame);
  m_usernameInput->setPlaceholderText("Enter username");
  m_usernameInput->setStyleSheet(R"(
        QLineEdit {
            background-color: rgba(255, 255, 255, 180);
            border: 1px solid rgba(200, 220, 240, 150);
            border-radius: 12px;
            padding: 10px 15px;
            font-size: 14px;
            color: #2d3748;
        }
        QLineEdit:focus {
            border: 1px solid #4A90E2;
            background-color: #FFFFFF;
        }
    )");
  connect(m_usernameInput, &QLineEdit::returnPressed, this,
          &AddFriendDialog::onAddClicked);
  frameLayout->addWidget(m_usernameInput);

  // Buttons
  QHBoxLayout *btnLayout = new QHBoxLayout();
  btnLayout->setSpacing(15);

  m_cancelBtn = new QPushButton("Cancel", glassFrame);
  m_cancelBtn->setCursor(Qt::PointingHandCursor);
  m_cancelBtn->setStyleSheet(R"(
        QPushButton {
            background-color: rgba(255, 255, 255, 150);
            border: 1px solid rgba(200, 200, 200, 150);
            border-radius: 12px;
            padding: 8px 15px;
            color: #4a5568;
            font-weight: 600;
        }
        QPushButton:hover {
            background-color: rgba(255, 255, 255, 220);
            color: #2d3748;
        }
        QPushButton:disabled {
            background-color: rgba(200, 200, 200, 100);
            color: #a0aec0;
        }
    )");
  connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

  m_addBtn = new QPushButton("Add", glassFrame);
  m_addBtn->setCursor(Qt::PointingHandCursor);
  m_addBtn->setStyleSheet(R"(
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #4facfe, stop:1 #00f2fe);
            border: none;
            border-radius: 12px;
            padding: 8px 20px;
            color: white;
            font-weight: 700;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #439ce0, stop:1 #00dce8);
        }
        QPushButton:disabled {
            background: rgba(100, 180, 255, 80);
            color: rgba(255, 255, 255, 150);
        }
    )");
  connect(m_addBtn, &QPushButton::clicked, this, &AddFriendDialog::onAddClicked);

  btnLayout->addWidget(m_cancelBtn);
  btnLayout->addWidget(m_addBtn);

  frameLayout->addLayout(btnLayout);
  mainLayout->addWidget(glassFrame);
}
