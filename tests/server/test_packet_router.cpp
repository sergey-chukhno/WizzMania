#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "mocks.h"
#include "../../server/handlers/PacketRouter.h"
#include "../../server/handlers/IPacketHandler.h"
#include "../../common/Packet.h"

using namespace wizz;
using namespace ::testing;

// Mock for IPacketHandler
class MockPacketHandler : public IPacketHandler {
public:
    MOCK_METHOD(void, handle, (ClientSession* session, Packet& packet), (override));
};

class PacketRouterTest : public ::testing::Test {
protected:
    asio::io_context io;
    asio::ssl::context ssl{asio::ssl::context::sslv23};
    TcpServer server{0, "test_router.db"};
    PacketRouter router;
    std::shared_ptr<MockClientSession> session1;

    void SetUp() override {
        session1 = std::make_shared<MockClientSession>(1, io, ssl, &server);
    }
};

TEST_F(PacketRouterTest, DispatchesToRegisteredHandler) {
    auto mockHandler = std::make_unique<MockPacketHandler>();
    MockPacketHandler* handlerPtr = mockHandler.get();
    
    router.registerHandler(PacketType::Login, std::move(mockHandler));
    
    Packet loginPacket(PacketType::Login);
    
    // Expect the handler to be called once
    EXPECT_CALL(*handlerPtr, handle(session1.get(), _)).Times(1);
    
    router.handle(session1.get(), loginPacket);
}

TEST_F(PacketRouterTest, IgnoresUnregisteredType) {
    Packet unknownPacket(PacketType::Nudge);
    EXPECT_NO_THROW(router.handle(session1.get(), unknownPacket));
}

TEST_F(PacketRouterTest, MultipleHandlers) {
    auto loginHandler = std::make_unique<MockPacketHandler>();
    auto msgHandler = std::make_unique<MockPacketHandler>();
    
    MockPacketHandler* loginPtr = loginHandler.get();
    MockPacketHandler* msgPtr = msgHandler.get();
    
    router.registerHandler(PacketType::Login, std::move(loginHandler));
    router.registerHandler(PacketType::DirectMessage, std::move(msgHandler));
    
    Packet lp(PacketType::Login);
    Packet mp(PacketType::DirectMessage);
    
    EXPECT_CALL(*loginPtr, handle(_, _)).Times(1);
    EXPECT_CALL(*msgPtr, handle(_, _)).Times(1);
    
    router.handle(session1.get(), lp);
    router.handle(session1.get(), mp);
}
