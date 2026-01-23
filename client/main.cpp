#include "LoginDialog.h"
#include "RegisterDialog.h"
#include <QApplication>
#include <QMessageBox>

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  while (true) {
    LoginDialog login;

    // Connect "Create account" button to open RegisterDialog
    QObject::connect(&login, &LoginDialog::createAccountRequested, [&]() {
      RegisterDialog reg;

      // If user clicks "Back to login" or successfully registers, return to
      // login
      QObject::connect(&reg, &RegisterDialog::backToLoginRequested, [&]() {
        // Just close register dialog, loop will show login again
      });

      reg.exec();
    });

    int result = login.exec();

    if (result == QDialog::Accepted) {
      // Login Success!
      // Future: MainWindow w; w.show();

      // Temporary feedback until Main Window is implemented
      QMessageBox::information(nullptr, "Welcome",
                               "Login Successful! Loading Chat...");
      return app.exec();
    } else if (result == QDialog::Rejected) {
      // Check if user wants to register (handled by signal above)
      // If truly cancelled, exit
      break;
    }
  }

  return 0;
}
