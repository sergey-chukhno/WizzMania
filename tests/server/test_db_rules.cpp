#include <gtest/gtest.h>
#include "../../server/DatabaseManager.h"
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
    EXPECT_TRUE(db.createUser("sergey", "secure_password"));

    // 2. Duplicate detection
    EXPECT_FALSE(db.createUser("sergey", "another_password"));

    // 3. Credential verification
    EXPECT_TRUE(db.checkCredentials("sergey", "secure_password"));
    EXPECT_FALSE(db.checkCredentials("sergey", "wrong_password"));
    EXPECT_FALSE(db.checkCredentials("unknown", "password"));
}

/**
 * @brief Verifies social graph logic (Friends/Followers).
 */
TEST_F(DatabaseManagerTest, FriendshipLogic) {
    DatabaseManager db(testDbPath);
    ASSERT_TRUE(db.init());

    db.createUser("alice", "pass");
    db.createUser("bob", "pass");

    // 1. Add friend
    EXPECT_TRUE(db.addFriend("alice", "bob"));
    
    // 2. Verify bidirectional friendship/followers
    auto aliceFriends = db.getFriends("alice");
    EXPECT_EQ(aliceFriends.size(), 1);
    EXPECT_EQ(aliceFriends[0], "bob");

    auto bobFollowers = db.getFollowers("bob");
    EXPECT_EQ(bobFollowers.size(), 1);
    EXPECT_EQ(bobFollowers[0], "alice");

    // 3. Duplicate friendship
    EXPECT_FALSE(db.addFriend("alice", "bob"));

    // 4. Remove friend
    EXPECT_TRUE(db.removeFriend("alice", "bob"));
    EXPECT_EQ(db.getFriends("alice").size(), 0);
}

/**
 * @brief Verifies message persistence and pending message retrieval.
 */
TEST_F(DatabaseManagerTest, MessagePersistence) {
    DatabaseManager db(testDbPath);
    ASSERT_TRUE(db.init());

    db.createUser("sender", "pass");
    db.createUser("recipient", "pass");

    // 1. Store undelivered message
    EXPECT_TRUE(db.storeMessage("sender", "recipient", "Hello Offline", false));
    EXPECT_TRUE(db.storeMessage("sender", "recipient", "Second Message", false));

    // 2. Fetch pending messages
    auto pending = db.fetchPendingMessages("recipient");
    EXPECT_EQ(pending.size(), 2);
    EXPECT_EQ(pending[0].body, "Hello Offline");
    EXPECT_EQ(pending[1].body, "Second Message");

    // 3. Mark as delivered and verify
    db.markAsDelivered(pending[0].id);
    db.markAsDelivered(pending[1].id);
    
    EXPECT_EQ(db.fetchPendingMessages("recipient").size(), 0);
}

/**
 * @brief Verifies that data persists across database closure and re-initialization.
 */
TEST_F(DatabaseManagerTest, PersistenceAcrossReinit) {
    {
        DatabaseManager db(testDbPath);
        ASSERT_TRUE(db.init());
        db.createUser("persistent_user", "pass123");
        db.createUser("friend_user", "pass");
        db.addFriend("persistent_user", "friend_user");
    } // db is destroyed here (sqlite3_close)

    {
        DatabaseManager db(testDbPath);
        ASSERT_TRUE(db.init());
        
        // 1. Verify user exists and credentials work
        EXPECT_TRUE(db.checkCredentials("persistent_user", "pass123"));
        
        // 2. Verify friendship persisted
        auto friends = db.getFriends("persistent_user");
        EXPECT_EQ(friends.size(), 1);
        EXPECT_EQ(friends[0], "friend_user");

        // 3. Duplicate detection still works on re-init session
        EXPECT_FALSE(db.createUser("persistent_user", "newpass"));
    }
}

/**
 * @brief Verifies End-to-End Encryption Key storage (Zero-Knowledge Relay Phase 1).
 */
TEST_F(DatabaseManagerTest, E2EEKeyRegistry) {
    DatabaseManager db(testDbPath);
    ASSERT_TRUE(db.init());

    db.createUser("alice", "pass");

    // 1. Store Identity and Signed Pre-Keys
    EXPECT_TRUE(db.storeUserKeys("alice", "identity_base64", "signed_prekey_base64", "signature_base64"));
    
    // 2. Upload One-Time Pre-Keys
    std::vector<std::pair<int, std::string>> otks = {
        {1, "otk_1_base64"},
        {2, "otk_2_base64"},
        {3, "otk_3_base64"}
    };
    EXPECT_TRUE(db.storeOneTimeKeys("alice", otks));

    // 3. Fetch PreKey Bundle
    auto bundle = db.fetchPreKeyBundle("alice");
    EXPECT_EQ(bundle.identityKey, "identity_base64");
    EXPECT_EQ(bundle.signedPreKey, "signed_prekey_base64");
    EXPECT_EQ(bundle.signedPreKeySignature, "signature_base64");
    
    // It should pop one OTK
    EXPECT_EQ(bundle.oneTimeKeyId, 1);
    EXPECT_EQ(bundle.oneTimeKey, "otk_1_base64");

    // 4. Fetch another bundle, should pop the next OTK
    auto bundle2 = db.fetchPreKeyBundle("alice");
    EXPECT_EQ(bundle2.oneTimeKeyId, 2);
    EXPECT_EQ(bundle2.oneTimeKey, "otk_2_base64");
}

} // namespace wizz
