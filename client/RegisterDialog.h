#pragma once

#include <QDialog>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

class RegisterDialog : public QDialog {
  Q_OBJECT

public:
  explicit RegisterDialog(QWidget *parent = nullptr);

signals:
  void backToLoginRequested();

private slots:
  void onRegisterClicked();
  void onRegisterSuccess();
  void onRegisterFailed(const QString &reason);
  void onConnectionError(const QString &error);

private:
  void setupUI();
  void applyStyles();
  QPixmap processTransparentImage(const QString &path, int size);

  // Default connection settings
  QString m_defaultHost = "127.0.0.1";
  quint16 m_defaultPort = 8080;

  QLineEdit *m_usernameInput;
  QLineEdit *m_passwordInput;
  QLineEdit *m_confirmPasswordInput;
  QPushButton *m_registerButton;
  QLabel *m_statusLabel;
};
