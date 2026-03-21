import sys

filename = "server/TcpServer.cpp"
with open(filename, "r") as f:
    lines = f.readlines()

new_lines = []
skip = False

# Includes
new_lines.append('#include "TcpServer.h"\n')
new_lines.append('#include "handlers/AuthHandlers.h"\n')
new_lines.append('#include "handlers/SocialHandlers.h"\n')
new_lines.append('#include "handlers/GameHandlers.h"\n')

for i, line in enumerate(lines):
    if i == 0: continue # Skip first include
    if "TcpServer::TcpServer(int port)" in line:
        new_lines.append(line)
        for j in range(1, 12):
            new_lines.append(lines[i+j])
        new_lines.append(
"""  m_packetRouter.registerHandler(PacketType::Login, std::make_unique<LoginHandler>());
  m_packetRouter.registerHandler(PacketType::Register, std::make_unique<RegisterHandler>());
  m_packetRouter.registerHandler(PacketType::DirectMessage, std::make_unique<MessageHandler>());
  m_packetRouter.registerHandler(PacketType::Nudge, std::make_unique<NudgeHandler>());
  m_packetRouter.registerHandler(PacketType::VoiceMessage, std::make_unique<VoiceMessageHandler>());
  m_packetRouter.registerHandler(PacketType::TypingIndicator, std::make_unique<TypingIndicatorHandler>());
  m_packetRouter.registerHandler(PacketType::ContactStatusChange, std::make_unique<StatusChangeHandler>());
  m_packetRouter.registerHandler(PacketType::UpdateStatus, std::make_unique<UpdateStatusHandler>());
  m_packetRouter.registerHandler(PacketType::UpdateAvatar, std::make_unique<UpdateAvatarHandler>());
  m_packetRouter.registerHandler(PacketType::GetAvatar, std::make_unique<GetAvatarHandler>());
  m_packetRouter.registerHandler(PacketType::AddContact, std::make_unique<AddContactHandler>());
  m_packetRouter.registerHandler(PacketType::RemoveContact, std::make_unique<RemoveContactHandler>());
  m_packetRouter.registerHandler(PacketType::GameStatus, std::make_unique<GameStatusHandler>());
  m_packetRouter.registerHandler(PacketType::GameInvite, std::make_unique<GameInviteHandler>());
  m_packetRouter.registerHandler(PacketType::GameInviteResponse, std::make_unique<GameInviteResponseHandler>());
  m_packetRouter.registerHandler(PacketType::GameMove, std::make_unique<GameMoveHandler>());
}\n""")
        skip = True
        continue
    
    if skip and line.startswith("TcpServer::~TcpServer()"): # Line 28
        skip = False
    
    if skip: continue

    if "auto session = std::make_shared<ClientSession>(" in line:
        new_lines.append("      auto session = std::make_shared<ClientSession>(\n")
        new_lines.append("          sessionId, std::move(socket), m_sslContext, this);\n")
        skip = True
        continue
    
    if skip and "m_sessionManager.addSession(sessionId, session);" in line:
        skip = False
    
    if skip: continue

    if "void wizz::TcpServer::handleLogin(ClientSession *session) {" in line:
        skip = True
        continue
    
    if skip and "void wizz::TcpServer::handleDisconnect(int sessionId) {" in line:
        skip = False

    if skip: continue

    new_lines.append(line)

with open(filename, "w") as f:
    f.writelines(new_lines)
