#pragma once

#include <QDialog>
#include <QFormLayout>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

class LoginDialog : public QDialog {
  Q_OBJECT

public:
  explicit LoginDialog(QWidget *parent = nullptr);

  QString getHost() const { return m_defaultHost; }
  quint16 getPort() const { return m_defaultPort; }

private slots:
  void onLoginClicked();
  void onLoginSuccess();
  void onLoginFailed(const QString &reason);
  void onConnectionError(const QString &error);

private:
  void setupUI();
  void applyStyles();
  QPixmap processTransparentImage(const QString &path, int size);

  // Default connection settings (hidden)
  QString m_defaultHost = "127.0.0.1";
  quint16 m_defaultPort = 8080;

  QLineEdit *m_usernameInput;
  QLineEdit *m_passwordInput;
  QPushButton *m_loginButton;
  QLabel *m_statusLabel;
};
