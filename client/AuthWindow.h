#pragma once

#include "../common/Packet.h"
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWidget>

/**
 * @brief AuthWindow - Single window containing both Login and Register views
 * Uses QStackedWidget for smooth navigation between views without close/reopen
 * effect
 */
class AuthWindow : public QWidget {
  Q_OBJECT

public:
  explicit AuthWindow(QWidget *parent = nullptr);

protected:
  void paintEvent(QPaintEvent *event) override;

signals:
  void loginSuccessful();

private slots:
  void onLoginClicked();
  void onLoginConnected();
  void onLoginPacketReceived(const wizz::Packet &pkt);
  void onLoginError(const QString &error);

  void onRegisterClicked();
  void onRegisterConnected();
  void onRegisterPacketReceived(const wizz::Packet &pkt);
  void onRegisterError(const QString &error);

private:
  void setupUI();
  QPixmap processTransparentImage(const QString &path, int size);
  QFrame *createLoginCard();
  QFrame *createRegisterCard();

  QPixmap m_backgroundPixmap;

  QStackedWidget *m_stack;
  QWidget *m_loginPage;
  QWidget *m_registerPage;

  // Login page widgets
  QLineEdit *m_loginUsername;
  QLineEdit *m_loginPassword;
  QPushButton *m_loginButton;
  QLabel *m_loginStatus;

  // Register page widgets
  QLineEdit *m_regUsername;
  QLineEdit *m_regPassword;
  QLineEdit *m_regConfirmPassword;
  QPushButton *m_registerButton;
  QLabel *m_regStatus;
};
