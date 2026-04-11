#pragma once

#include <QHash>
#include <QObject>
#include <QPixmap>
#include <QString>

class AvatarManager : public QObject {
  Q_OBJECT
public:
  static AvatarManager &instance();

  // Retrieves from cache, or returns generated initials avatar.
  // Also requests from server if not cached yet.
  QPixmap getAvatar(const QString &username, int size = 50);

  // Explicitly generate the default initials avatar
  QPixmap createAvatarWithInitials(const QString &name, int size);

signals:
  void avatarUpdated(const QString &username, const QPixmap &avatar);

private slots:
  void onNetworkAvatarReceived(const QString &username, const QByteArray &data);

private:
  explicit AvatarManager(QObject *parent = nullptr);
  ~AvatarManager();

  // Prevent copy
  AvatarManager(const AvatarManager &) = delete;
  AvatarManager &operator=(const AvatarManager &) = delete;

  QHash<QString, QPixmap> m_avatarCache;
};
