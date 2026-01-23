#pragma once

#include <QFrame>
#include <QLineEdit>
#include <QScrollArea>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

class ChatWindow : public QWidget {
  Q_OBJECT
public:
  explicit ChatWindow(const QString &partnerName, QWidget *parent = nullptr);
  ~ChatWindow();

  void addMessage(const QString &sender, const QString &text, bool isSelf);
  QString getPartnerName() const { return m_partnerName; }

signals:
  void sendMessage(const QString &text);
  void windowClosed(const QString &partnerName);

protected:
  void closeEvent(QCloseEvent *event) override;
  void paintEvent(QPaintEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;

private slots:
  void onSendClicked();

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
};
