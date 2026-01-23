#include "AddFriendDialog.h"
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QVBoxLayout>

AddFriendDialog::AddFriendDialog(QWidget *parent) : QDialog(parent) {
  setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
  setAttribute(Qt::WA_TranslucentBackground);
  setFixedSize(300, 180);
  setupUI();
}

QString AddFriendDialog::getUsername() const { return m_usernameInput->text(); }

void AddFriendDialog::setupUI() {
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(10, 10, 10, 10);

  // Glass Frame
  QFrame *glassFrame = new QFrame(this);
  glassFrame->setObjectName("dialogFrame");
  glassFrame->setStyleSheet(R"(
        #dialogFrame {
            background-color: rgba(255, 255, 255, 240);
            border: 1px solid rgba(255, 255, 255, 200);
            border-radius: 20px;
        }
    )");

  QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
  shadow->setBlurRadius(20);
  shadow->setColor(QColor(0, 0, 0, 40));
  shadow->setOffset(0, 5);
  glassFrame->setGraphicsEffect(shadow);

  QVBoxLayout *frameLayout = new QVBoxLayout(glassFrame);
  frameLayout->setContentsMargins(20, 20, 20, 20);
  frameLayout->setSpacing(15);

  // Title
  QLabel *titleLabel = new QLabel("Add Friend", glassFrame);
  titleLabel->setStyleSheet(
      "font-size: 18px; font-weight: 700; color: #1a2530; "
      "background: transparent;");
  titleLabel->setAlignment(Qt::AlignCenter);
  frameLayout->addWidget(titleLabel);

  // Input
  m_usernameInput = new QLineEdit(glassFrame);
  m_usernameInput->setPlaceholderText("Enter username");
  m_usernameInput->setStyleSheet(R"(
        QLineEdit {
            background-color: rgba(240, 245, 250, 200);
            border: 1px solid rgba(200, 220, 240, 150);
            border-radius: 10px;
            padding: 8px 12px;
            font-size: 14px;
            color: #2d3748;
        }
        QLineEdit:focus {
            border: 1px solid #4A90E2;
            background-color: #FFFFFF;
        }
    )");
  frameLayout->addWidget(m_usernameInput);

  // Buttons
  QHBoxLayout *btnLayout = new QHBoxLayout();
  btnLayout->setSpacing(10);

  QPushButton *cancelBtn = new QPushButton("Cancel", glassFrame);
  cancelBtn->setCursor(Qt::PointingHandCursor);
  cancelBtn->setStyleSheet(R"(
        QPushButton {
            background-color: transparent;
            border: 1px solid rgba(200, 200, 200, 150);
            border-radius: 10px;
            padding: 6px 12px;
            color: #718096;
            font-weight: 600;
        }
        QPushButton:hover {
            background-color: rgba(0, 0, 0, 10);
            color: #4a5568;
        }
    )");
  connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

  QPushButton *addBtn = new QPushButton("Add", glassFrame);
  addBtn->setCursor(Qt::PointingHandCursor);
  addBtn->setStyleSheet(R"(
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #4facfe, stop:1 #00f2fe);
            border: none;
            border-radius: 10px;
            padding: 6px 20px;
            color: white;
            font-weight: 700;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #439ce0, stop:1 #00dce8);
        }
    )");
  connect(addBtn, &QPushButton::clicked, this, &QDialog::accept);

  btnLayout->addWidget(cancelBtn);
  btnLayout->addWidget(addBtn);

  frameLayout->addLayout(btnLayout);
  mainLayout->addWidget(glassFrame);
}
