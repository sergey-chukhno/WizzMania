#pragma once

#include <QProcess>
#include <QString>

class GameLauncher {
public:
  static bool launchGame(const QString &gameName, const QString &username = "");
  static QString resolveWorkingDir(const QString &gameFolder);

private:
  static QString resolveExecutablePath(const QString &baseName);
};
