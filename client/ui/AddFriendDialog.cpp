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
  
  // Deep shadow for depth separation (scrim effect)
  auto* shadow = new QGraphicsDropShadowEffect(this);
  shadow->setBlurRadius(40);
  shadow->setColor(QColor(0, 0, 0, 180));
  shadow->setOffset(0, 0);
  setGraphicsEffect(shadow);
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

  QRectF baseRect = rect().adjusted(1, 1, -1, -1);
  QPainterPath path;
  path.addRoundedRect(baseRect, 24, 24);

  // Unified Arcade Style Gradient
  QLinearGradient grad(0, 0, 0, height());
  grad.setColorAt(0, QColor(40, 45, 60, 245));
  grad.setColorAt(1, QColor(20, 25, 35, 245));
  painter.fillPath(path, grad);

  // Consistent thin border
  painter.setPen(QPen(QColor(255, 255, 255, 40), 1));
  painter.drawPath(path);
}

void AddFriendDialog::setupUI() {
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(0);

  // Sleek minimalist header (Unifies with Arcade modals)
  auto* headerLayout = new QHBoxLayout();
  headerLayout->setContentsMargins(24, 16, 24, 0);
  
  auto* headerTitle = new QLabel("FIND CITIZENS");
  headerTitle->setStyleSheet("color: rgba(255, 255, 255, 150); font-weight: 900; letter-spacing: 3px; font-size: 10px;");
  
  auto* closeBtn = new QPushButton("✕");
  closeBtn->setFixedSize(24, 24);
  closeBtn->setCursor(Qt::PointingHandCursor);
  closeBtn->setStyleSheet(R"(
      QPushButton {
          background: rgba(255, 255, 255, 15);
          color: white;
          border-radius: 12px;
          font-weight: bold;
      }
      QPushButton:hover { background: rgba(255, 255, 255, 30); }
  )");
  connect(closeBtn, &QPushButton::clicked, this, &QDialog::reject);

  headerLayout->addWidget(headerTitle);
  headerLayout->addStretch();
  headerLayout->addWidget(closeBtn);
  mainLayout->addLayout(headerLayout);

  QWidget *contentWidget = new QWidget(this);
  QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
  contentLayout->setContentsMargins(25, 10, 25, 25);
  contentLayout->setSpacing(15);
  mainLayout->addWidget(contentWidget);

  // Icon & Title Row
  auto* titleRow = new QHBoxLayout();
  titleRow->setSpacing(12);

  QLabel *iconLabel = new QLabel("👤", contentWidget);
  iconLabel->setStyleSheet(QString("font-size: 28px; color: %1; background: transparent;").arg(TE::accent().name()));
  iconLabel->setAlignment(Qt::AlignCenter);
  
  auto* titleText = new QLabel("Add a New Friend", contentWidget);
  titleText->setStyleSheet(QString("color: white; font-size: 16px; font-weight: 800;"));
  
  titleRow->addWidget(iconLabel);
  titleRow->addWidget(titleText);
  titleRow->addStretch();
  contentLayout->addLayout(titleRow);

  // Error Label
  m_errorLabel = new QLabel("", contentWidget);
  m_errorLabel->setStyleSheet(QString("font-size: 12px; color: %1; font-weight: 600; background: transparent;").arg(TE::danger().name()));
  m_errorLabel->setAlignment(Qt::AlignCenter);
  m_errorLabel->setFixedHeight(20);
  contentLayout->addWidget(m_errorLabel);

  // Input
  m_usernameInput = new QLineEdit(contentWidget);
  m_usernameInput->setPlaceholderText("Target Username");
  m_usernameInput->setAttribute(Qt::WA_MacShowFocusRect, false);
  
  auto updateInputStyle = [this](bool isValid, bool isFocused) {
      QString borderColor = isFocused ? TE::accent().name() : "rgba(255, 255, 255, 20)";
      if (!m_usernameInput->text().isEmpty()) {
          borderColor = isValid ? TE::success().name() : TE::danger().name();
      }
      
      m_usernameInput->setStyleSheet(QString(R"(
          QLineEdit {
              background-color: rgba(0, 0, 0, 100);
              border: 1px solid %1;
              border-radius: %2px;
              padding: 10px 15px;
              font-size: 14px;
              color: white;
          }
      )").arg(borderColor).arg(TE::rMd));
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
