#include <gtest/gtest.h>
#include "../../server/data/DatabaseManager.h"
#include <cstdio>
#include <vector>
#include <string>

namespace wizz {

class DatabaseManagerTest : public ::testing::Test {
protected:
    const std::string testDbPath = "test_wizzmania.db";

    void SetUp() override {
        // Ensure a clean state for each test
        std::remove(testDbPath.c_str());
    }

    void TearDown() override {
        // Cleanup after each test
        std::remove(testDbPath.c_str());
    }
};

/**
 * @brief Verifies that the database initializes successfully and creates tables.
 */
TEST_F(DatabaseManagerTest, Initialization) {
    DatabaseManager db(testDbPath);
    EXPECT_TRUE(db.init());
}

/**
 * @brief Verifies user creation and duplicate prevention.
 */
TEST_F(DatabaseManagerTest, UserManagement) {
    DatabaseManager db(testDbPath);
    ASSERT_TRUE(db.init());

    // 1. Create a new user
    EXPECT_TRUE(db.auth()->createUser("testuser", "secure_password"));

    // 2. Duplicate detection
    EXPECT_FALSE(db.auth()->createUser("testuser", "another_password"));

    // 3. Credential verification
    EXPECT_TRUE(db.auth()->checkCredentials("testuser", "secure_password"));
    EXPECT_FALSE(db.auth()->checkCredentials("testuser", "wrong_password"));
    EXPECT_FALSE(db.auth()->checkCredentials("unknown", "password"));
}

/**
 * @brief Verifies social graph logic (Friends/Followers).
 */
TEST_F(DatabaseManagerTest, FriendshipLogic) {
    DatabaseManager db(testDbPath);
    ASSERT_TRUE(db.init());

    db.auth()->createUser("alice", "pass");
    db.auth()->createUser("bob", "pass");

    // 1. Add friend
    EXPECT_EQ(db.social()->addFriend("alice", "bob"), 0);
    
    // 2. Verify bidirectional friendship/followers
    auto aliceFriends = db.social()->getFriends("alice");
    EXPECT_EQ(aliceFriends.size(), 1);
    EXPECT_EQ(aliceFriends[0], "bob");

    auto bobFollowers = db.social()->getFollowers("bob");
    EXPECT_EQ(bobFollowers.size(), 1);
    EXPECT_EQ(bobFollowers[0], "alice");

    // 3. Duplicate friendship
    EXPECT_NE(db.social()->addFriend("alice", "bob"), 0);

    // 4. Remove friend
    EXPECT_TRUE(db.social()->removeFriend("alice", "bob"));
    EXPECT_EQ(db.social()->getFriends("alice").size(), 0);
}

/**
 * @brief Verifies message persistence and pending message retrieval.
 */
TEST_F(DatabaseManagerTest, MessagePersistence) {
    DatabaseManager db(testDbPath);
    ASSERT_TRUE(db.init());

    db.auth()->createUser("sender", "pass");
    db.auth()->createUser("recipient", "pass");

    // 1. Store undelivered message
    EXPECT_TRUE(db.social()->storeMessage("sender", "recipient", "Hello Offline", false));
    EXPECT_TRUE(db.social()->storeMessage("sender", "recipient", "Second Message", false));

    // 2. Fetch pending messages
    auto pending = db.social()->fetchPendingMessages("recipient");
    EXPECT_EQ(pending.size(), 2);
    EXPECT_EQ(pending[0].body, "Hello Offline");
    EXPECT_EQ(pending[1].body, "Second Message");

    // 3. Mark as delivered and verify
    db.social()->markAsDelivered(pending[0].id);
    db.social()->markAsDelivered(pending[1].id);
    
    EXPECT_EQ(db.social()->fetchPendingMessages("recipient").size(), 0);
}

/**
 * @brief Verifies that data persists across database closure and re-initialization.
 */
TEST_F(DatabaseManagerTest, PersistenceAcrossReinit) {
    {
        DatabaseManager db(testDbPath);
        ASSERT_TRUE(db.init());
        db.auth()->createUser("persistent_user", "pass123");
        db.auth()->createUser("friend_user", "pass");
        db.social()->addFriend("persistent_user", "friend_user");
    } // db is destroyed here (sqlite3_close)

    {
        DatabaseManager db(testDbPath);
        ASSERT_TRUE(db.init());
        
        // 1. Verify user exists and credentials work
        EXPECT_TRUE(db.auth()->checkCredentials("persistent_user", "pass123"));
        
        // 2. Verify friendship persisted
        auto friends = db.social()->getFriends("persistent_user");
        EXPECT_EQ(friends.size(), 1);
        EXPECT_EQ(friends[0], "friend_user");

        // 3. Duplicate detection still works on re-init session
        EXPECT_FALSE(db.auth()->createUser("persistent_user", "newpass"));
    }
}

/**
 * @brief Verifies End-to-End Encryption Key storage (Zero-Knowledge Relay Phase 1).
 */
TEST_F(DatabaseManagerTest, E2EEKeyRegistry) {
    DatabaseManager db(testDbPath);
    ASSERT_TRUE(db.init());

    db.auth()->createUser("alice", "pass");

    // 1. Store Identity and Signed Pre-Keys
    std::string ident(33, 'A');
    std::string signedKey(33, 'B');
    std::string signature(64, 'C');
    EXPECT_TRUE(db.crypto()->storeUserKeys("alice", ident, signedKey, signature, 12345));
    
    // 2. Upload One-Time Pre-Keys
    std::vector<std::pair<int, std::string>> otks = {
        {1, std::string(33, '1')},
        {2, std::string(33, '2')},
        {3, std::string(33, '3')}
    };
    EXPECT_TRUE(db.crypto()->storeOneTimeKeys("alice", otks));

    // 3. Fetch PreKey Bundle
    auto bundle = db.crypto()->fetchPreKeyBundle("alice");
    EXPECT_EQ(bundle.identityKey, ident);
    EXPECT_EQ(bundle.signedPreKey, signedKey);
    EXPECT_EQ(bundle.signedPreKeySignature, signature);
    EXPECT_EQ(bundle.registrationId, 12345);
    
    // It should pop one OTK
    EXPECT_EQ(bundle.oneTimeKeyId, 1);
    EXPECT_EQ(bundle.oneTimeKey, std::string(33, '1'));

    // 4. Fetch another bundle, should pop the next OTK
    auto bundle2 = db.crypto()->fetchPreKeyBundle("alice");
    EXPECT_EQ(bundle2.oneTimeKeyId, 2);
    EXPECT_EQ(bundle2.oneTimeKey, std::string(33, '2'));

    // 5. Fetch another, should pop the last OTK
    auto bundle3 = db.crypto()->fetchPreKeyBundle("alice");
    EXPECT_EQ(bundle3.oneTimeKeyId, 3);
    EXPECT_EQ(bundle3.oneTimeKey, std::string(33, '3'));
}

} // namespace wizz
