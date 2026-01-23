#include "AuthWindow.h"
#include "MainWindow.h"
#include <QApplication>

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  AuthWindow *authWindow = new AuthWindow();
  MainWindow *mainWindow = nullptr;

  // Connect login success to show main window
  QObject::connect(authWindow, &AuthWindow::loginSuccessful, [&]() {
    // Get logged-in username (we'll need to pass this)
    QString username = authWindow->getLoggedInUsername();

    // Create and show main window
    mainWindow = new MainWindow(username);
    mainWindow->show();

    // Hide auth window
    authWindow->hide();
  });

  authWindow->show();

  return app.exec();
}
