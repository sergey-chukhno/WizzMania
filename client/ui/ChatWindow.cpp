#include "ChatWindow.h"
#include "../network/NetworkManager.h"
#include "../logic/AvatarManager.h"
#include "theme/ThemeEngine.h"
#include "widgets/TitleBar.h"
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
#include <QWindow>

#include <QRandomGenerator>
#include <QUrl>
#include <memory>

ChatWindow::ChatWindow(const QString &partnerName, const QPoint &initialPos,
                       QWidget *parent)
    : QWidget(parent), m_partnerName(partnerName) {
  m_audioManager = new AudioManager(this);
  setWindowFlags(Qt::FramelessWindowHint | Qt::Window | Qt::WindowStaysOnTopHint);
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
    m_vibrationSteps--;
  });

  // Init Typing Logic
  m_typingStopTimer = new QTimer(this);
  m_typingStopTimer->setSingleShot(true);

  // Connect Stop Timer
  connect(m_typingStopTimer, &QTimer::timeout, this, [this]() {
    m_isTyping = false;
    NetworkManager::instance().sendTypingPacket(m_partnerName, false);
  });

  // Listen for Incoming Typing Indicators
  connect(&NetworkManager::instance(), &NetworkManager::userTyping, this,
          [this](const QString &sender, bool isTyping) {
            if (sender == m_partnerName) {
              m_typingContainer->setVisible(isTyping);
              if (isTyping) {
                m_typingBubble->startAnimation();
              } else {
                m_typingBubble->stopAnimation();
              }
            }
          });

  // Dynamic Avatar Updates
  connect(&::AvatarManager::instance(), &::AvatarManager::avatarUpdated, this,
          [this](const QString &user, const QPixmap &avatar) {
            if (user == m_partnerName && m_headerAvatar) {
                m_headerAvatar->setPixmap(avatar.scaled(32, 32, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
                
                // Circle Mask
                QPixmap circular(32, 32);
                circular.fill(Qt::transparent);
                QPainter painter(&circular);
                painter.setRenderHint(QPainter::Antialiasing);
                QPainterPath path;
                path.addEllipse(0, 0, 32, 32);
                painter.setClipPath(path);
                painter.drawPixmap(0, 0, avatar.scaled(32, 32, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
                m_headerAvatar->setPixmap(circular);
            }
          });
  
  // Initial Avatar Fetch & Immediate Apply
  QPixmap cachedAvatar = ::AvatarManager::instance().getAvatar(m_partnerName, 32);
  if (!cachedAvatar.isNull() && m_headerAvatar) {
      // Circle Mask
      QPixmap circular(32, 32);
      circular.fill(Qt::transparent);
      QPainter painter(&circular);
      painter.setRenderHint(QPainter::Antialiasing);
      QPainterPath path;
      path.addEllipse(0, 0, 32, 32);
      painter.setClipPath(path);
      painter.drawPixmap(0, 0, cachedAvatar.scaled(32, 32, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
      m_headerAvatar->setPixmap(circular);
  }
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

  // Rounded clip matching MainWindow
  QPainterPath path;
  path.addRoundedRect(rect(), 30, 30);
  painter.setClipPath(path);

  // 1. Draw Unified Background Pixmap
  if (!m_background.isNull()) {
    painter.drawPixmap(rect(), m_background.scaled(
                                   size(), Qt::KeepAspectRatioByExpanding,
                                   Qt::SmoothTransformation));
  }
  
  // 2. Subtle glass border
  painter.setPen(QPen(QColor(255, 255, 255, 40), 1));
  painter.drawPath(path);

  // Flash Overlay
  if (m_overlayColor.alpha() > 0) {
    painter.fillRect(rect(), m_overlayColor);
  }
}

// Dragging logic
ChatWindow::ResizeEdge ChatWindow::edgeAtPoint(const QPoint& p) const {
  const int margin = 6;
  bool left = p.x() < margin;
  bool right = p.x() > width() - margin;
  bool top = p.y() < margin;
  bool bottom = p.y() > height() - margin;

  if (top && left) return ResizeEdge::TopLeft;
  if (top && right) return ResizeEdge::TopRight;
  if (bottom && left) return ResizeEdge::BottomLeft;
  if (bottom && right) return ResizeEdge::BottomRight;
  if (left) return ResizeEdge::Left;
  if (right) return ResizeEdge::Right;
  if (top) return ResizeEdge::Top;
  if (bottom) return ResizeEdge::Bottom;

  return ResizeEdge::None;
}

void ChatWindow::updateCursor(const QPoint& p) {
  if (m_isResizing) return;
  ResizeEdge edge = edgeAtPoint(p);
  switch (edge) {
      case ResizeEdge::Left:
      case ResizeEdge::Right: setCursor(Qt::SizeHorCursor); break;
      case ResizeEdge::Top:
      case ResizeEdge::Bottom: setCursor(Qt::SizeVerCursor); break;
      case ResizeEdge::TopLeft:
      case ResizeEdge::BottomRight: setCursor(Qt::SizeFDiagCursor); break;
      case ResizeEdge::TopRight:
      case ResizeEdge::BottomLeft: setCursor(Qt::SizeBDiagCursor); break;
      default: setCursor(Qt::ArrowCursor); break;
  }
}

void ChatWindow::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
      m_currentEdge = edgeAtPoint(event->pos());
      if (m_currentEdge != ResizeEdge::None) {
          m_isResizing = true;
          m_resizeStartPos = event->globalPos();
          m_resizeStartGeometry = geometry();
          event->accept();
          return;
      }
      
      // Allow window dragging from title bar area handled by TitleBar itself
      // Fallback native drag
      if (window()->windowHandle()) {
          window()->windowHandle()->startSystemMove();
      }
  }
  QWidget::mousePressEvent(event);
}

void ChatWindow::mouseMoveEvent(QMouseEvent *event) {
  if (m_isResizing) {
      QPoint delta = event->globalPos() - m_resizeStartPos;
      QRect g = m_resizeStartGeometry;

      switch (m_currentEdge) {
          case ResizeEdge::Left: g.setLeft(g.left() + delta.x()); break;
          case ResizeEdge::Right: g.setRight(g.right() + delta.x()); break;
          case ResizeEdge::Top: g.setTop(g.top() + delta.y()); break;
          case ResizeEdge::Bottom: g.setBottom(g.bottom() + delta.y()); break;
          case ResizeEdge::TopLeft: g.setTopLeft(g.topLeft() + delta); break;
          case ResizeEdge::TopRight: g.setTopRight(g.topRight() + QPoint(delta.x(), delta.y())); break;
          case ResizeEdge::BottomLeft: g.setBottomLeft(g.bottomLeft() + QPoint(delta.x(), delta.y())); break;
          case ResizeEdge::BottomRight: g.setBottomRight(g.bottomRight() + delta); break;
          default: break;
      }
      
      g.setWidth(qMax(g.width(), minimumWidth()));
      g.setHeight(qMax(g.height(), minimumHeight()));
      setGeometry(g);
      event->accept();
      return;
  }
  
  updateCursor(event->pos());
  QWidget::mouseMoveEvent(event);
}

void ChatWindow::mouseReleaseEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton && m_isResizing) {
      m_isResizing = false;
      m_currentEdge = ResizeEdge::None;
      updateCursor(event->pos());
      event->accept();
      return;
  }
  QWidget::mouseReleaseEvent(event);
}


void ChatWindow::addVoiceMessage(const QString &sender, uint16_t duration,
                                 const std::vector<uint8_t> &data,
                                 bool isSelf) {
  Q_UNUSED(sender);
  QString time = QDateTime::currentDateTime().toString("HH:mm");
  QWidget *bubble = createVoiceBubble(duration, data, time, isSelf);
  m_chatLayout->addWidget(bubble);

  QTimer::singleShot(10, [this]() {
    m_chatArea->verticalScrollBar()->setValue(
        m_chatArea->verticalScrollBar()->maximum());
  });
}

void ChatWindow::addGameInvite(const QString &sender, const QString &gameName) {
  Q_UNUSED(sender);
  QWidget *bubble = createInviteBubble(sender, gameName);
  m_chatLayout->addWidget(bubble);

  QTimer::singleShot(10, [this]() {
    m_chatArea->verticalScrollBar()->setValue(
        m_chatArea->verticalScrollBar()->maximum());
  });
}

void ChatWindow::addRichMessage(const QString &sender, const QString &category, const QString &text, bool isSelf) {
  Q_UNUSED(sender);
  QString time = QDateTime::currentDateTime().toString("HH:mm");
  
  QWidget *container = new QWidget();
  QHBoxLayout *layout = new QHBoxLayout(container);
  layout->setContentsMargins(0, 5, 0, 5);

  QWidget *contentWidget = new QWidget();
  QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
  contentLayout->setContentsMargins(0, 0, 0, 0);
  contentLayout->setSpacing(2);

  // Main Card
  QWidget* card = new QWidget();
  QVBoxLayout* cardLayout = new QVBoxLayout(card);
  cardLayout->setContentsMargins(12, 12, 12, 12);
  cardLayout->setSpacing(4);
  
  // Icon and Title
  QLabel* titleLbl = new QLabel(QString("<b>%1 Activity</b>").arg(category));
  titleLbl->setStyleSheet(QString("color: %1;").arg(wizz::ui::ThemeEngine::accent().name()));
  
  QLabel* textLbl = new QLabel(text);
  textLbl->setWordWrap(true);
  textLbl->setStyleSheet(QString("color: %1; font-size: 13px;").arg(wizz::ui::ThemeEngine::onSurface().name()));
  
  cardLayout->addWidget(titleLbl);
  cardLayout->addWidget(textLbl);
  
  card->setStyleSheet(QString(R"(
      QWidget {
          background-color: %1;
          border: 1px solid %2;
          border-radius: %3px;
      }
  )").arg(wizz::ui::ThemeEngine::surfaceHigh().name())
     .arg(wizz::ui::ThemeEngine::accent().name())
     .arg(wizz::ui::ThemeEngine::rMd));

  QLabel *timeLabel = new QLabel(time);
  timeLabel->setStyleSheet("color: #718096; font-size: 10px;");

  if (isSelf) {
    layout->addStretch();
    layout->addWidget(contentWidget);
    contentLayout->addWidget(card, 0, Qt::AlignRight);
    contentLayout->addWidget(timeLabel, 0, Qt::AlignRight);
  } else {
    layout->addWidget(contentWidget);
    layout->addStretch();
    contentLayout->addWidget(card, 0, Qt::AlignLeft);
    contentLayout->addWidget(timeLabel, 0, Qt::AlignLeft);
  }

  m_chatLayout->addWidget(container);
  QTimer::singleShot(10, [this]() {
    m_chatArea->verticalScrollBar()->setValue(m_chatArea->verticalScrollBar()->maximum());
  });
}

void ChatWindow::addMessage(const QString &sender, const QString &text,
                            bool isSelf) {
  QString time = QDateTime::currentDateTime().toString("HH:mm");
  QString displayName = isSelf ? "You" : sender;
  QWidget *bubble = createMessageBubble(displayName, text, time, isSelf);
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

  // Stop Typing immediately
  if (m_isTyping) {
    m_isTyping = false;
    m_typingStopTimer->stop();
    NetworkManager::instance().sendTypingPacket(m_partnerName, false);
  }
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
  menu->exec(QCursor::pos());
  menu->deleteLater();
}

void ChatWindow::onMicClicked() {
  if (!m_audioManager->isRecording()) {
    if (m_audioManager->startRecording()) {
      m_micBtn->setText("⏹");
      m_micBtn->setStyleSheet(
          "background-color: #e53e3e; color: white; border-radius: 18px; "
          "font-size: 16px; border: none;");
    }
  } else {
    uint16_t duration = 0;
    auto data = m_audioManager->stopRecording(duration);
    m_micBtn->setText("🎤");
    // Reset Style
    m_micBtn->setStyleSheet(R"(
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

    if (!data.empty()) {
      emit sendVoiceMessage(duration, data);
      addVoiceMessage("Me", duration, data, true);
    }
  }
}

QWidget *ChatWindow::createMessageBubble(const QString &sender, const QString &text,
                                         const QString &time, bool isSelf) {
  QWidget *container = new QWidget();
  QHBoxLayout *layout = new QHBoxLayout(container);
  layout->setContentsMargins(0, 5, 0, 5);

  QWidget *contentWidget = new QWidget();
  QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
  contentLayout->setContentsMargins(0, 0, 0, 0);
  contentLayout->setSpacing(2);

  // Sender Attribution ("Bob said:")
  QLabel *attribution = new QLabel(QString("%1 said:").arg(sender));
  attribution->setStyleSheet("color: rgba(255, 255, 255, 150); font-size: 10px; font-weight: 600; margin-bottom: 2px;");

  QLabel *bubble = new QLabel(text);
  bubble->setWordWrap(true);
  bubble->setTextInteractionFlags(Qt::TextSelectableByMouse);
  bubble->setMaximumWidth(250);

  QLabel *timeLabel = new QLabel(time);
  timeLabel->setStyleSheet("color: rgba(255, 255, 255, 120); font-size: 10px;");

  if (isSelf) {
    layout->addStretch();
    layout->addWidget(contentWidget);
    contentLayout->addWidget(attribution, 0, Qt::AlignRight);
    contentLayout->addWidget(bubble, 0, Qt::AlignRight);
    contentLayout->addWidget(timeLabel, 0, Qt::AlignRight);

    bubble->setStyleSheet(QString(R"(
        QLabel {
            background-color: %1;
            color: white;
            border-radius: 18px;
            padding: 10px 14px;
            font-size: 13px;
        }
    )").arg(wizz::ui::ThemeEngine::accent().name()));
  } else {
    layout->addWidget(contentWidget);
    layout->addStretch();
    contentLayout->addWidget(attribution, 0, Qt::AlignLeft);
    contentLayout->addWidget(bubble, 0, Qt::AlignLeft);
    contentLayout->addWidget(timeLabel, 0, Qt::AlignLeft);

    bubble->setStyleSheet(R"(
        QLabel {
            background-color: rgba(255, 255, 255, 30);
            border: 1px solid rgba(255, 255, 255, 20);
            color: white;
            border-radius: 18px;
            padding: 10px 14px;
            font-size: 13px;
        }
    )");
  }
  return container;
}

QWidget *ChatWindow::createVoiceBubble(uint16_t duration,
                                       const std::vector<uint8_t> &data,
                                       const QString &time, bool isSelf) {
  QWidget *container = new QWidget();
  QHBoxLayout *layout = new QHBoxLayout(container);
  layout->setContentsMargins(0, 5, 0, 5);

  QWidget *contentWidget = new QWidget();
  QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
  contentLayout->setContentsMargins(0, 0, 0, 0);
  contentLayout->setSpacing(2);

  QPushButton *playBtn =
      new QPushButton("▶ " + QString::number(duration) + "s");
  playBtn->setFixedWidth(100);
  playBtn->setCursor(Qt::PointingHandCursor);

  // Copy data for the lambda capture
  std::vector<uint8_t> audioData = data;

  // Connection for click
  connect(playBtn, &QPushButton::clicked, this,
          [this, audioData, playBtn, duration]() {
            auto connStop = std::make_shared<QMetaObject::Connection>();
            *connStop = connect(m_audioManager, &AudioManager::playbackStopped,
                                this, [playBtn, duration, connStop]() {
                                  playBtn->setText(
                                      "▶ " + QString::number(duration) + "s");
                                  disconnect(*connStop);
                                });

            auto connStart = std::make_shared<QMetaObject::Connection>();
            *connStart = connect(m_audioManager, &AudioManager::playbackStarted,
                                 this, [playBtn, connStart]() {
                                   playBtn->setText("🔊 Playing...");
                                   disconnect(*connStart);
                                 });

            m_audioManager->playAudio(audioData);
          });

  QLabel *timeLabel = new QLabel(time);
  timeLabel->setStyleSheet("color: #718096; font-size: 10px;");

  if (isSelf) {
    layout->addStretch();
    layout->addWidget(contentWidget);
    contentLayout->addWidget(playBtn, 0, Qt::AlignRight);
    contentLayout->addWidget(timeLabel, 0, Qt::AlignRight);
    playBtn->setStyleSheet(R"(
          QPushButton {
              background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #4facfe, stop:1 #00f2fe);
              color: white; border-radius: 18px; padding: 5px; border: none; font-weight: bold;
              text-align: left; padding-left: 15px;
              min-height: 36px;
          }
      )");
  } else {
    layout->addWidget(contentWidget);
    layout->addStretch();
    contentLayout->addWidget(playBtn, 0, Qt::AlignLeft);
    contentLayout->addWidget(timeLabel, 0, Qt::AlignLeft);
    playBtn->setStyleSheet(R"(
          QPushButton {
              background-color: rgba(255, 255, 255, 180);
              border: 1px solid rgba(255, 255, 255, 100);
              color: #2d3748; border-radius: 18px; padding: 5px; font-weight: bold;
              text-align: left; padding-left: 15px;
              min-height: 36px;
          }
      )");
  }
  return container;
}

QWidget *ChatWindow::createInviteBubble(const QString &sender,
                                        const QString &gameName) {
  QWidget *container = new QWidget();
  QHBoxLayout *layout = new QHBoxLayout(container);
  layout->setContentsMargins(0, 5, 0, 5);

  QWidget *contentWidget = new QWidget();
  QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
  contentLayout->setContentsMargins(0, 0, 0, 0);
  contentLayout->setSpacing(5);

  // Styling for the glassmorphism card
  contentWidget->setStyleSheet(QString(R"(
      QWidget {
          background-color: %1;
          border: 1px solid %2;
          border-radius: %3px;
      }
  )").arg(wizz::ui::ThemeEngine::surfaceHigh().name())
     .arg(wizz::ui::ThemeEngine::accent().name())
     .arg(wizz::ui::ThemeEngine::rMd));

  QLabel *titleLabel =
      new QLabel("⚡ " + sender + " challenged you to " + gameName + "! ⚡");
  titleLabel->setStyleSheet(QString(
      "color: %1; font-weight: bold; font-size: 13px; background: "
      "transparent; border: none; padding: 5px;").arg(wizz::ui::ThemeEngine::onSurface().name()));
  titleLabel->setAlignment(Qt::AlignCenter);
  titleLabel->setWordWrap(true);
  titleLabel->setMinimumWidth(220);
  titleLabel->setMinimumHeight(44);

  QHBoxLayout *btnLayout = new QHBoxLayout();
  QPushButton *acceptBtn = new QPushButton("Accept");
  QPushButton *declineBtn = new QPushButton("Decline");

  QString btnStyle = R"(
      QPushButton {
          color: white; 
          border-radius: 6px; 
          padding: 8px; 
          font-weight: bold;
          border: none;
      }
  )";

  acceptBtn->setStyleSheet(
      btnStyle +
      QString("QPushButton { background-color: %1; } ").arg(wizz::ui::ThemeEngine::accent().name()) +
      QString("QPushButton:hover { background-color: %1; }").arg(QColor(wizz::ui::ThemeEngine::accent().name()).lighter(115).name()));
  declineBtn->setStyleSheet(
      btnStyle +
      "QPushButton { background-color: rgba(200, 50, 50, 200); } "
      "QPushButton:hover { background-color: rgba(255, 80, 80, 255); }");

  acceptBtn->setCursor(Qt::PointingHandCursor);
  declineBtn->setCursor(Qt::PointingHandCursor);

  btnLayout->addWidget(acceptBtn);
  btnLayout->addWidget(declineBtn);

  contentLayout->addWidget(titleLabel);
  contentLayout->addLayout(btnLayout);

  connect(acceptBtn, &QPushButton::clicked, this,
          [this, sender, gameName, contentWidget]() {
            // Send Accept
            NetworkManager::instance().sendGameInviteResponse(sender, gameName,
                                                               true);
            // Disable the card to prevent double-clicking
            contentWidget->setEnabled(false);
            contentWidget->setStyleSheet(contentWidget->styleSheet() +
                                         " QWidget { opacity: 0.5; }");
          });

  connect(declineBtn, &QPushButton::clicked, this,
          [this, sender, gameName, contentWidget]() {
            // Send Decline
            NetworkManager::instance().sendGameInviteResponse(sender, gameName,
                                                               false);
            contentWidget->setEnabled(false);
            contentWidget->setStyleSheet(contentWidget->styleSheet() +
                                         " QWidget { opacity: 0.5; }");
          });

  // Always align left (incoming)
  layout->addWidget(contentWidget);
  layout->addStretch();

  return container;
}

void ChatWindow::resizeEvent(QResizeEvent *event) {
  QWidget::resizeEvent(event);
  
  // Apply Rounded Mask (Sync with paintEvent)
  QPainterPath path;
  path.addRoundedRect(rect(), 30, 30);
  this->setMask(path.toFillPolygon().toPolygon());
}

void ChatWindow::setupUI() {
  setMouseTracking(true); // Required for frameless edge detection
  setMinimumSize(320, 450);

  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(0);

  // Premium Integrated Header
  auto* headerWidget = new QWidget(this);
  headerWidget->setFixedHeight(60);
  auto* headerLayout = new QHBoxLayout(headerWidget);
  headerLayout->setContentsMargins(20, 10, 20, 0);
  
  m_headerAvatar = new QLabel();
  m_headerAvatar->setFixedSize(32, 32);
  m_headerAvatar->setStyleSheet("background: rgba(255,255,255,10); border-radius: 16px;");
  
  auto* nameLabel = new QLabel(m_partnerName);
  nameLabel->setStyleSheet("color: white; font-size: 15px; font-weight: 800;");
  
  // WizzMania Pro Branding (Right-aligned next to name)
  auto* brandContainer = new QWidget();
  auto* brandLayout = new QHBoxLayout(brandContainer);
  brandLayout->setContentsMargins(0, 0, 0, 0);
  brandLayout->setSpacing(6);
  
  QLabel* logoLabel = new QLabel();
  logoLabel->setFixedSize(16, 16);
  logoLabel->setPixmap(QPixmap(":/assets/butterfly.png").scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation));
  
  QLabel* proLabel = new QLabel("WizzMania Pro");
  proLabel->setStyleSheet("color: rgba(255, 255, 255, 120); font-size: 10px; font-weight: 700; letter-spacing: 1px;");
  
  brandLayout->addWidget(logoLabel);
  brandLayout->addWidget(proLabel);
  
  auto* closeBtn = new QPushButton("✕");
  closeBtn->setFixedSize(28, 28);
  closeBtn->setCursor(Qt::PointingHandCursor);
  closeBtn->setStyleSheet(R"(
      QPushButton {
          background: rgba(255, 255, 255, 15);
          color: white;
          border-radius: 14px;
          font-weight: bold;
      }
      QPushButton:hover { background: rgba(255, 255, 255, 30); }
  )");
  connect(closeBtn, &QPushButton::clicked, this, &QWidget::close);

  headerLayout->addWidget(m_headerAvatar);
  headerLayout->addWidget(nameLabel);
  headerLayout->addStretch();
  headerLayout->addWidget(brandContainer);
  headerLayout->addSpacing(15);
  headerLayout->addWidget(closeBtn);
  mainLayout->addWidget(headerWidget);

  // Content Area
  QWidget *contentWidget = new QWidget(this);
  QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
  contentLayout->setContentsMargins(16, 0, 16, 12);
  contentLayout->setSpacing(0);
  mainLayout->addWidget(contentWidget, 1);

  // Chat Area
  m_chatArea = new QScrollArea(this);
  m_chatArea->setWidgetResizable(true);
  m_chatArea->setStyleSheet("background: transparent; border: none;");
  m_chatArea->viewport()->setStyleSheet("background: transparent;");
  m_chatArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding); 

  m_chatContainer = new QWidget();
  m_chatContainer->setStyleSheet("background: transparent;");
  m_chatLayout = new QVBoxLayout(m_chatContainer);
  m_chatLayout->addStretch(); // Push messages down

  m_chatArea->setWidget(m_chatContainer);
  contentLayout->addWidget(m_chatArea);

  // Sticky Typing Indicator (In-Chat)
  m_typingContainer = new QWidget(this);
  auto* typingLayout = new QHBoxLayout(m_typingContainer);
  typingLayout->setContentsMargins(15, 5, 15, 5);
  typingLayout->setSpacing(8);
  
  m_typingBubble = new wizz::ui::TypingBubble(this);
  m_typingBubble->setFixedSize(45, 25);
  
  m_typingLabel = new QLabel(QString("%1 is typing...").arg(m_partnerName));
  m_typingLabel->setStyleSheet("color: rgba(255, 255, 255, 180); font-size: 11px; font-style: italic;");
  
  typingLayout->addWidget(m_typingBubble);
  typingLayout->addWidget(m_typingLabel);
  typingLayout->addStretch();
  m_typingContainer->hide(); // Hidden by default
  contentLayout->addWidget(m_typingContainer);

  // Input Area
  QWidget *inputContainer = new QWidget(this);
  inputContainer->setSizePolicy(QSizePolicy::Preferred,
                                QSizePolicy::Fixed); 
  QHBoxLayout *inputLayout = new QHBoxLayout(inputContainer);
  inputLayout->setContentsMargins(0, 10, 0, 0);
  inputLayout->setSpacing(8);

  // Emoji Button
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
  m_messageInput->setPlaceholderText("Message...");
  m_messageInput->setAttribute(Qt::WA_MacShowFocusRect, false); 
  m_messageInput->setStyleSheet(QString(R"(
      QLineEdit {
          background-color: %1;
          border: 1px solid rgba(255, 255, 255, 15);
          border-radius: 18px;
          padding: 8px 15px;
          font-size: 13px;
          color: %2; 
          min-height: 24px;
      }
      QLineEdit:focus {
          border: 1px solid %3;
      }
  )").arg(wizz::ui::ThemeEngine::surfaceHigh().name())
     .arg(wizz::ui::ThemeEngine::onSurface().name())
     .arg(wizz::ui::ThemeEngine::accent().name()));
  connect(m_messageInput, &QLineEdit::returnPressed, this,
          &ChatWindow::onSendClicked);

  // Trigger Typing Start
  connect(m_messageInput, &QLineEdit::textChanged, this,
          [this](const QString &text) {
            if (text.isEmpty()) {
              return;
            }

            if (!m_isTyping) {
              m_isTyping = true;
              NetworkManager::instance().sendTypingPacket(m_partnerName, true);
            }
            // Restart timeout (debounce)
            m_typingStopTimer->start(3000);
          });

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
  sendBtn->setStyleSheet(QString(R"(
      QPushButton {
          background: %1;
          color: %2;
          border-radius: 18px;
          font-size: 14px;
          border: none;
      }
      QPushButton:hover {
          background: %3;
      }
  )").arg(wizz::ui::ThemeEngine::accent().name())
     .arg(wizz::ui::ThemeEngine::surface().name())
     .arg(QColor(wizz::ui::ThemeEngine::accent().name()).lighter(115).name()));
  connect(sendBtn, &QPushButton::clicked, this, &ChatWindow::onSendClicked);

  // Mic Button
  m_micBtn = new QPushButton("🎤", inputContainer);
  m_micBtn->setFixedSize(36, 36);
  m_micBtn->setCursor(Qt::PointingHandCursor);
  m_micBtn->setStyleSheet(R"(
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
  connect(m_micBtn, &QPushButton::clicked, this, &ChatWindow::onMicClicked);

  inputLayout->addWidget(m_micBtn);
  inputLayout->addWidget(emojiBtn);
  inputLayout->addWidget(m_messageInput);
  inputLayout->addWidget(wizzBtn);
  inputLayout->addWidget(sendBtn);

  contentLayout->addWidget(inputContainer);
}
