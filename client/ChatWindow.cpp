#include "ChatWindow.h"
#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QGraphicsDropShadowEffect>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QScrollBar>
#include <QWidgetAction>

#include <QRandomGenerator>
#include <QUrl>

ChatWindow::ChatWindow(const QString &partnerName, const QPoint &initialPos,
                       QWidget *parent)
    : QWidget(parent), m_partnerName(partnerName) {
  setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
  setAttribute(Qt::WA_TranslucentBackground);
  setAttribute(Qt::WA_DeleteOnClose); // Lifecycle: Destroy on close

  // Set consistent size (Compact)
  resize(320, 450);

  // Position relative to Main Window
  if (!initialPos.isNull()) {
    move(initialPos);
  }

  // Load background
  m_background = QPixmap(":/assets/login_bg.png");

  setupUI();

  // Initialize flash timer
  m_flashTimer = new QTimer(this);
  connect(m_flashTimer, &QTimer::timeout, this, [this]() {
    if (!m_flashing) {
      m_flashTimer->stop();
      m_overlayColor = Qt::transparent;
      update();
      return;
    }

    // Toggle color (Blink effect)
    m_flashCount++;
    if (m_flashCount % 2 == 0) {
      m_overlayColor = m_flashTargetColor;
    } else {
      m_overlayColor = Qt::transparent;
    }
    update();

    if (m_flashCount > 10) { // Blink 5 times (10 toggles)
      m_flashing = false;
      m_flashCount = 0;
    }
  });

  // Init Sound
  m_soundEffect = new QSoundEffect(this);
  m_soundEffect->setSource(QUrl("qrc:/assets/wizz.wav"));
  m_soundEffect->setVolume(1.0f);
  // Init Vibration
  m_vibrationTimer = new QTimer(this);
  connect(m_vibrationTimer, &QTimer::timeout, this, [this]() {
    if (m_vibrationSteps <= 0) {
      m_vibrationTimer->stop();
      move(m_originalPos); // Restore exact position
      return;
    }

    // Random "Violent" Offset
    int x = QRandomGenerator::global()->bounded(-8, 9); // -8 to +8
    int y = QRandomGenerator::global()->bounded(-8, 9);
    move(m_originalPos + QPoint(x, y));

    m_vibrationSteps--;
  });
}

ChatWindow::~ChatWindow() {}

void ChatWindow::closeEvent(QCloseEvent *event) {
  emit windowClosed(m_partnerName);
  QWidget::closeEvent(event);
}

// Dragging logic
void ChatWindow::paintEvent(QPaintEvent *event) {
  Q_UNUSED(event);
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

  // Rounded Path
  QPainterPath path;
  path.addRoundedRect(rect(), 20, 20);

  // Clip to rounded rect
  painter.setClipPath(path);

  if (!m_background.isNull()) {
    // Fill with scaled background
    painter.drawPixmap(
        rect(), m_background.scaled(size(), Qt::KeepAspectRatioByExpanding,
                                    Qt::SmoothTransformation));
  }
  // Overlay to ensure readability
  painter.fillRect(rect(),
                   QColor(255, 255, 255, 150)); // Semi-transparent white

  // Flash Overlay
  if (m_overlayColor.alpha() > 0) {
    painter.fillRect(rect(), m_overlayColor);
  }
}

// Dragging logic
void ChatWindow::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    m_dragPosition =
        event->globalPosition().toPoint() - frameGeometry().topLeft();
    event->accept();
  }
}

void ChatWindow::mouseMoveEvent(QMouseEvent *event) {
  if (event->buttons() & Qt::LeftButton) {
    move(event->globalPosition().toPoint() - m_dragPosition);
    event->accept();
  }
}

