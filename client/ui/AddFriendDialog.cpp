#include "AddFriendDialog.h"
#include "theme/ThemeEngine.h"
#include "widgets/TitleBar.h"
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QVBoxLayout>
#include <QTimer>

using TE = wizz::ui::ThemeEngine;

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

void AddFriendDialog::paintEvent(QPaintEvent* event) {
  Q_UNUSED(event);
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

  QPainterPath path;
  path.addRoundedRect(rect(), TE::rLg, TE::rLg);

  painter.fillPath(path, TE::surfaceHigh());
  painter.setPen(QPen(QColor(255, 255, 255, 20), 1));
  painter.drawPath(path);
}

void AddFriendDialog::setupUI() {
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(0);

  auto *titleBar = new TitleBar("Add Friend", this);
  mainLayout->addWidget(titleBar);

  QWidget *contentWidget = new QWidget(this);
  QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
  contentLayout->setContentsMargins(25, 10, 25, 25);
  contentLayout->setSpacing(15);
  mainLayout->addWidget(contentWidget);

  // Icon / Header
  QLabel *iconLabel = new QLabel("👤", contentWidget);
  iconLabel->setStyleSheet("font-size: 40px; background: transparent;");
  iconLabel->setAlignment(Qt::AlignCenter);
  contentLayout->addWidget(iconLabel);

  // Error Label
  m_errorLabel = new QLabel("", contentWidget);
  m_errorLabel->setStyleSheet(QString("font-size: 12px; color: %1; font-weight: 600; background: transparent;").arg(TE::danger().name()));
  m_errorLabel->setAlignment(Qt::AlignCenter);
  m_errorLabel->setFixedHeight(20);
  contentLayout->addWidget(m_errorLabel);

  // Input
  m_usernameInput = new QLineEdit(contentWidget);
  m_usernameInput->setPlaceholderText("Enter username");
  m_usernameInput->setAttribute(Qt::WA_MacShowFocusRect, false);
  
  auto updateInputStyle = [this](bool isValid, bool isFocused) {
      QString borderColor = isFocused ? TE::accent().name() : "rgba(255, 255, 255, 15)";
      if (!m_usernameInput->text().isEmpty()) {
          borderColor = isValid ? TE::success().name() : TE::danger().name();
      }
      
      m_usernameInput->setStyleSheet(QString(R"(
          QLineEdit {
              background-color: %1;
              border: 1px solid %2;
              border-bottom: 2px solid %2;
              border-radius: %3px;
              padding: 10px 15px;
              font-size: 14px;
              color: %4;
          }
      )").arg(TE::surface().name()).arg(borderColor).arg(TE::rMd).arg(TE::onSurface().name()));
  };
  
  updateInputStyle(true, false);

  connect(m_usernameInput, &QLineEdit::textChanged, this, [this, updateInputStyle](const QString& text) {
      bool isValid = text.length() >= 3 && !text.contains(" ");
      updateInputStyle(isValid, m_usernameInput->hasFocus());
  });

  connect(m_usernameInput, &QLineEdit::returnPressed, this, &AddFriendDialog::onAddClicked);
  contentLayout->addWidget(m_usernameInput);

  contentLayout->addStretch();

  // Buttons
  QHBoxLayout *btnLayout = new QHBoxLayout();
  btnLayout->setSpacing(15);

  m_cancelBtn = new QPushButton("Cancel", contentWidget);
  m_cancelBtn->setCursor(Qt::PointingHandCursor);
  m_cancelBtn->setFixedHeight(36);
  m_cancelBtn->setStyleSheet(QString(R"(
        QPushButton {
            background-color: transparent;
            border: 1px solid rgba(255, 255, 255, 30);
            border-radius: %1px;
            color: %2;
            font-weight: 600;
        }
        QPushButton:hover {
            background-color: rgba(255, 255, 255, 10);
            color: %3;
        }
        QPushButton:disabled {
            color: rgba(255, 255, 255, 50);
        }
    )").arg(TE::rMd).arg(TE::onSurface2().name()).arg(TE::onSurface().name()));
  connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

  m_addBtn = new QPushButton("Add", contentWidget);
  m_addBtn->setCursor(Qt::PointingHandCursor);
  m_addBtn->setFixedHeight(36);
  m_addBtn->setStyleSheet(QString(R"(
        QPushButton {
            background: %1;
            border: none;
            border-radius: %2px;
            color: white;
            font-weight: 700;
        }
        QPushButton:hover {
            background: %3;
        }
        QPushButton:disabled {
            background: rgba(255, 255, 255, 20);
            color: rgba(255, 255, 255, 50);
        }
    )").arg(TE::accent().name()).arg(TE::rMd).arg(QColor(TE::accent().name()).lighter(115).name()));
  connect(m_addBtn, &QPushButton::clicked, this, &AddFriendDialog::onAddClicked);

  btnLayout->addWidget(m_cancelBtn);
  btnLayout->addWidget(m_addBtn);

  contentLayout->addLayout(btnLayout);
}
