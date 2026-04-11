#include "SignalProvider.h"
#include "../data/LocalDatabase.h"
#include <openssl/rand.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <iostream>
#include <vector>

namespace wizz {
namespace client {
namespace crypto {

static int crypto_random(uint8_t *data, size_t len, void *user_data) {
    return RAND_bytes(data, len) == 1 ? 0 : -1;
}

static int hmac_sha256_init(void **hmac_context, const uint8_t *key, size_t key_len, void *user_data) {
    HMAC_CTX *ctx = HMAC_CTX_new();
    if (!ctx) return -1;
    if (HMAC_Init_ex(ctx, key, key_len, EVP_sha256(), nullptr) != 1) {
        HMAC_CTX_free(ctx);
        return -1;
    }
    *hmac_context = ctx;
    return 0;
}

static int hmac_sha256_update(void *hmac_context, const uint8_t *data, size_t data_len, void *user_data) {
    HMAC_CTX *ctx = static_cast<HMAC_CTX*>(hmac_context);
    return HMAC_Update(ctx, data, data_len) == 1 ? 0 : -1;
}

static int hmac_sha256_final(void *hmac_context, signal_buffer **output, void *user_data) {
    HMAC_CTX *ctx = static_cast<HMAC_CTX*>(hmac_context);
    uint8_t md[EVP_MAX_MD_SIZE];
    unsigned int len = 0;
    if (HMAC_Final(ctx, md, &len) != 1) return -1;
    *output = signal_buffer_create(md, len);
    return (*output != nullptr) ? 0 : -1;
}

static void hmac_sha256_cleanup(void *hmac_context, void *user_data) {
    if (hmac_context) {
        HMAC_CTX_free(static_cast<HMAC_CTX*>(hmac_context));
    }
}

static int sha512_digest_init(void **digest_context, void *user_data) {
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx) return -1;
    if (EVP_DigestInit_ex(ctx, EVP_sha512(), nullptr) != 1) {
        EVP_MD_CTX_free(ctx);
        return -1;
    }
    *digest_context = ctx;
    return 0;
}

static int sha512_digest_update(void *digest_context, const uint8_t *data, size_t data_len, void *user_data) {
    EVP_MD_CTX *ctx = static_cast<EVP_MD_CTX*>(digest_context);
    return EVP_DigestUpdate(ctx, data, data_len) == 1 ? 0 : -1;
}

static int sha512_digest_final(void *digest_context, signal_buffer **output, void *user_data) {
    EVP_MD_CTX *ctx = static_cast<EVP_MD_CTX*>(digest_context);
    uint8_t md[EVP_MAX_MD_SIZE];
    unsigned int len = 0;
    if (EVP_DigestFinal_ex(ctx, md, &len) != 1) return -1;
    *output = signal_buffer_create(md, len);
    return (*output != nullptr) ? 0 : -1;
}

static void sha512_digest_cleanup(void *digest_context, void *user_data) {
    if (digest_context) {
        EVP_MD_CTX_free(static_cast<EVP_MD_CTX*>(digest_context));
    }
}

static int aes_encrypt(signal_buffer **output, int cipher, const uint8_t *key, size_t key_len, const uint8_t *iv, size_t iv_len, const uint8_t *plaintext, size_t plaintext_len, void *user_data) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return -1;

    const EVP_CIPHER *evp_cipher = nullptr;
    if (cipher == SG_CIPHER_AES_CTR_NOPADDING) {
        if (key_len == 32) evp_cipher = EVP_aes_256_ctr();
        else if (key_len == 16) evp_cipher = EVP_aes_128_ctr();
    } else if (cipher == SG_CIPHER_AES_CBC_PKCS5) {
        if (key_len == 32) evp_cipher = EVP_aes_256_cbc();
        else if (key_len == 16) evp_cipher = EVP_aes_128_cbc();
    }