void ChatWindow::addMessage(const QString &sender, const QString &text,
                            bool isSelf) {
  Q_UNUSED(sender);
  QString time = QDateTime::currentDateTime().toString("HH:mm");
  QWidget *bubble = createMessageBubble(text, time, isSelf);
  m_chatLayout->addWidget(bubble);

  // Auto-scroll to bottom
  QTimer::singleShot(10, [this]() {
    m_chatArea->verticalScrollBar()->setValue(
        m_chatArea->verticalScrollBar()->maximum());
  });
}

void ChatWindow::flash(const QColor &color) {
  m_flashTargetColor = color;
  m_flashing = true;
  m_flashCount = 0;
  m_flashTimer->start(400);         // Blink every 400ms
  m_overlayColor = Qt::transparent; // Reset first
}

void ChatWindow::shake() {

  // Play Sound
  if (m_soundEffect) {
    m_soundEffect->play();
  }

  // Start Rigid Vibration
  m_originalPos = pos();
  m_vibrationSteps = 40; // ~600ms total
  m_vibrationTimer->start(
      15); // Update every 15ms (approx 60fps) for staccato feel

  // Trigger Red Flash
  flash(QColor(255, 0, 0, 120));
}

void ChatWindow::onSendClicked() {
  QString text = m_messageInput->text().trimmed();
  if (text.isEmpty())
    return;

  emit sendMessage(text);
  addMessage("Me", text, true);
  m_messageInput->clear();
}

void ChatWindow::onWizzClicked() {
  emit sendNudge();
  addMessage("Me", "You sent a Wizz!", true); // Local echo
}

void ChatWindow::onEmojiClicked() {
  QMenu *menu = new QMenu(this);
  menu->setStyleSheet("background: white; border-radius: 10px; border: 1px "
                      "solid #cbd5e0;");

  QWidget *container = new QWidget();
  QGridLayout *layout = new QGridLayout(container);
  layout->setSpacing(5);
  layout->setContentsMargins(10, 10, 10, 10);

  QStringList emojis = {"😀", "😂", "🥰", "😎", "🤔", "😴", "😭", "😡",
                        "👍", "👎", "❤️",  "🦋", "🚀", "⚡", "🎉", "🔥"};

  int row = 0, col = 0;
  for (const QString &emoji : emojis) {
    QPushButton *btn = new QPushButton(emoji);
    btn->setFixedSize(32, 32);
    btn->setFlat(true);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setStyleSheet("font-size: 20px; border: none;"); // Clean look

    connect(btn, &QPushButton::clicked, menu, [this, menu, emoji]() {
      m_messageInput->insert(emoji);
      menu->close();
    });

    layout->addWidget(btn, row, col);
    col++;
    if (col > 3) { // 4 columns
      col = 0;
      row++;
    }
  }

  QWidgetAction *action = new QWidgetAction(menu);
  action->setDefaultWidget(container);
  menu->addAction(action);

  // Show above the button
  // Ideally near the emoji button, but input works as anchor too.
  menu->exec(QCursor::pos());
  menu->deleteLater();
}

QWidget *ChatWindow::createMessageBubble(const QString &text,
                                         const QString &time, bool isSelf) {
  QWidget *container = new QWidget();
  QHBoxLayout *layout = new QHBoxLayout(container);
  layout->setContentsMargins(0, 5, 0, 5);

  QWidget *contentWidget = new QWidget();
  QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
  contentLayout->setContentsMargins(0, 0, 0, 0);
  contentLayout->setSpacing(2);

  QLabel *bubble = new QLabel(text);
  bubble->setWordWrap(true);
  bubble->setTextInteractionFlags(Qt::TextSelectableByMouse);
  bubble->setMaximumWidth(250); // Max width of bubble

  QLabel *timeLabel = new QLabel(time);
  timeLabel->setStyleSheet("color: #718096; font-size: 10px;");

  // Style based on sender
  if (isSelf) {
    layout->addStretch();
    layout->addWidget(contentWidget);

    contentLayout->addWidget(bubble, 0, Qt::AlignRight);
    contentLayout->addWidget(timeLabel, 0, Qt::AlignRight);

    bubble->setStyleSheet(R"(
        QLabel {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #4facfe, stop:1 #00f2fe);
            color: white;
            border-radius: 15px;
            padding: 10px;
            font-size: 13px;
        }
    )");
  } else {
    // Received
    layout->addWidget(contentWidget);
    layout->addStretch();

    contentLayout->addWidget(bubble, 0, Qt::AlignLeft);
    contentLayout->addWidget(timeLabel, 0, Qt::AlignLeft);

    // Glassy gray style
    bubble->setStyleSheet(R"(
        QLabel {
            background-color: rgba(255, 255, 255, 180);
            border: 1px solid rgba(255, 255, 255, 100);
            color: #2d3748;
            border-radius: 15px;
            padding: 10px;
            font-size: 13px;
        }
    )");
  }

  return container;
}

