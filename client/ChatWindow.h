#pragma once

#include <QFrame>
#include <QLineEdit>
#include <QScrollArea>
#include <QSoundEffect>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

class ChatWindow : public QWidget {
  Q_OBJECT
public:
  explicit ChatWindow(const QString &partnerName, const QPoint &initialPos,
                      QWidget *parent = nullptr);
  ~ChatWindow();

  void addMessage(const QString &sender, const QString &text, bool isSelf);
  void flash(); // Visual alert
  void shake(); // Nudge effect
  QString getPartnerName() const { return m_partnerName; }

signals:
  void sendMessage(const QString &text);
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

private:
  void setupUI();
  QWidget *createMessageBubble(const QString &text, bool isSelf);

  QString m_partnerName;
  QPoint m_dragPosition;

  // UI Elements
  QScrollArea *m_chatArea;
  QWidget *m_chatContainer;
  QVBoxLayout *m_chatLayout;
  QLineEdit *m_messageInput;
  QPixmap m_background;

  // Flash animation state
  bool m_flashing = false;
  int m_flashCount = 0;
  QTimer *m_flashTimer;
  QColor m_overlayColor = Qt::transparent;

  // Sound
  QSoundEffect *m_soundEffect;
};
