#include <gtest/gtest.h>
#include <gmock/gmock.h>
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

// Dummy for ClientSession
namespace wizz { class ClientSession {}; }

class PacketRouterTest : public ::testing::Test {
protected:
    PacketRouter router;
    ClientSession* dummySession = nullptr;
};

TEST_F(PacketRouterTest, DispatchesToRegisteredHandler) {
    auto mockHandler = std::make_unique<MockPacketHandler>();
    MockPacketHandler* handlerPtr = mockHandler.get();
    
    router.registerHandler(PacketType::Login, std::move(mockHandler));
    
    Packet loginPacket(PacketType::Login);
    
    // Expect the handler to be called once
    EXPECT_CALL(*handlerPtr, handle(dummySession, _)).Times(1);
    
    router.handle(dummySession, loginPacket);
}

TEST_F(PacketRouterTest, IgnoresUnregisteredType) {
    // No handlers registered
    Packet unknownPacket(PacketType::Nudge);
    
    // Should not crash or throw
    EXPECT_NO_THROW(router.handle(dummySession, unknownPacket));
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
    
    router.handle(dummySession, lp);
    router.handle(dummySession, mp);
}
