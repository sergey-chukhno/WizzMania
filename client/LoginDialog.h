#pragma once

#include <QDialog>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

class LoginDialog : public QDialog {
  Q_OBJECT

public:
  explicit LoginDialog(QWidget *parent = nullptr);

private slots:
  void onLoginClicked();
  void onLoginSuccess();
  void onLoginFailed(const QString &reason);
  void onConnectionError(const QString &error);

private:
  void setupUI();
  void applyStyles();
  QPixmap processTransparentImage(const QString &path, int size);

  QLineEdit *m_hostInput;
  QLineEdit *m_portInput;
  QLineEdit *m_usernameInput;
  QLineEdit *m_passwordInput;
  QPushButton *m_loginButton;
  QLabel *m_statusLabel;
};
