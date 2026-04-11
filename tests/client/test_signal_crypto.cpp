#include <gtest/gtest.h>
#include "../../client/crypto/SignalProvider.h"
#include <signal_protocol.h>
#include <signal_protocol_internal.h>
#include <vector>
#include <cstring>

class SignalCryptoTest : public ::testing::Test {
protected:
    void SetUp() override {
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
