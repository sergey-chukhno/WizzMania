#include "AuthWindow.h"
#include <QApplication>
#include <QMessageBox>

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  AuthWindow authWindow;

  // Connect login success to proceed to main window
  QObject::connect(&authWindow, &AuthWindow::loginSuccessful, [&]() {
    // Future: MainWindow w; w.show(); authWindow.hide();

    // Temporary feedback until Main Window is implemented
    QMessageBox::information(nullptr, "Welcome",
                             "Login Successful! Loading Chat...");
    QApplication::quit();
  });

  authWindow.show();

  return app.exec();
}