void ChatWindow::setupUI() {
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(15, 15, 15, 15);

  // Header
  QWidget *header = new QWidget(this);
  QHBoxLayout *headerLayout = new QHBoxLayout(header);
  headerLayout->setContentsMargins(0, 0, 0, 10);

  // Butterfly Icon
  QLabel *icon = new QLabel(header);
  QPixmap butterfly(":/assets/butterfly.png");
  if (!butterfly.isNull())
    icon->setPixmap(butterfly.scaled(24, 24, Qt::KeepAspectRatio,
                                     Qt::SmoothTransformation));
  icon->setStyleSheet("background: transparent;");

  // Title Column
  QWidget *titleGroup = new QWidget(header);
  QVBoxLayout *titleLayout = new QVBoxLayout(titleGroup);
  titleLayout->setContentsMargins(8, 0, 0, 0);
  titleLayout->setSpacing(0);

  QLabel *mainTitle = new QLabel("Wizz Mania", titleGroup);
  mainTitle->setStyleSheet("font-size: 14px; font-weight: bold; color: "
                           "#1a2530; background: transparent;");

  QLabel *subTitle = new QLabel(m_partnerName + " - Conversation", titleGroup);
  subTitle->setStyleSheet(
      "font-size: 11px; color: #4a5568; background: transparent;");

  titleLayout->addWidget(mainTitle);
  titleLayout->addWidget(subTitle);

  headerLayout->addWidget(icon);
  headerLayout->addWidget(titleGroup);
  headerLayout->addStretch();

  QPushButton *closeBtn = new QPushButton("X", header);
  closeBtn->setFixedSize(28, 28);
  closeBtn->setCursor(Qt::PointingHandCursor);
  closeBtn->setStyleSheet(R"(
      QPushButton {
          background: rgba(0, 0, 0, 20);
          color: #4a5568;
          border-radius: 14px;
          border: none;
          font-weight: bold;
      }
      QPushButton:hover {
          background: #e53e3e;
          color: white;
      }
  )");
  connect(closeBtn, &QPushButton::clicked, this, &QWidget::close);

  headerLayout->addWidget(closeBtn);

  mainLayout->addWidget(header);

  // Add spacer between header and chat if needed, but ScrollArea should take
  // it. Ensure header is fixed size
  header->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  header->setFixedHeight(50);

  // Chat Area
  m_chatArea = new QScrollArea(this);
  m_chatArea->setWidgetResizable(true);
  m_chatArea->setStyleSheet("background: transparent; border: none;");
  m_chatArea->viewport()->setStyleSheet("background: transparent;");
  m_chatArea->setSizePolicy(QSizePolicy::Expanding,
                            QSizePolicy::Expanding); // FORCE EXPAND

  m_chatContainer = new QWidget();
  m_chatContainer->setStyleSheet("background: transparent;");
  m_chatLayout = new QVBoxLayout(m_chatContainer);
  m_chatLayout->addStretch(); // Push messages down

  m_chatArea->setWidget(m_chatContainer);
  mainLayout->addWidget(m_chatArea);

  // Input Area
  QWidget *inputContainer = new QWidget(this);
  inputContainer->setSizePolicy(QSizePolicy::Preferred,
                                QSizePolicy::Fixed); // Fixed height
  // ...
  QHBoxLayout *inputLayout = new QHBoxLayout(inputContainer);
  inputLayout->setContentsMargins(0, 10, 0, 0);
  inputLayout->setSpacing(8);

  // Emoji Button (Placeholder)
  QPushButton *emojiBtn = new QPushButton("😊", inputContainer);
  emojiBtn->setFixedSize(36, 36);
  emojiBtn->setCursor(Qt::PointingHandCursor);
  emojiBtn->setStyleSheet(R"(
      QPushButton {
          background: rgba(255, 255, 255, 100);
          border-radius: 18px;
          border: 1px solid rgba(255, 255, 255, 200);
          font-size: 16px;
      }
      QPushButton:hover {
          background: rgba(255, 255, 255, 150);
      }
  )");
  connect(emojiBtn, &QPushButton::clicked, this, &ChatWindow::onEmojiClicked);

  m_messageInput = new QLineEdit(inputContainer);
  m_messageInput->setPlaceholderText("Type a message...");
  m_messageInput->setAttribute(Qt::WA_MacShowFocusRect,
                               false); // Remove native rect
  m_messageInput->setStyleSheet(R"(
      QLineEdit {
          background-color: rgba(255, 255, 255, 200);
          border: 1px solid rgba(255, 255, 255, 150);
          border-radius: 20px;
          padding: 8px 15px;
          font-size: 13px;
          min-height: 24px;
      }
      QLineEdit:focus {
          background-color: white;
          border: 2px solid #4facfe;
      }
  )");
  connect(m_messageInput, &QLineEdit::returnPressed, this,
          &ChatWindow::onSendClicked);

  // Wizz Button
  QPushButton *wizzBtn = new QPushButton(inputContainer);
  wizzBtn->setFixedSize(40, 40);
  wizzBtn->setCursor(Qt::PointingHandCursor);
  QPixmap wizzIcon(":/assets/wizz_icon.png");
  if (!wizzIcon.isNull()) {
    wizzBtn->setIcon(wizzIcon);
    wizzBtn->setIconSize(QSize(24, 24));
  } else {
    wizzBtn->setText("⚡");
  }
  wizzBtn->setStyleSheet(R"(
      QPushButton {
          background: rgba(255, 255, 255, 100);
          border-radius: 20px;
          border: 1px solid rgba(255, 255, 255, 200);
      }
      QPushButton:hover {
          background: rgba(255, 255, 255, 180);
           border: 1px solid #a1c4fd;
      }
  )");
  connect(wizzBtn, &QPushButton::clicked, this, &ChatWindow::onWizzClicked);

  QPushButton *sendBtn = new QPushButton("➤", inputContainer);
  sendBtn->setFixedSize(36, 36);
  sendBtn->setCursor(Qt::PointingHandCursor);
  sendBtn->setStyleSheet(R"(
      QPushButton {
          background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #4facfe, stop:1 #00f2fe);
          color: white;
          border-radius: 18px;
          font-size: 14px;
          border: none;
      }
      QPushButton:hover {
          background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #439ce0, stop:1 #00dce8);
      }
  )");
  connect(sendBtn, &QPushButton::clicked, this, &ChatWindow::onSendClicked);

  inputLayout->addWidget(emojiBtn);
  inputLayout->addWidget(m_messageInput);
  inputLayout->addWidget(wizzBtn);
  inputLayout->addWidget(sendBtn);

  QVBoxLayout *bottomLayout = new QVBoxLayout();
  bottomLayout->addWidget(inputContainer);

  // Typing Label
  QLabel *typingLabel = new QLabel("", this);
  typingLabel->setStyleSheet("font-size: 10px; color: #4a5568; font-style: "
                             "italic; margin-left: 50px;");
  bottomLayout->addWidget(typingLabel);

  mainLayout->addLayout(bottomLayout);
}
