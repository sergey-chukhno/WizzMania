#include <gtest/gtest.h>
#include "mocks.h"
#include "../../server/logic/SessionManager.h"
#include <memory>
#include <asio.hpp>

namespace wizz {

class SessionManagerTest : public ::testing::Test {
protected:
    asio::io_context io;
    asio::ssl::context ssl{asio::ssl::context::sslv23};
    TcpServer server{0, "test_session.db"};
    SessionManager manager;
    
    std::shared_ptr<MockClientSession> session1;
    std::shared_ptr<MockClientSession> session2;

    void SetUp() override {
        session1 = std::make_shared<MockClientSession>(101, io, ssl, &server);
        session2 = std::make_shared<MockClientSession>(102, io, ssl, &server);
    }
};

TEST_F(SessionManagerTest, AddAndRemoveSession) {
    manager.addSession(101, session1);
    EXPECT_EQ(manager.getSessionById(101), session1.get());
    
    manager.removeSession(101);
    EXPECT_EQ(manager.getSessionById(101), nullptr);
}

TEST_F(SessionManagerTest, UserOnlineStatus) {
    ON_CALL(*session1, getUsername()).WillByDefault(::testing::Return("sergey"));
    
    manager.setUserOnline("sergey", session1.get(), "Coding...");
    
    EXPECT_TRUE(manager.isUserOnline("sergey"));
    EXPECT_EQ(manager.getSessionByUsername("sergey"), session1.get());
    EXPECT_EQ(manager.getCustomStatus("sergey"), "Coding...");
    
    manager.setUserOffline("sergey");
    EXPECT_FALSE(manager.isUserOnline("sergey"));
    EXPECT_EQ(manager.getSessionByUsername("sergey"), nullptr);
}

TEST_F(SessionManagerTest, StatusUpdates) {
    ON_CALL(*session2, getUsername()).WillByDefault(::testing::Return("bob"));
    manager.setUserOnline("bob", session2.get(), "");
    
    manager.updateStatus("bob", wizz::UserStatus::Away);
    EXPECT_EQ(manager.getStatus("bob"), wizz::UserStatus::Away);
    
    manager.updateCustomStatus("bob", "At lunch");
    EXPECT_EQ(manager.getCustomStatus("bob"), "At lunch");
}

} // namespace wizz
