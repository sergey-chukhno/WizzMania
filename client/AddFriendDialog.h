#pragma once

#include <QDialog>
#include <QLineEdit>

class AddFriendDialog : public QDialog {
  Q_OBJECT
public:
  explicit AddFriendDialog(QWidget *parent = nullptr);
  QString getUsername() const;

private:
  QLineEdit *m_usernameInput;
  void setupUI();
};
