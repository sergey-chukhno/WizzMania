#pragma once

#include <QPixmap>
#include <QProcess>
#include <QString>

class GameLauncher {
public:
  static bool launchGame(const QString &gameName, const QString &username = "");
  static QProcess *launchTicTacToe(const QString &username,
                                   const QString &roomId, char symbol,
                                   const QString &opponent,
                                   const QPixmap &opponentAvatar = QPixmap());
  static QString resolveWorkingDir(const QString &gameFolder);

private:
  static QString resolveExecutablePath(const QString &baseName);
};
