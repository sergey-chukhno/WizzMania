#include <gtest/gtest.h>
#include "../../server/SessionManager.h"
#include <memory>

namespace wizz {
    // Forward declaration or dummy for ClientSession if needed
    class ClientSession {};
}

using namespace wizz;

class SessionManagerTest : public ::testing::Test {
protected:
    SessionManager manager;
    // We use nullptr or small dummy objects for ClientSession pointers
    // since SessionManager doesn't dereference them in the public API
    std::shared_ptr<ClientSession> dummySession1 = std::make_shared<ClientSession>();
    std::shared_ptr<ClientSession> dummySession2 = std::make_shared<ClientSession>();
};

TEST_F(SessionManagerTest, AddAndRemoveSession) {
    manager.addSession(101, dummySession1);
    EXPECT_EQ(manager.getSessionById(101), dummySession1.get());
    
    manager.removeSession(101);
    EXPECT_EQ(manager.getSessionById(101), nullptr);
}

TEST_F(SessionManagerTest, UserOnlineStatus) {
    manager.setUserOnline("sergey", dummySession1.get(), "Coding...");
    
    EXPECT_TRUE(manager.isUserOnline("sergey"));
    EXPECT_EQ(manager.getSessionByUsername("sergey"), dummySession1.get());
    EXPECT_EQ(manager.getCustomStatus("sergey"), "Coding...");
    
    manager.setUserOffline("sergey");
    EXPECT_FALSE(manager.isUserOnline("sergey"));
    EXPECT_EQ(manager.getSessionByUsername("sergey"), nullptr);
}

TEST_F(SessionManagerTest, StatusUpdates) {
    manager.setUserOnline("bob", dummySession2.get(), "");
    
    // Status (0=Online, 1=Away, 2=Busy, 3=Offline)
    manager.updateStatus("bob", 1);
    EXPECT_EQ(manager.getStatus("bob"), 1);
    
    manager.updateCustomStatus("bob", "At lunch");
    EXPECT_EQ(manager.getCustomStatus("bob"), "At lunch");
}

TEST_F(SessionManagerTest, GetAllOnline) {
    manager.setUserOnline("user1", dummySession1.get(), "");
    manager.setUserOnline("user2", dummySession2.get(), "");
    
    auto usernames = manager.getAllOnlineUsernames();
    EXPECT_EQ(usernames.size(), 2);
    EXPECT_TRUE(std::find(usernames.begin(), usernames.end(), "user1") != usernames.end());
    EXPECT_TRUE(std::find(usernames.begin(), usernames.end(), "user2") != usernames.end());
    
    auto sessions = manager.getAllOnlineSessions();
    EXPECT_EQ(sessions.size(), 2);
}

TEST_F(SessionManagerTest, NonExistentUser) {
    EXPECT_FALSE(manager.isUserOnline("nobody"));
    EXPECT_EQ(manager.getSessionByUsername("nobody"), nullptr);
    EXPECT_EQ(manager.getCustomStatus("nobody"), "");
}
