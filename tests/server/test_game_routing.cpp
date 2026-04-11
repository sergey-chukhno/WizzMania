#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "mocks.h"
#include "../../server/logic/handlers/GameHandlers.h"
#include "../../server/core/ClientSession.h"
#include "../../server/core/TcpServer.h"
#include "../../server/logic/SessionManager.h"
#include "../../server/logic/GameRoomManager.h"
#include "../../common/Packet.h"

using namespace ::testing;

namespace wizz {

class GameRoutingTest : public ::testing::Test {
protected:
    asio::io_context ioContext;
    asio::ssl::context sslContext{asio::ssl::context::sslv23};
    TcpServer server{0, "test_game_routing.db"}; // Port 0 for testing
    std::shared_ptr<MockClientSession> alice;
    std::shared_ptr<MockClientSession> bob;

    void SetUp() override {
        alice = std::make_shared<MockClientSession>(1, ioContext, sslContext, &server);
        bob = std::make_shared<MockClientSession>(2, ioContext, sslContext, &server);

        ON_CALL(*alice, isLoggedIn()).WillByDefault(Return(true));
        ON_CALL(*alice, getUsername()).WillByDefault(Return("alice"));
        
        ON_CALL(*bob, isLoggedIn()).WillByDefault(Return(true));
        ON_CALL(*bob, getUsername()).WillByDefault(Return("bob"));

        server.getSessionManager().addSession(1, alice);
        server.getSessionManager().addSession(2, bob);
        server.getSessionManager().setUserOnline("alice", alice.get(), "");
        server.getSessionManager().setUserOnline("bob", bob.get(), "");
    }
};

TEST_F(GameRoutingTest, RouteInvite) {
    GameInviteHandler handler;
    
    Packet invitePkt(PacketType::GameInvite);
    invitePkt.writeString("bob");
    invitePkt.writeString("TicTacToe");

    EXPECT_CALL(*bob, sendPacket(Property(&Packet::type, PacketType::GameInvite))).Times(1);

    handler.handle(alice.get(), invitePkt);
}

TEST_F(GameRoutingTest, AcceptInviteStartsGame) {
    GameInviteResponseHandler handler;

    Packet responsePkt(PacketType::GameInviteResponse);
    responsePkt.writeString("alice");
    responsePkt.writeString("TicTacToe");
    responsePkt.writeInt(1); // Accepted

    // Alice (original sender) receives BOTH an acknowledgment and the start packet
    EXPECT_CALL(*alice, sendPacket(Property(&Packet::type, PacketType::GameInviteResponse))).Times(1);
    EXPECT_CALL(*alice, sendPacket(Property(&Packet::type, PacketType::GameStart))).Times(1);
    
    // Bob (acceptor) only receives GameStart
    EXPECT_CALL(*bob, sendPacket(Property(&Packet::type, PacketType::GameStart))).Times(1);

    handler.handle(bob.get(), responsePkt);
}

TEST_F(GameRoutingTest, RelayGameMove) {
    std::string roomId = "test_room";
    server.getGameRoomManager().createRoom(roomId, alice.get(), bob.get());

    GameMoveHandler handler;
    Packet movePkt(PacketType::GameMove);
    movePkt.writeString(roomId);
    movePkt.writeInt(4); 

    EXPECT_CALL(*bob, sendPacket(Property(&Packet::type, PacketType::GameMove))).Times(1);

    handler.handle(alice.get(), movePkt);
}

} // namespace wizz
