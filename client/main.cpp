#include "ui/AuthWindow.h"
#include "ui/MainWindow.h"
#include "ui/theme/ThemeEngine.h"
#include "network/NetworkManager.h"
#include <QApplication>

int main(int argc, char *argv[]) {
  QApplication::setAttribute(Qt::AA_DontShowIconsInMenus, false);
  QApplication app(argc, argv);

  // Load embedded fonts (must be after QApplication)
  wizz::ui::ThemeEngine::loadFonts();

  // Apply global stylesheet
  app.setStyleSheet(wizz::ui::ThemeEngine::globalStyleSheet());

  AuthWindow *authWindow = new AuthWindow();
  MainWindow *mainWindow = nullptr;

  // Connect login success to show main window
  QObject::connect(authWindow, &AuthWindow::loginSuccessful, [&]() {
    // Get logged-in username (we'll need to pass this)
    QString username = authWindow->getLoggedInUsername();

    // Create and show main window (at same position)
    mainWindow = new MainWindow(username, authWindow->pos());
    mainWindow->show();

    // Hide auth window
    authWindow->hide();
  });

  authWindow->show();

  int ret = app.exec();

  // Clean up the background network thread safely
  NetworkManager::shutdown();

  return ret;
}