    if (!evp_cipher) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    if (EVP_EncryptInit_ex(ctx, evp_cipher, nullptr, key, iv) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    if (cipher == SG_CIPHER_AES_CTR_NOPADDING) {
        EVP_CIPHER_CTX_set_padding(ctx, 0);
    }

    int outlen1 = 0, outlen2 = 0;
    std::vector<uint8_t> outbuf(plaintext_len + EVP_MAX_BLOCK_LENGTH);

    if (EVP_EncryptUpdate(ctx, outbuf.data(), &outlen1, plaintext, plaintext_len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    if (EVP_EncryptFinal_ex(ctx, outbuf.data() + outlen1, &outlen2) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    *output = signal_buffer_create(outbuf.data(), outlen1 + outlen2);
    EVP_CIPHER_CTX_free(ctx);
    return (*output != nullptr) ? 0 : -1;
}

static int aes_decrypt(signal_buffer **output, int cipher, const uint8_t *key, size_t key_len, const uint8_t *iv, size_t iv_len, const uint8_t *ciphertext, size_t ciphertext_len, void *user_data) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return -1;

    const EVP_CIPHER *evp_cipher = nullptr;
    if (cipher == SG_CIPHER_AES_CTR_NOPADDING) {
        if (key_len == 32) evp_cipher = EVP_aes_256_ctr();
        else if (key_len == 16) evp_cipher = EVP_aes_128_ctr();
    } else if (cipher == SG_CIPHER_AES_CBC_PKCS5) {
        if (key_len == 32) evp_cipher = EVP_aes_256_cbc();
        else if (key_len == 16) evp_cipher = EVP_aes_128_cbc();
    }

    if (!evp_cipher) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    if (EVP_DecryptInit_ex(ctx, evp_cipher, nullptr, key, iv) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    if (cipher == SG_CIPHER_AES_CTR_NOPADDING) {
        EVP_CIPHER_CTX_set_padding(ctx, 0);
    }

    int outlen1 = 0, outlen2 = 0;
    std::vector<uint8_t> outbuf(ciphertext_len + EVP_MAX_BLOCK_LENGTH);

    if (EVP_DecryptUpdate(ctx, outbuf.data(), &outlen1, ciphertext, ciphertext_len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    if (EVP_DecryptFinal_ex(ctx, outbuf.data() + outlen1, &outlen2) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    *output = signal_buffer_create(outbuf.data(), outlen1 + outlen2);
    EVP_CIPHER_CTX_free(ctx);
    return (*output != nullptr) ? 0 : -1;
}

int setupSignalCryptoProvider(signal_context** global_context) {
    int ret = signal_context_create(global_context, nullptr);
    if (ret != SG_SUCCESS) return ret;

    signal_crypto_provider provider = {
        .random_func = crypto_random,
        .hmac_sha256_init_func = hmac_sha256_init,
        .hmac_sha256_update_func = hmac_sha256_update,
        .hmac_sha256_final_func = hmac_sha256_final,
        .hmac_sha256_cleanup_func = hmac_sha256_cleanup,
        .sha512_digest_init_func = sha512_digest_init,
        .sha512_digest_update_func = sha512_digest_update,
        .sha512_digest_final_func = sha512_digest_final,
        .sha512_digest_cleanup_func = sha512_digest_cleanup,
        .encrypt_func = aes_encrypt,
        .decrypt_func = aes_decrypt,
        .user_data = nullptr
    };

    return signal_context_set_crypto_provider(*global_context, &provider);
}

bool initializeLocalKeys(signal_context* global_context, LocalDatabase* db) {
    if (!db) return false;
    
    std::vector<uint8_t> identBuffer;
    uint32_t regId = 0;
    
    // If we already possess an identity in LocalDatabase, we're fully generated
    if (db->getIdentity(identBuffer, regId)) {
        return true;
    }

    std::cout << "[Crypto] No keys found in LocalDatabase. Generating fresh E2EE Identity..." << std::endl;
    
    // 1. Generate Registration ID
    uint8_t regIDBytes[4];
    crypto_random(regIDBytes, 4, nullptr);
    regId = (*(uint32_t*)regIDBytes) % 16380 + 1; // standard protocol bounds

    // 2. Generate Identity Key (Curve25519)
    ec_key_pair *identityPair = nullptr;
    if (curve_generate_key_pair(global_context, &identityPair) != 0) return false;

    // 3. Serialize and Store Identity
    // We store the raw public and private key bytes safely.
    signal_buffer *pub_buf = nullptr;
    signal_buffer *priv_buf = nullptr;
    ec_public_key_serialize(&pub_buf, ec_key_pair_get_public(identityPair));
    ec_private_key_serialize(&priv_buf, ec_key_pair_get_private(identityPair));

    // For LocalDB MVP, we will concatenate: [1-byte length of Public][PublicBytes][PrivateBytes]
    std::vector<uint8_t> composite_identity;
    size_t pub_len = signal_buffer_len(pub_buf);
    size_t priv_len = signal_buffer_len(priv_buf);
    composite_identity.push_back((uint8_t)pub_len);
    const uint8_t* pub_data = signal_buffer_data(pub_buf);
    composite_identity.insert(composite_identity.end(), pub_data, pub_data + pub_len);
    const uint8_t* priv_data = signal_buffer_data(priv_buf);
    composite_identity.insert(composite_identity.end(), priv_data, priv_data + priv_len);

    db->setIdentity(composite_identity, regId);

    // --- Generate Signed PreKey ---
    ec_key_pair *signed_prekey_pair = nullptr;
    curve_generate_key_pair(global_context, &signed_prekey_pair);
    
    signal_buffer *signed_pub_buf = nullptr;
    signal_buffer *signed_priv_buf = nullptr;
    ec_public_key_serialize(&signed_pub_buf, ec_key_pair_get_public(signed_prekey_pair));
    ec_private_key_serialize(&signed_priv_buf, ec_key_pair_get_private(signed_prekey_pair));
    
    signal_buffer *signature = nullptr;
    curve_calculate_signature(global_context, &signature, ec_key_pair_get_private(identityPair), signal_buffer_data(signed_pub_buf), signal_buffer_len(signed_pub_buf));
    
    std::vector<uint8_t> composite_signed_prekey;
    composite_signed_prekey.push_back((uint8_t)signal_buffer_len(signed_pub_buf));
    composite_signed_prekey.insert(composite_signed_prekey.end(), signal_buffer_data(signed_pub_buf), signal_buffer_data(signed_pub_buf) + signal_buffer_len(signed_pub_buf));
    composite_signed_prekey.insert(composite_signed_prekey.end(), signal_buffer_data(signed_priv_buf), signal_buffer_data(signed_priv_buf) + signal_buffer_len(signed_priv_buf));
    composite_signed_prekey.push_back((uint8_t)signal_buffer_len(signature));
    composite_signed_prekey.insert(composite_signed_prekey.end(), signal_buffer_data(signature), signal_buffer_data(signature) + signal_buffer_len(signature));
    
    db->storeSignedPreKey(1, composite_signed_prekey);
    
    signal_buffer_free(signed_pub_buf);
    signal_buffer_free(signed_priv_buf);
    signal_buffer_free(signature);
    SIGNAL_UNREF(signed_prekey_pair);

    signal_buffer_free(pub_buf);
    signal_buffer_free(priv_buf);

    // 4. Generate PreKeys (Standard expects ~100 keys)
    for (uint32_t i = 0; i < 100; i++) {
        ec_key_pair *prekey_pair = nullptr;
        curve_generate_key_pair(global_context, &prekey_pair);
        
        signal_buffer *pre_pub = nullptr;
        signal_buffer *pre_priv = nullptr;
        ec_public_key_serialize(&pre_pub, ec_key_pair_get_public(prekey_pair));
        ec_private_key_serialize(&pre_priv, ec_key_pair_get_private(prekey_pair));
        
        std::vector<uint8_t> composite_prekey;
        composite_prekey.push_back((uint8_t)signal_buffer_len(pre_pub));
        composite_prekey.insert(composite_prekey.end(), signal_buffer_data(pre_pub), signal_buffer_data(pre_pub) + signal_buffer_len(pre_pub));
        composite_prekey.insert(composite_prekey.end(), signal_buffer_data(pre_priv), signal_buffer_data(pre_priv) + signal_buffer_len(pre_priv));
        
        db->storePreKey(i + 1, composite_prekey);
        
        signal_buffer_free(pre_pub);
        signal_buffer_free(pre_priv);
        SIGNAL_UNREF(prekey_pair);
    }
    
    std::cout << "[Crypto] Identity generation and serialization into LocalDatabase completed." << std::endl;
    SIGNAL_UNREF(identityPair);

    return true;
}

} // namespace crypto
} // namespace client
} // namespace wizz
