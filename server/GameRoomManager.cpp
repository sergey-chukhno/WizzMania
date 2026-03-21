#include "GameRoomManager.h"
#include "ClientSession.h"

namespace wizz {

void GameRoomManager::updateGameStatus(const std::string& username, const std::string& gameName, uint32_t score) {
  m_gameStatuses[username] = {gameName, score};
}

bool GameRoomManager::getGameStatus(const std::string& username, std::string& outGameName, uint32_t& outScore) const {
  auto it = m_gameStatuses.find(username);
  if (it != m_gameStatuses.end()) {
    outGameName = it->second.gameName;
    outScore = it->second.score;
    return true;
  }
  return false;
}

void GameRoomManager::clearGameStatus(const std::string& username) {
  m_gameStatuses.erase(username);
}

void GameRoomManager::createRoom(const std::string& roomId, ClientSession* playerX, ClientSession* playerO) {
  m_gameRooms[roomId] = std::make_pair(playerX, playerO);
}

void GameRoomManager::removeRoom(const std::string& roomId) {
  m_gameRooms.erase(roomId);
}

bool GameRoomManager::roomExists(const std::string& roomId) const {
  return m_gameRooms.find(roomId) != m_gameRooms.end();
}

std::pair<ClientSession*, ClientSession*> GameRoomManager::getRoomPlayers(const std::string& roomId) const {
  auto it = m_gameRooms.find(roomId);
  if (it != m_gameRooms.end()) {
    return it->second;
  }
  return {nullptr, nullptr};
}

ClientSession* GameRoomManager::getOpponent(const std::string& roomId, ClientSession* requestingPlayer) const {
  auto it = m_gameRooms.find(roomId);
  if (it != m_gameRooms.end()) {
    if (it->second.first == requestingPlayer) {
      return it->second.second;
    } else if (it->second.second == requestingPlayer) {
      return it->second.first;
    }
  }
  return nullptr;
}

} // namespace wizz
