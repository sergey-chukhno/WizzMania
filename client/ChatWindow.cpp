#include "ChatWindow.h"
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QScrollBar>

ChatWindow::ChatWindow(const QString &partnerName, QWidget *parent)
    : QWidget(parent), m_partnerName(partnerName) {
  setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
  setAttribute(Qt::WA_TranslucentBackground);
  setAttribute(Qt::WA_DeleteOnClose); // Lifecycle: Destroy on close

  // Set consistent size
  resize(400, 550);

  // Load background
  m_background = QPixmap(":/assets/login_bg.png");

  setupUI();
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
  if (!m_background.isNull()) {
    // Fill with scaled background
    painter.drawPixmap(
        rect(), m_background.scaled(size(), Qt::KeepAspectRatioByExpanding,
                                    Qt::SmoothTransformation));
  }
  // Overlay to ensure readability
  painter.fillRect(rect(),
                   QColor(255, 255, 255, 150)); // Semi-transparent white
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
  QWidget *bubble = createMessageBubble(text, isSelf);
  m_chatLayout->addWidget(bubble);

  // Auto-scroll to bottom
  QTimer::singleShot(10, [this]() {
    m_chatArea->verticalScrollBar()->setValue(
        m_chatArea->verticalScrollBar()->maximum());
  });
}

void ChatWindow::onSendClicked() {
  QString text = m_messageInput->text().trimmed();
  if (text.isEmpty())
    return;

  emit sendMessage(text);
  addMessage("Me", text, true);
  m_messageInput->clear();
}

QWidget *ChatWindow::createMessageBubble(const QString &text, bool isSelf) {
  QWidget *container = new QWidget();
  QHBoxLayout *layout = new QHBoxLayout(container);
  layout->setContentsMargins(0, 5, 0, 5);

  QLabel *bubble = new QLabel(text);
  bubble->setWordWrap(true);
  bubble->setTextInteractionFlags(Qt::TextSelectableByMouse);
  bubble->setMaximumWidth(280); // Max width of bubble

  // Style based on sender
  if (isSelf) {
    layout->addStretch();
    layout->addWidget(bubble);
    bubble->setStyleSheet(R"(
        QLabel {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #4facfe, stop:1 #00f2fe);
            color: white;
            border-radius: 15px;
            padding: 10px;
            font-size: 14px;
        }
    )");
  } else {
    // Received
    layout->addWidget(bubble);
    layout->addStretch();
    // Glassy gray style
    bubble->setStyleSheet(R"(
        QLabel {
            background-color: rgba(255, 255, 255, 180);
            border: 1px solid rgba(255, 255, 255, 100);
            color: #2d3748;
            border-radius: 15px;
            padding: 10px;
            font-size: 14px;
        }
    )");
  }

  return container;
}

void ChatWindow::setupUI() {
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(20, 20, 20, 20);

  // Header
  QWidget *header = new QWidget(this);
  QHBoxLayout *headerLayout = new QHBoxLayout(header);
  headerLayout->setContentsMargins(0, 0, 0, 10);

  QLabel *nameLabel = new QLabel(m_partnerName, header);
  nameLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: "
                           "#1a2530; background: transparent;");

  QPushButton *closeBtn = new QPushButton("X", header);
  closeBtn->setFixedSize(30, 30);
  closeBtn->setCursor(Qt::PointingHandCursor);
  closeBtn->setStyleSheet(R"(
      QPushButton {
          background: rgba(0, 0, 0, 20);
          color: #4a5568;
          border-radius: 15px;
          border: none;
          font-weight: bold;
      }
      QPushButton:hover {
          background: #e53e3e;
          color: white;
      }
  )");
  connect(closeBtn, &QPushButton::clicked, this, &QWidget::close);

  headerLayout->addWidget(nameLabel);
  headerLayout->addStretch();
  headerLayout->addWidget(closeBtn);

  mainLayout->addWidget(header);

  // Chat Area
  m_chatArea = new QScrollArea(this);
  m_chatArea->setWidgetResizable(true);
  m_chatArea->setStyleSheet("background: transparent; border: none;");
  m_chatArea->viewport()->setStyleSheet("background: transparent;");

  m_chatContainer = new QWidget();
  m_chatContainer->setStyleSheet("background: transparent;");
  m_chatLayout = new QVBoxLayout(m_chatContainer);
  m_chatLayout->addStretch(); // Push messages down

  m_chatArea->setWidget(m_chatContainer);
  mainLayout->addWidget(m_chatArea);

  // Input Area
  QWidget *inputContainer = new QWidget(this);
  QHBoxLayout *inputLayout = new QHBoxLayout(inputContainer);
  inputLayout->setContentsMargins(0, 10, 0, 0);

  m_messageInput = new QLineEdit(inputContainer);
  m_messageInput->setPlaceholderText("Type a message...");
  m_messageInput->setStyleSheet(R"(
      QLineEdit {
          background-color: rgba(255, 255, 255, 150);
          border: 1px solid rgba(255, 255, 255, 200);
          border-radius: 20px;
          padding: 10px 15px;
          font-size: 14px;
      }
      QLineEdit:focus {
          background-color: white;
          border: 1px solid #4facfe;
      }
  )");
  connect(m_messageInput, &QLineEdit::returnPressed, this,
          &ChatWindow::onSendClicked);

  QPushButton *sendBtn = new QPushButton("➤", inputContainer);
  sendBtn->setFixedSize(40, 40);
  sendBtn->setCursor(Qt::PointingHandCursor);
  sendBtn->setStyleSheet(R"(
      QPushButton {
          background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #4facfe, stop:1 #00f2fe);
          color: white;
          border-radius: 20px;
          font-size: 16px;
          border: none;
      }
      QPushButton:hover {
          background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #439ce0, stop:1 #00dce8);
      }
  )");
  connect(sendBtn, &QPushButton::clicked, this, &ChatWindow::onSendClicked);

  inputLayout->addWidget(m_messageInput);
  inputLayout->addWidget(sendBtn);

  mainLayout->addWidget(inputContainer);
}
