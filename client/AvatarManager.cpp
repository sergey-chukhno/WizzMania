#include "AvatarManager.h"
#include "NetworkManager.h"
#include <QFont>
#include <QPainter>
#include <QPainterPath>

AvatarManager &AvatarManager::instance() {
  static AvatarManager _instance;
  return _instance;
}

AvatarManager::AvatarManager(QObject *parent) : QObject(parent) {
  // Listen to Network Manager for incoming avatars
  connect(&NetworkManager::instance(), &NetworkManager::avatarReceived, this,
          &AvatarManager::onNetworkAvatarReceived);
}

AvatarManager::~AvatarManager() {}

QPixmap AvatarManager::getAvatar(const QString &username, int size) {
  if (m_avatarCache.contains(username)) {
    return m_avatarCache[username];
  }

  // If not in cache, request it from the server.
  // Assuming NetworkManager sends the packet asynchronously.
  NetworkManager::instance().requestAvatar(username);

  // Return a generated placeholder immediately
  return createAvatarWithInitials(username, size);
}

QPixmap AvatarManager::createAvatarWithInitials(const QString &name, int size) {
  QPixmap avatar(size, size);
  avatar.fill(Qt::transparent);

  QPainter painter(&avatar);
  painter.setRenderHint(QPainter::Antialiasing);

  // Generate color from name hash
  uint hash = qHash(name);
  QColor bgColor = QColor::fromHsl(hash % 360, 150, 120);

  // Draw circle
  painter.setBrush(bgColor);
  painter.setPen(Qt::NoPen);
  painter.drawEllipse(0, 0, size, size);

  // Draw initials
  QString initials;
  QStringList parts = name.split('_');
  if (parts.isEmpty())
    parts = name.split(' ');
  for (const QString &part : parts) {
    if (!part.isEmpty())
      initials += part[0].toUpper();
    if (initials.length() >= 2)
      break;
  }
  if (initials.isEmpty() && !name.isEmpty()) {
    initials = name.left(2).toUpper();
  }

  painter.setPen(Qt::white);
  QFont font("SF Pro Display", size / 3, QFont::Bold);
  painter.setFont(font);
  painter.drawText(QRect(0, 0, size, size), Qt::AlignCenter, initials);

  return avatar;
}

void AvatarManager::onNetworkAvatarReceived(const QString &username,
                                            const QByteArray &data) {
  QPixmap avatar;
  if (avatar.loadFromData(data)) {
    m_avatarCache[username] = avatar;
    emit avatarUpdated(username, avatar);
  }
}
