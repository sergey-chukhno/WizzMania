#include "LoginDialog.h"
#include <QApplication>
#include <QMessageBox>

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  LoginDialog login;
  if (login.exec() == QDialog::Accepted) {
    // Login Success!
    // Future: MainWindow w; w.show();

    // Temporary feedback until Main Window is implemented
    QMessageBox::information(nullptr, "Welcome",
                             "Login Successful! Loading Chat...");
    return app.exec();
  } else {
    // User cancelled or closed window
    return 0;
  }
}
