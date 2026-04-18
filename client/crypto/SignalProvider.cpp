#include "SignalProvider.h"
#include "../data/LocalDatabase.h"
#include <openssl/rand.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <iostream>
#include <ctime>
#include <QDebug>
#include <vector>
#include <session_pre_key.h>
#include <session_cipher.h>
#include <protocol.h>

namespace wizz {
namespace client {
namespace crypto {

static int crypto_random(uint8_t *data, size_t len, void *user_data) {
    return RAND_bytes(data, len) == 1 ? 0 : -1;
}

static int hmac_sha256_init(void **hmac_context, const uint8_t *key, size_t key_len, void *user_data) {
    std::cerr << "[Crypto] HMAC Init - Key len: " << key_len << std::endl;
    HMAC_CTX *ctx = HMAC_CTX_new();
    if (!ctx) return -1;
    if (HMAC_Init_ex(ctx, key, key_len, EVP_sha256(), nullptr) != 1) {
        qWarning() << "[Crypto] HMAC_Init_ex failed!";
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
    if (HMAC_Final(ctx, md, &len) != 1) {
        qWarning() << "[Crypto] HMAC_Final failed!";
        return -1;
    }
    *output = signal_buffer_create(md, len);
    qDebug() << "[Crypto] HMAC Final - Result len:" << len;
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
    qDebug() << "[Crypto] AES Encrypt - Cipher:" << cipher << "KeyLen:" << key_len << "IVLen:" << iv_len << "PlainLen:" << plaintext_len;
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
        qWarning() << "[Crypto] AES Encrypt - Unsupported Cipher/KeyLen combination!";
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
 
    if (EVP_EncryptInit_ex(ctx, evp_cipher, nullptr, key, iv) != 1) {
        qWarning() << "[Crypto] EVP_EncryptInit_ex failed!";
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
 
    if (cipher == SG_CIPHER_AES_CTR_NOPADDING) {
        EVP_CIPHER_CTX_set_padding(ctx, 0);
    }
 
    int outlen1 = 0, outlen2 = 0;
    std::vector<uint8_t> outbuf(plaintext_len + EVP_MAX_BLOCK_LENGTH);
 
    if (EVP_EncryptUpdate(ctx, outbuf.data(), &outlen1, plaintext, plaintext_len) != 1) {
        qWarning() << "[Crypto] EVP_EncryptUpdate failed!";
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
 
    if (EVP_EncryptFinal_ex(ctx, outbuf.data() + outlen1, &outlen2) != 1) {
        qWarning() << "[Crypto] EVP_EncryptFinal_ex failed!";
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
 
    *output = signal_buffer_create(outbuf.data(), outlen1 + outlen2);
    EVP_CIPHER_CTX_free(ctx);
    qDebug() << "[Crypto] AES Encrypt Done - Result ptr:" << *output;
    return (*output != nullptr) ? 0 : -1;
}

static int aes_decrypt(signal_buffer **output, int cipher, const uint8_t *key, size_t key_len, const uint8_t *iv, size_t iv_len, const uint8_t *ciphertext, size_t ciphertext_len, void *user_data) {
    qDebug() << "[Crypto] AES Decrypt - Cipher:" << cipher << "KeyLen:" << key_len << "IVLen:" << iv_len << "DataLen:" << ciphertext_len;
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
        qWarning() << "[Crypto] AES Decrypt - Unsupported Cipher/KeyLen combination!";
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
 
    if (EVP_DecryptInit_ex(ctx, evp_cipher, nullptr, key, iv) != 1) {
        qWarning() << "[Crypto] EVP_DecryptInit_ex failed!";
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
 
    if (cipher == SG_CIPHER_AES_CTR_NOPADDING) {
        EVP_CIPHER_CTX_set_padding(ctx, 0);
    }
 
    int outlen1 = 0, outlen2 = 0;
    std::vector<uint8_t> outbuf(ciphertext_len + EVP_MAX_BLOCK_LENGTH);
 
    if (EVP_DecryptUpdate(ctx, outbuf.data(), &outlen1, ciphertext, ciphertext_len) != 1) {
        qWarning() << "[Crypto] EVP_DecryptUpdate failed!";
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
 
    if (EVP_DecryptFinal_ex(ctx, outbuf.data() + outlen1, &outlen2) != 1) {
        qWarning() << "[Crypto] EVP_DecryptFinal_ex failed!";
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
 
    *output = signal_buffer_create(outbuf.data(), outlen1 + outlen2);
    EVP_CIPHER_CTX_free(ctx);
    qDebug() << "[Crypto] AES Decrypt Done - Result ptr:" << *output;
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
    
    std::vector<uint8_t> pubKey, privKey;
    uint32_t regId = 0;
    
    // Check if we possess an identity and if it's in the correct format
    if (db->getIdentity(pubKey, privKey, regId)) {
        // Try to load a SignedPreKey to verify format
        std::vector<uint8_t> spkBuf;
        if (db->loadSignedPreKey(1, spkBuf) && !spkBuf.empty()) {
            session_signed_pre_key *testRecord = nullptr;
            int res = session_signed_pre_key_deserialize(&testRecord, spkBuf.data(), spkBuf.size(), global_context);
            if (res == 0) {
                SIGNAL_UNREF(testRecord);
                return true; // Format is correct, skip generation
            }
            std::cerr << "[Crypto] STALE FORMAT DETECTED (Error " << res << ") in LocalDatabase. Wiping for fresh generation..." << std::endl;
            db->deleteAllData();
        } else {
            std::cerr << "[Crypto] Identity found but SignedPreKey missing. Forcing regeneration..." << std::endl;
            db->deleteAllData();
        }
        // If we reach here, data is corrupt or old format. Wipe it.
        // For MVP, we can just clear what we need
        // (In a real app, we might handle migration, but here fresh start is safer)
    }

    std::cout << "[Crypto] No keys found in LocalDatabase (or stale format). Generating fresh E2EE Identity..." << std::endl;
    
    // 1. Generate Registration ID
    uint8_t regIDBytes[4];
    crypto_random(regIDBytes, 4, nullptr);
    regId = (*(uint32_t*)regIDBytes) % 16380 + 1; // standard protocol bounds

    // 2. Generate Identity Key (Curve25519)
    ec_key_pair *identityPair = nullptr;
    if (curve_generate_key_pair(global_context, &identityPair) != 0) return false;

    // 3. Serialize and Store Identity
    // We store the raw public and private key bytes safely into separate columns to avoid any slicing/corruption.
    signal_buffer *pub_buf = nullptr;
    signal_buffer *priv_buf = nullptr;
    ec_public_key_serialize(&pub_buf, ec_key_pair_get_public(identityPair));
    ec_private_key_serialize(&priv_buf, ec_key_pair_get_private(identityPair));

    std::vector<uint8_t> pub_vec(signal_buffer_data(pub_buf), signal_buffer_data(pub_buf) + signal_buffer_len(pub_buf));
    std::vector<uint8_t> priv_vec(signal_buffer_data(priv_buf), signal_buffer_data(priv_buf) + signal_buffer_len(priv_buf));

    db->setIdentity(pub_vec, priv_vec, regId);

    // --- Generate Signed PreKey using libsignal's own record format ---
    ec_key_pair *signed_prekey_pair = nullptr;
    curve_generate_key_pair(global_context, &signed_prekey_pair);

    signal_buffer *signature = nullptr;
    signal_buffer *signed_pub_buf = nullptr;
    ec_public_key_serialize(&signed_pub_buf, ec_key_pair_get_public(signed_prekey_pair));
    
    // CRITICAL FIX: Extraction and Alignment. 
    // curve_calculate_signature expects an ec_private_key* object.
    // Serialization adds 0x05 prefix (33-bytes). We ensure we have 32-bytes raw entropy.
    const uint8_t* raw_priv = signal_buffer_data(priv_buf);
    size_t raw_priv_len = signal_buffer_len(priv_buf);
    if (raw_priv_len == 33 && raw_priv[0] == 0x05) { raw_priv++; raw_priv_len = 32; }

    ec_private_key *signing_key_obj = nullptr;
    if (curve_decode_private_point(&signing_key_obj, raw_priv, raw_priv_len, global_context) != 0) {
        std::cerr << "[Crypto] FAILED to decode private key entropy for signing!" << std::endl;
        return false;
    }

    int sign_res = curve_calculate_signature(global_context, &signature,
                              signing_key_obj,
                              signal_buffer_data(signed_pub_buf), signal_buffer_len(signed_pub_buf));
    
    SIGNAL_UNREF(signing_key_obj); // Free the temporary object

    if (sign_res != 0) {
        std::cerr << "[Crypto] FAILED to calculate signature! Result: " << sign_res << std::endl;
        return false;
    }

    // --- CRYPTO PARITY CHECK: Verify the signature we just created ---
    int verify_status = curve_verify_signature(ec_key_pair_get_public(identityPair),
                                              signal_buffer_data(signed_pub_buf), signal_buffer_len(signed_pub_buf),
                                              signal_buffer_data(signature), signal_buffer_len(signature));
    
    std::cerr << "[Crypto] Generation Status:"
              << "\n[Crypto]   Identity Pub Len: " << pub_vec.size()
              << "\n[Crypto]   Identity Priv (Raw) Len: " << raw_priv_len
              << "\n[Crypto]   Signature Len: " << signal_buffer_len(signature)
              << "\n[Crypto]   LOCAL VERIFICATION (Internal): " << (verify_status == 1 ? "SUCCESS" : "FAILED (Result " + std::to_string(verify_status) + ")")
              << std::endl;

    // Create the signed prekey record the way libsignal expects
    session_signed_pre_key *signed_prekey_record = nullptr;
    session_signed_pre_key_create(&signed_prekey_record, 1, (uint64_t)time(nullptr),
                                   signed_prekey_pair,
                                   signal_buffer_data(signature), signal_buffer_len(signature));

    signal_buffer *spk_serialized = nullptr;
    session_signed_pre_key_serialize(&spk_serialized, signed_prekey_record);

    std::vector<uint8_t> spk_blob(signal_buffer_data(spk_serialized),
                                   signal_buffer_data(spk_serialized) + signal_buffer_len(spk_serialized));
    db->storeSignedPreKey(1, spk_blob);

    signal_buffer_free(signed_pub_buf);
    signal_buffer_free(signature);
    signal_buffer_free(spk_serialized);
    SIGNAL_UNREF(signed_prekey_record);
    SIGNAL_UNREF(signed_prekey_pair);

    signal_buffer_free(pub_buf);
    signal_buffer_free(priv_buf);

    // 4. Generate PreKeys using libsignal's own record format (100 keys)
    for (uint32_t i = 1; i <= 100; i++) {
        ec_key_pair *prekey_pair = nullptr;
        curve_generate_key_pair(global_context, &prekey_pair);

        session_pre_key *prekey_record = nullptr;
        session_pre_key_create(&prekey_record, i, prekey_pair);

        signal_buffer *pk_serialized = nullptr;
        session_pre_key_serialize(&pk_serialized, prekey_record);

        std::vector<uint8_t> pk_blob(signal_buffer_data(pk_serialized),
                                      signal_buffer_data(pk_serialized) + signal_buffer_len(pk_serialized));
        db->storePreKey(i, pk_blob);

        signal_buffer_free(pk_serialized);
        SIGNAL_UNREF(prekey_record);
        SIGNAL_UNREF(prekey_pair);
    }
    
    std::cout << "[Crypto] Identity generation and serialization into LocalDatabase completed." << std::endl;
    SIGNAL_UNREF(identityPair);

    return true;
}
std::optional<std::string> decryptMessage(const std::string& sender, const std::vector<uint8_t>& payload, 
LocalDatabase* localDb,
signal_context* globalContext, 
signal_protocol_store_context* storeContext) {
    
    if (payload.empty()) return std::nullopt;

    const signal_protocol_address address = {sender.c_str(), static_cast<size_t> (sender.length()), 1};

    session_cipher *cipher = nullptr;
    session_cipher_create(&cipher, storeContext, &address, globalContext); 

    signal_buffer *plaintext = nullptr; 
    int res = -1; 

    //Phase 1: Try Native SignalMessage
    signal_message *sm = nullptr; 
    int dRes = signal_message_deserialize(&sm, payload.data(), payload.size(), globalContext); 

    if (dRes == 0 && sm) {
        res = session_cipher_decrypt_signal_message(cipher, sm, nullptr, &plaintext);
        SIGNAL_UNREF(sm);
    } else {
        pre_key_signal_message *pm = nullptr;
        dRes = pre_key_signal_message_deserialize(&pm, payload.data(), payload.size(), globalContext);
        
        if (dRes == 0 && pm) {
            res = session_cipher_decrypt_pre_key_signal_message(cipher, pm, nullptr, &plaintext);
            SIGNAL_UNREF(pm);
        } else {
            // Fatal Rejection: Wipe invalid channel
            if (dRes == -1100 || dRes == SG_ERR_INVALID_MESSAGE) {
                std::string addStr = sender + ":1";
                localDb->deleteSession(addStr);
            }
        }
    }
        
    session_cipher_free(cipher); 
    
    if (res == 0 && plaintext) {
        std::string decrypted(reinterpret_cast<const char*>(signal_buffer_data(plaintext)), 
                              signal_buffer_len(plaintext));
        signal_buffer_free(plaintext);
        return decrypted;
    }
    
    return std::nullopt;
}

} // namespace crypto
} // namespace client
} // namespace wizz
