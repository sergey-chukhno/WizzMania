#include "GameLauncher.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QProcess>

QString GameLauncher::resolveExecutablePath(const QString &baseName) {
  QString exeName = baseName;
#ifdef Q_OS_WIN
  exeName += ".exe";
#endif

  QDir appDir(QCoreApplication::applicationDirPath());
  QStringList searchPaths;

  // Walk up the directory tree to handle nested builds
  QDir currentDir = appDir;
  for (int i = 0; i < 5; ++i) {
    searchPaths << currentDir.absoluteFilePath("games/" + baseName + "/" +
                                               exeName);
    searchPaths << currentDir.absoluteFilePath("bin/" + exeName);
    searchPaths << currentDir.absoluteFilePath("games/" + baseName + "/Debug/" +
                                               exeName);
    searchPaths << currentDir.absoluteFilePath("games/" + baseName +
                                               "/Release/" + exeName);
    searchPaths << currentDir.absoluteFilePath("bin/Debug/" + exeName);
    searchPaths << currentDir.absoluteFilePath("bin/Release/" + exeName);

    if (!currentDir.cdUp())
      break;
  }

  for (const QString &path : searchPaths) {
    if (QFile::exists(path)) {
      return QDir::cleanPath(path);
    }
  }
  return "";
}

QString GameLauncher::resolveWorkingDir(const QString &gameFolder) {
  QDir dir(QCoreApplication::applicationDirPath());
  for (int i = 0; i < 6; ++i) {
    if (dir.exists("games/" + gameFolder + "/assets")) {
      return QDir::cleanPath(dir.absoluteFilePath("games/" + gameFolder));
    }
    if (!dir.cdUp())
      break;
  }
  return QCoreApplication::applicationDirPath();
}

bool GameLauncher::launchGame(const QString &gameName,
                              const QString &username) {
  QString exeName;
  QString folderName = gameName;
  if (gameName == "TileTwister") {
    exeName = gameName;
  } else if (gameName == "BrickBreaker" || gameName == "Cyberpunk") {
    exeName = "CyberpunkCannonShooter";
    folderName = "BrickBreaker";
  } else {
    exeName = gameName;
  }

  QString exePath = resolveExecutablePath(exeName);
  if (exePath.isEmpty()) {
    QMessageBox::critical(nullptr, "Launch Error",
                          "Could not find executable for " + gameName +
                              "!\nCheck your build paths.");
    return false;
  }

  QString workingDir = resolveWorkingDir(
      folderName); // Assumes gameFolder matching name or derived appropriately.

  QProcess *process = new QProcess();
  process->setWorkingDirectory(workingDir);
  QStringList args;
  if (!username.isEmpty()) {
    args << username;
  }
  process->start(exePath, args);

  if (!process->waitForStarted()) {
    QMessageBox::critical(nullptr, "Launch Error",
                          "Failed to start " + gameName + ":\n" +
                              process->errorString());
    delete process;
    return false;
  }

  // Release process to system so it runs independent of UI lifecycle
  // A fire-and-forget mechanism or handling should ideally be bound, but
  // keeping it simple like previous QProcess* new.
  QObject::connect(
      process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
      process, &QObject::deleteLater);

  return true;
}
