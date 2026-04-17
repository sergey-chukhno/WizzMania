#include <gtest/gtest.h>
#include "../../client/crypto/SignalProvider.h"
#include <signal_protocol.h>
#include <signal_protocol_internal.h>
#include <vector>
#include <cstring>
#include <session_builder.h>
#include <session_cipher.h>
#include <session_pre_key.h>

class SignalCryptoTest : public ::testing::Test {
protected:
    void SetUp() override {
        std::remove("alice_test.db");
        std::remove("bob_test.db");
        wizz::client::crypto::setupSignalCryptoProvider(&context);
    }

    void TearDown() override {
        // signal_context has no destroy?
    }

    signal_context* context = nullptr;
};

TEST_F(SignalCryptoTest, ProviderIsInitialized) {
    ASSERT_NE(context, nullptr);
    signal_crypto_provider* provider = &context->crypto_provider;
    ASSERT_NE(provider, nullptr);
}

TEST_F(SignalCryptoTest, RandomNumberGeneration) {
    uint8_t buf1[32];
    uint8_t buf2[32];
    
    signal_crypto_provider* provider = &context->crypto_provider;
    provider->random_func(buf1, 32, nullptr);
    provider->random_func(buf2, 32, nullptr);
    
    EXPECT_NE(std::memcmp(buf1, buf2, 32), 0);
}

TEST_F(SignalCryptoTest, SHA512Digest) {
    signal_crypto_provider* provider = &context->crypto_provider;
    
    const char* input = "WizzMania E2EE";
    void* digest_ctx = nullptr;
    provider->sha512_digest_init_func(&digest_ctx, nullptr);
    provider->sha512_digest_update_func(digest_ctx, (const uint8_t*)input, strlen(input), nullptr);
    
    signal_buffer* output = nullptr;
    provider->sha512_digest_final_func(digest_ctx, &output, nullptr);
    provider->sha512_digest_cleanup_func(digest_ctx, nullptr);
    
    ASSERT_NE(output, nullptr);
    EXPECT_EQ(signal_buffer_len(output), 64u); // SHA512 is 64 bytes
    signal_buffer_free(output);
}

TEST_F(SignalCryptoTest, HMACSHA256) {
    signal_crypto_provider* provider = &context->crypto_provider;
    
    uint8_t key[32] = {0x01};
    const char* data = "Secret Message";
    
    void* hmac_ctx = nullptr;
    provider->hmac_sha256_init_func(&hmac_ctx, key, 32, nullptr);
    provider->hmac_sha256_update_func(hmac_ctx, (const uint8_t*)data, strlen(data), nullptr);
    
    signal_buffer* output = nullptr;
    provider->hmac_sha256_final_func(hmac_ctx, &output, nullptr);
    provider->hmac_sha256_cleanup_func(hmac_ctx, nullptr);
    
    ASSERT_NE(output, nullptr);
    EXPECT_EQ(signal_buffer_len(output), 32u); // HMAC-SHA256 is 32 bytes
    signal_buffer_free(output);
}
 
TEST_F(SignalCryptoTest, AES256CBCEncryptionDecryption) {
    signal_crypto_provider* provider = &context->crypto_provider;
    
    uint8_t key[32];
    uint8_t iv[16];
    std::memset(key, 0x42, 32);
    std::memset(iv, 0x13, 16);
    
    const char* plaintext = "Hello WizzMania E2EE!";
    size_t len = std::strlen(plaintext);
    
    signal_buffer* ciphertext = nullptr;
    int res = provider->encrypt_func(&ciphertext, SG_CIPHER_AES_CBC_PKCS5, 
                                        key, 32, iv, 16, 
                                        (const uint8_t*)plaintext, len, nullptr);
    
    ASSERT_EQ(res, 0);
    ASSERT_NE(ciphertext, nullptr);
    EXPECT_GT(signal_buffer_len(ciphertext), len);
 
    signal_buffer* decrypted = nullptr;
    res = provider->decrypt_func(&decrypted, SG_CIPHER_AES_CBC_PKCS5,
                                    key, 32, iv, 16,
                                    signal_buffer_data(ciphertext), signal_buffer_len(ciphertext), nullptr);
                                    
    ASSERT_EQ(res, 0);
    ASSERT_NE(decrypted, nullptr);
    EXPECT_EQ(signal_buffer_len(decrypted), len);
    EXPECT_EQ(std::memcmp(signal_buffer_data(decrypted), plaintext, len), 0);
 
    signal_buffer_free(ciphertext);
    signal_buffer_free(decrypted);
}
 
#include "../../client/crypto/SignalStoreContext.h"
#include "../../client/data/LocalDatabase.h"
 
