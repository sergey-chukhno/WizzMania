#ifndef CHATWINDOW_H
#define CHATWINDOW_H

#include <QColor>
#include <QMouseEvent>
#include <QPixmap>
#include <QPoint>
#include <QSoundEffect>
#include <QTimer>
#include <QtWidgets/QFrame>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

#include "AudioManager.h"

class ChatWindow : public QWidget {
  Q_OBJECT
public:
  explicit ChatWindow(const QString &partnerName, const QPoint &initialPos,
                      QWidget *parent = nullptr);
  ~ChatWindow();

  void addMessage(const QString &sender, const QString &text, bool isSelf);
  void addVoiceMessage(const QString &sender, uint16_t duration,
                       const std::vector<uint8_t> &data, bool isSelf);
  void flash(const QColor &color);
  void shake();
  QString getPartnerName() const { return m_partnerName; }

signals:
  void sendMessage(const QString &text);
  void sendVoiceMessage(uint16_t duration, const std::vector<uint8_t> &data);
  void sendNudge();
  void windowClosed(const QString &partnerName);

protected:
  void closeEvent(QCloseEvent *event) override;
  void paintEvent(QPaintEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;

private slots:
  void onSendClicked();
  void onWizzClicked();
  void onEmojiClicked();
  void onMicClicked();

private:
  void setupUI();
  QWidget *createMessageBubble(const QString &text, const QString &time,
                               bool isSelf);
  QWidget *createVoiceBubble(uint16_t duration,
                             const std::vector<uint8_t> &data,
                             const QString &time, bool isSelf);

  QString m_partnerName;
  QPoint m_dragPosition;

  // UI Elements
  QScrollArea *m_chatArea;
  QWidget *m_chatContainer;
  QVBoxLayout *m_chatLayout;
  QLineEdit *m_messageInput;
  QPushButton *m_micBtn;
  QPixmap m_background;
  QLabel *m_typingLabel;

  // Typing Logic
  bool m_isTyping = false;
  QTimer *m_typingStopTimer;

  // Audio Logic
  AudioManager *m_audioManager;

  // Flash animation state
  bool m_flashing = false;
  int m_flashCount = 0;
  QTimer *m_flashTimer;
  QColor m_overlayColor = Qt::transparent;
  QColor m_flashTargetColor = QColor(255, 0, 0, 120);

  // Sound
  QSoundEffect *m_soundEffect;

  // Vibration
  QTimer *m_vibrationTimer;
  int m_vibrationSteps;
  QPoint m_originalPos;
};

#endif // CHATWINDOW_H
