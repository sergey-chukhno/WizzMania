#pragma once

#include <string>
#include <unordered_map>
#include <utility>

namespace wizz {

class ClientSession;

class GameRoomManager {
public:
  GameRoomManager() = default;
  ~GameRoomManager() = default;

  // Game Status Tracking (Global Highscores / Current games)
  void updateGameStatus(const std::string& username, const std::string& gameName, uint32_t score);
  bool getGameStatus(const std::string& username, std::string& outGameName, uint32_t& outScore) const;
  void clearGameStatus(const std::string& username);

  // Matchmaking / Active Rooms
  void createRoom(const std::string& roomId, ClientSession* playerX, ClientSession* playerO);
  void removeRoom(const std::string& roomId);
  bool roomExists(const std::string& roomId) const;
  
  std::pair<ClientSession*, ClientSession*> getRoomPlayers(const std::string& roomId) const;
  ClientSession* getOpponent(const std::string& roomId, ClientSession* requestingPlayer) const;

private:
  struct GameStatusInfo {
    std::string gameName;
    uint32_t score;
  };
  
  // Tracks what game a user is currently playing (or last highscore)
  std::unordered_map<std::string, GameStatusInfo> m_gameStatuses;

  // Active Multiplayer Game Rooms
  // Key: Room ID, Value: Pair of ClientSession* (Player X, Player O)
  std::unordered_map<std::string, std::pair<ClientSession*, ClientSession*>> m_gameRooms;
};

} // namespace wizz
