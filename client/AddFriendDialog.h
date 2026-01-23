#pragma once

#include <QDialog>
#include <QLabel> // Added
#include <QLineEdit>
#include <QPixmap> // Added

class AddFriendDialog : public QDialog {
  Q_OBJECT
public:
  explicit AddFriendDialog(QWidget *parent = nullptr);
  QString getUsername() const;
  void showError(const QString &message);
  void clearInput();

signals:
  void addRequested(const QString &username);

private slots:
  void onAddClicked();

private:
  QLineEdit *m_usernameInput;
  QLabel *m_errorLabel;
  void setupUI();
};