TEST_F(SignalCryptoTest, FullE2EHandshake) {
    // This test simulates Alice sending an encrypted message to Bob.
    // It verifies: Identity generation, PreKey generation, Bundle processing, Handshake, and Encryption.
    
    // 1. Setup Alice's environment
    wizz::client::LocalDatabase aliceDb(":memory:");
    aliceDb.init();
    signal_protocol_store_context* aliceStore = nullptr;
    wizz::client::crypto::setupSignalStoreContext(&aliceStore, context, &aliceDb);
    wizz::client::crypto::initializeLocalKeys(context, &aliceDb);
 
    // 2. Setup Bob's environment
    wizz::client::LocalDatabase bobDb(":memory:");
    bobDb.init();
    signal_protocol_store_context* bobStore = nullptr;
    wizz::client::crypto::setupSignalStoreContext(&bobStore, context, &bobDb);
    wizz::client::crypto::initializeLocalKeys(context, &bobDb);
 
    // 3. Create Bob's Bundle (Alice needs this to start a session)
    // In reality, this comes from the server.
    uint32_t bobRegId = 0;
    std::vector<uint8_t> bobPub, bobPriv;
    bobDb.getIdentity(bobPub, bobPriv, bobRegId);
    
    // Bob's Identity Key - now stored as raw public key bytes
    signal_buffer* bobIdentBuf = signal_buffer_create(bobPub.data(), bobPub.size());
    ec_public_key* bobIdentPub = nullptr;
    int decIdentRes = curve_decode_point(&bobIdentPub, signal_buffer_data(bobIdentBuf), signal_buffer_len(bobIdentBuf), context);
    ASSERT_EQ(decIdentRes, 0) << "Failed to decode Bob's identity key!";
    ASSERT_NE(bobIdentPub, nullptr);
 
    // Bob's Signed PreKey - now stored as a proper libsignal protobuf record
    std::vector<uint8_t> bobSignedPreKeyData;
    bobDb.loadSignedPreKey(1, bobSignedPreKeyData);
    
    session_signed_pre_key* bobSignedPreKeyRecord = nullptr;
    int deserRes = session_signed_pre_key_deserialize(&bobSignedPreKeyRecord,
                                                       bobSignedPreKeyData.data(),
                                                       bobSignedPreKeyData.size(), context);
    ASSERT_EQ(deserRes, 0) << "Failed to deserialize Bob's SignedPreKey record!";

    ec_public_key* bobSignedPub = ec_key_pair_get_public(
        session_signed_pre_key_get_key_pair(bobSignedPreKeyRecord));
    const uint8_t* sig_ptr = session_signed_pre_key_get_signature(bobSignedPreKeyRecord);
    size_t sig_len = session_signed_pre_key_get_signature_len(bobSignedPreKeyRecord);

    session_pre_key_bundle* bundle = nullptr;
    session_pre_key_bundle_create(&bundle, bobRegId, 1, 
                                 1, nullptr, // pre_key_id, pre_key_public (optional)
                                 1, bobSignedPub, // signed_pre_key_id, signed_pre_key_public
                                 sig_ptr, sig_len,
                                 bobIdentPub);
 
    // 4. Alice processes Bob's Bundle to establish a session
    signal_protocol_address bobAddress = { "bob", 3, 1 };
    session_builder* builder = nullptr;
    session_builder_create(&builder, aliceStore, &bobAddress, context);
    
    int res = session_builder_process_pre_key_bundle(builder, bundle);
    ASSERT_EQ(res, 0) << "session_builder_process_pre_key_bundle failed with code: " << res;
 
    // 5. Alice encrypts a message for Bob
    session_cipher* aliceCipher = nullptr;
    session_cipher_create(&aliceCipher, aliceStore, &bobAddress, context);
    
    const char* secret = "Alice's secret message";
    ciphertext_message* encryptedMsg = nullptr;
    res = session_cipher_encrypt(aliceCipher, (const uint8_t*)secret, strlen(secret), &encryptedMsg);
    
    ASSERT_EQ(res, 0) << "session_cipher_encrypt failed with code: " << res;
    ASSERT_NE(encryptedMsg, nullptr);
 
    // 6. Cleanup
    session_cipher_free(aliceCipher);
    session_builder_free(builder);
    session_pre_key_bundle_destroy((signal_type_base*)bundle);
    signal_buffer_free(bobIdentBuf);
    SIGNAL_UNREF(bobSignedPreKeyRecord);
    SIGNAL_UNREF(bobIdentPub);
    SIGNAL_UNREF(encryptedMsg);
    
    // Remove test DBs
    std::remove("alice_test.db");
    std::remove("bob_test.db");
}
