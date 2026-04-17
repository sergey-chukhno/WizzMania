#pragma once

#include <QDialog>
#include <QLabel> 
#include <QLineEdit>
#include <QPixmap> 

class AddFriendDialog : public QDialog {
  Q_OBJECT
public:
  explicit AddFriendDialog(QWidget *parent = nullptr);
  QString getUsername() const;
  void showError(const QString &message);
  void clearInput();
  void setLoading(bool loading);

signals:
  void addRequested(const QString &username);

private slots:
  void onAddClicked();
  void onWatchdogTimeout();

private:
  QLineEdit *m_usernameInput;
  QLabel *m_errorLabel;
  class QPushButton *m_addBtn;
  class QPushButton *m_cancelBtn;
  class QTimer *m_watchdog;
  void setupUI();
};
