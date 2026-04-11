#include <gtest/gtest.h>
#include "../../client/data/LocalDatabase.h"
#include <vector>
#include <filesystem>

using namespace wizz::client;

class LocalDatabaseTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path = "test_wizz_session.db";
        std::filesystem::remove(db_path);
        db = new LocalDatabase(db_path);
        db->init();
    }

    void TearDown() override {
        delete db;
        std::filesystem::remove(db_path);
    }

    std::string db_path;
    LocalDatabase* db;
};

TEST_F(LocalDatabaseTest, StoresAndRetrievesIdentity) {
    std::vector<uint8_t> ident = {0x01, 0x02, 0x03, 0x04};
    uint32_t regId = 1234;

    db->setIdentity(ident, regId);

    std::vector<uint8_t> fetchedIdent;
    uint32_t fetchedRegId = 0;
    db->getIdentity(fetchedIdent, fetchedRegId);

    EXPECT_EQ(fetchedIdent, ident);
    EXPECT_EQ(fetchedRegId, regId);
}

TEST_F(LocalDatabaseTest, StoresAndRetrievesPreKeys) {
    std::vector<uint8_t> keyData = {0xAA, 0xBB, 0xCC};
    uint32_t keyId = 5;

    db->storePreKey(keyId, keyData);

    std::vector<uint8_t> fetchedData;
    EXPECT_TRUE(db->loadPreKey(keyId, fetchedData));
    EXPECT_EQ(fetchedData, keyData);

    db->removePreKey(keyId);
    EXPECT_FALSE(db->loadPreKey(keyId, fetchedData));
}

TEST_F(LocalDatabaseTest, StoresAndRetrievesSignedPreKeys) {
    std::vector<uint8_t> keyData = {0xDE, 0xAD, 0xBE, 0xEF};
    uint32_t keyId = 1;

    db->storeSignedPreKey(keyId, keyData);

    std::vector<uint8_t> fetchedData;
    EXPECT_TRUE(db->loadSignedPreKey(keyId, fetchedData));
    EXPECT_EQ(fetchedData, keyData);
}

TEST_F(LocalDatabaseTest, StoresAndRetrievesSessions) {
    std::string name = "alice";
    std::vector<uint8_t> sessionData = {0xDE, 0xCA, 0xFB, 0xAD};

    db->storeSession(name, sessionData);

    std::vector<uint8_t> fetchedData;
    EXPECT_TRUE(db->loadSession(name, fetchedData));
    EXPECT_EQ(fetchedData, sessionData);
}

TEST_F(LocalDatabaseTest, HandlesMissingData) {
    std::vector<uint8_t> data;
    EXPECT_FALSE(db->loadPreKey(999, data));
    EXPECT_FALSE(db->loadSession("bob", data));
}
