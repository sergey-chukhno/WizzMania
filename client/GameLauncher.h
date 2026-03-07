#pragma once

#include <QProcess>
#include <QString>

class GameLauncher {
public:
  static bool launchGame(const QString &gameName, const QString &username = "");
  static QProcess *launchTicTacToe(const QString &username,
                                   const QString &roomId, char symbol,
                                   const QString &opponent);
  static QString resolveWorkingDir(const QString &gameFolder);

private:
  static QString resolveExecutablePath(const QString &baseName);
};
