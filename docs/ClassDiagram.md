# WizzMania Technical Class Diagram (Low-Level)

```mermaid
classDiagram
class TcpServer
class PacketRouter
class IPacketHandler
class SessionManager
class DatabaseManager
class ClientSession
class NetworkManager
class MainWindow
class GameBridge
class NativeSharedMemory
class LoginHandler
class RegisterHandler
class MessageHandler
class GameMoveHandler

%% Server
TcpServer : run()
TcpServer : stop()
TcpServer : getSession(int id)
TcpServer : handleDisconnect(int id)
TcpServer : m_ioContext io_context
TcpServer : m_acceptor tcp_acceptor
TcpServer : m_db DatabaseManager
TcpServer : m_sessionManager SessionManager
TcpServer : m_packetRouter PacketRouter

PacketRouter : registerHandler(type, handler)
PacketRouter : handle(session, packet)
PacketRouter : m_handlers Map

IPacketHandler : handle(session, packet)

SessionManager : addSession(id, session)
SessionManager : removeSession(id)
SessionManager : setUserOnline(user, session, status)
SessionManager : updateStatus(user, status)
SessionManager : m_sessions Map
SessionManager : m_onlineUsers Map

DatabaseManager : init()
DatabaseManager : postTask(task)
DatabaseManager : createUser(user, pass)
DatabaseManager : checkCredentials(user, pass)
DatabaseManager : storeMessage(sender, recp, body)
DatabaseManager : addFriend(user, friend)
DatabaseManager : m_db sqlite3
DatabaseManager : m_tasks Queue

ClientSession : getId()
ClientSession : getUsername()
ClientSession : sendPacket(packet)
ClientSession : doRead()
ClientSession : m_socket ssl_stream
ClientSession : m_username string
ClientSession : m_isLoggedIn bool

%% Client
NetworkManager : instance()
NetworkManager : connectToHost(host, port)
NetworkManager : sendPacket(packet)
NetworkManager : m_socket QSslSocket
NetworkManager : m_packetHandlers Map

MainWindow : setContacts(list)
MainWindow : updateContactStatus(user, status)
MainWindow : onSendMessage()
MainWindow : onAddFriendClicked()
MainWindow : m_username string
MainWindow : m_contacts List
MainWindow : m_gameBridge GameBridge

GameBridge : startGameIPC()
GameBridge : startTicTacToe(user, room, ...)
GameBridge : m_gameIPC GameSharedMemory
GameBridge : m_tttMemory NativeSharedMemory
GameBridge : m_tttProcess QProcess

NativeSharedMemory : createAndMap()
NativeSharedMemory : openAndMap()
NativeSharedMemory : lock()
NativeSharedMemory : unlock()
NativeSharedMemory : m_fd int
NativeSharedMemory : m_data T*

%% Handlers
LoginHandler : handle(session, packet)
RegisterHandler : handle(session, packet)
MessageHandler : handle(session, packet)
GameMoveHandler : handle(session, packet)

%% Relationships
TcpServer *-- PacketRouter
TcpServer *-- SessionManager
TcpServer *-- DatabaseManager
PacketRouter o-- IPacketHandler
IPacketHandler <|-- LoginHandler
IPacketHandler <|-- RegisterHandler
IPacketHandler <|-- MessageHandler
IPacketHandler <|-- GameMoveHandler
SessionManager *-- ClientSession
MainWindow o-- NetworkManager
MainWindow o-- GameBridge
GameBridge o-- NativeSharedMemory
```

> [!NOTE]
> This diagram is 100% accurate to the refactored C++ code. The dynamic version is available in the presentation under `mermaid_view.html`.
