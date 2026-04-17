#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>

class LoginDialog : public QDialog {
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);

signals:
    void createAccountRequested();

private slots:
    void onLoginClicked();
    void onLoginSuccess();
    void onLoginFailed(const QString &reason);
    void onConnectionError(const QString &error);

private:
    void setupUI();
    void applyStyles();
    QPixmap processTransparentImage(const QString &path, int size);

    QLineEdit *m_usernameInput;
    QLineEdit *m_passwordInput;
    QPushButton *m_loginButton;
    QLabel *m_statusLabel;

    QString m_defaultHost = "127.0.0.1";
    uint16_t m_defaultPort = 8080;
    bool m_isConnecting = false;
};
