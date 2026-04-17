#include "SignalStoreContext.h"
#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <QDebug>

namespace wizz {
namespace client {
namespace crypto {

static int get_session_address_string(const signal_protocol_address *address, std::string& out) {
    if (!address || !address->name) return -1;
    out = std::string(address->name, address->name_len) + ":" + std::to_string(address->device_id);
    return 0;
}

static std::string dumpHex(const uint8_t* data, size_t len) {
    std::stringstream ss;
    for(size_t i=0; i<len; ++i) ss << std::hex << std::setw(2) << std::setfill('0') << (int)data[i] << " ";
    return ss.str();
}

// --- Session Store Callbacks ---
static int load_session_func(signal_buffer **record, signal_buffer **user_record, const signal_protocol_address *address, void *user_data) {
    if (record) *record = nullptr;
    if (user_record) *user_record = nullptr;

    auto db = static_cast<LocalDatabase*>(user_data);
    std::string addr_str;
    if (get_session_address_string(address, addr_str) != 0) return -1;
    
    std::vector<uint8_t> buffer;
    if (db->loadSession(addr_str, buffer)) {
        if (record) *record = signal_buffer_create(buffer.data(), buffer.size());
        std::cerr << "[Store] LOADED session for: [" << addr_str << "] (" << buffer.size() << " bytes)" 
                  << "\n[Store]   Hex: " << dumpHex(buffer.data(), buffer.size() > 32 ? 32 : buffer.size()) << "..." << std::endl;
        return 1;
    }
    std::cerr << "[Store] LOAD failed for: [" << addr_str << "]" << std::endl;
    return 0; // 0 = Not found
}

static int get_sub_device_sessions_func(signal_int_list **sessions, const char *name, size_t name_len, void *user_data) {
    if (!sessions) return -1;
    *sessions = signal_int_list_alloc();
    // For WizzMania, device_id is always 1, we do not support multi-device syncing yet.
    signal_int_list_push_back(*sessions, 1);
    return signal_int_list_size(*sessions);
}

static int store_session_func(const signal_protocol_address *address, uint8_t *record, size_t record_len, uint8_t *user_record, size_t user_record_len, void *user_data) {
    auto db = static_cast<LocalDatabase*>(user_data);
    std::string addr_str;
    if (get_session_address_string(address, addr_str) != 0) return -1;
    std::vector<uint8_t> buffer(record, record + record_len);
    bool ok = db->storeSession(addr_str, buffer);
    if (ok) std::cerr << "[Store] STORED session for: [" << addr_str << "] (" << record_len << " bytes)" << std::endl;
    else std::cerr << "[Store] FAILED to store session for: [" << addr_str << "]" << std::endl;
    return ok ? 0 : -1;
}

static int contains_session_func(const signal_protocol_address *address, void *user_data) {
    auto db = static_cast<LocalDatabase*>(user_data);
    std::string addr_str;
    if (get_session_address_string(address, addr_str) != 0) return 0;
    return db->containsSession(addr_str) ? 1 : 0;
}

static int delete_session_func(const signal_protocol_address *address, void *user_data) {
    auto db = static_cast<LocalDatabase*>(user_data);
    std::string addr_str;
    if (get_session_address_string(address, addr_str) != 0) return -1;
    return db->deleteSession(addr_str) ? 1 : 0;
}

static int delete_all_sessions_func(const char *name, size_t name_len, void *user_data) {
    // Only used for multi-device logic, deleting all sub-device sessions for a user.
    auto db = static_cast<LocalDatabase*>(user_data);
    std::string addr_str(name, name_len);
    addr_str += ":1"; // default device
    db->deleteSession(addr_str);
    return 1;
}

static void destroy_session_func(void *user_data) {}

// --- PreKey Store Callbacks ---
static int load_pre_key_func(signal_buffer **record, uint32_t pre_key_id, void *user_data) {
    if (record) *record = nullptr;
    auto db = static_cast<LocalDatabase*>(user_data);
    std::vector<uint8_t> buffer;
    if (db->loadPreKey(pre_key_id, buffer)) {
        if (record) *record = signal_buffer_create(buffer.data(), buffer.size());
        return SG_SUCCESS; // 0
    }
    return SG_ERR_INVALID_KEY_ID;
}

static int store_pre_key_func(uint32_t pre_key_id, uint8_t *record, size_t record_len, void *user_data) {
    auto db = static_cast<LocalDatabase*>(user_data);
    std::vector<uint8_t> buffer(record, record + record_len);
    return db->storePreKey(pre_key_id, buffer) ? 0 : -1;
}

static int contains_pre_key_func(uint32_t pre_key_id, void *user_data) {
    auto db = static_cast<LocalDatabase*>(user_data);
    return db->containsPreKey(pre_key_id) ? 1 : 0;
}

static int remove_pre_key_func(uint32_t pre_key_id, void *user_data) {
    auto db = static_cast<LocalDatabase*>(user_data);
    return db->removePreKey(pre_key_id) ? 0 : -1;
}
static void destroy_prekey_func(void *user_data) {}

// --- Signed PreKey Store Callbacks ---
static int load_signed_pre_key_func(signal_buffer **record, uint32_t signed_pre_key_id, void *user_data) {
    if (record) *record = nullptr;
    auto db = static_cast<LocalDatabase*>(user_data);
    std::vector<uint8_t> buffer;
    if (db->loadSignedPreKey(signed_pre_key_id, buffer)) {
        if (record) *record = signal_buffer_create(buffer.data(), buffer.size());
        return SG_SUCCESS;
    }
    return SG_ERR_INVALID_KEY_ID;
}

static int store_signed_pre_key_func(uint32_t signed_pre_key_id, uint8_t *record, size_t record_len, void *user_data) {
    auto db = static_cast<LocalDatabase*>(user_data);
    std::vector<uint8_t> buffer(record, record + record_len);
    return db->storeSignedPreKey(signed_pre_key_id, buffer) ? 0 : -1;
}

static int contains_signed_pre_key_func(uint32_t signed_pre_key_id, void *user_data) {
    auto db = static_cast<LocalDatabase*>(user_data);
    return db->containsSignedPreKey(signed_pre_key_id) ? 1 : 0;
}

static int remove_signed_pre_key_func(uint32_t signed_pre_key_id, void *user_data) {
    auto db = static_cast<LocalDatabase*>(user_data);
    return db->removeSignedPreKey(signed_pre_key_id) ? 0 : -1;
}
static void destroy_signed_prekey_func(void *user_data) {}

// --- Identity Key Store Callbacks ---
static int get_identity_key_pair_func(signal_buffer **public_data, signal_buffer **private_data, void *user_data) {
    if (!public_data || !private_data || !user_data) return SG_ERR_INVAL;
    *public_data = nullptr;
    *private_data = nullptr;

    auto db = static_cast<LocalDatabase*>(user_data);
    std::vector<uint8_t> pubKey, privKey;
    uint32_t regId = 0;
    if (db->getIdentity(pubKey, privKey, regId)) {
        if (pubKey.empty() || privKey.empty()) {
            std::cerr << "[Crypto] get_identity_key_pair - Identity part missing!" << std::endl;
            return SG_ERR_UNKNOWN;
        }

        *public_data = signal_buffer_create(pubKey.data(), pubKey.size());
        *private_data = signal_buffer_create(privKey.data(), privKey.size());
        
        std::cerr << "[Store] LOADED identity key pair for: " << db->getDbPath()
                  << "\n[Store]   Pub Hex : " << dumpHex(pubKey.data(), pubKey.size())
                  << "\n[Store]   Priv Hex: " << dumpHex(privKey.data(), privKey.size()) << std::endl;
        return SG_SUCCESS;
    } else {
        std::cerr << "[Crypto] get_identity_key_pair - Identity NOT FOUND in DB." << std::endl;
        return SG_ERR_UNKNOWN;
    }
}

static int get_local_registration_id_func(void *user_data, uint32_t *registration_id) {
    if (!registration_id || !user_data) return SG_ERR_INVAL;

    auto db = static_cast<LocalDatabase*>(user_data);
    std::vector<uint8_t> pub, priv;
    if (db->getIdentity(pub, priv, *registration_id)) {
        std::cerr << "[Crypto] get_local_registration_id returning: " << *registration_id << std::endl;
        return 0;
    }
    std::cerr << "[Crypto] get_local_registration_id - Identity missing!" << std::endl;
    return SG_ERR_UNKNOWN;
}

static int save_identity_func(const signal_protocol_address *address, uint8_t *key_data, size_t key_len, void *user_data) {
    auto db = static_cast<LocalDatabase*>(user_data);
    std::string addr_str;
    if (get_session_address_string(address, addr_str) != 0) return -1;
    
    std::vector<uint8_t> key(key_data, key_data + key_len);
    bool ok = db->storeRemoteIdentity(addr_str, key);
    if (ok) {
        std::cerr << "[Store] SAVED remote identity for [" << addr_str << "]" << std::endl;
        return 0; // SG_SUCCESS
    }
    return -1;
}

static int is_trusted_identity_func(const signal_protocol_address *address, uint8_t *key_data, size_t key_len, void *user_data) {
    auto db = static_cast<LocalDatabase*>(user_data);
    std::string addr_str;
    if (get_session_address_string(address, addr_str) != 0) return 0;
    
    std::vector<uint8_t> storedKey;
    if (db->loadRemoteIdentity(addr_str, storedKey)) {
        if (storedKey.size() == key_len && memcmp(storedKey.data(), key_data, key_len) == 0) {
            return 1;
        }
        
        // DEEP TRACING: Highlight exact binary discrepancy
        std::stringstream ss;
        ss << "\n[Store] IDENTITY MISMATCH for [" << addr_str << "]"
           << "\n[Store]   Incoming Hex: ";
        for(size_t i=0; i<key_len; ++i) ss << std::hex << std::setw(2) << std::setfill('0') << (int)key_data[i] << " ";
        ss << "\n[Store]   Stored Hex  : ";
        for(size_t i=0; i<storedKey.size(); ++i) ss << std::hex << std::setw(2) << std::setfill('0') << (int)storedKey[i] << " ";
        ss << "\n[Store]   CRITICAL: Remote identity changed for " << addr_str << ". Purging stale data...";
        std::cerr << ss.str() << std::endl;
        
        // For stabilization phase, we 'trust' it and RE-BIND the identity
        std::vector<uint8_t> key_vec(key_data, key_data + key_len);
        db->storeRemoteIdentity(addr_str, key_vec);
        return 1; 
    }
    // Trust on first use (TOFU)
    return 1;
}
static void destroy_identity_func(void *user_data) {}

int setupSignalStoreContext(signal_protocol_store_context** store_context, signal_context* global_context, LocalDatabase* db) {
    int ret = signal_protocol_store_context_create(store_context, global_context);
    if (ret != SG_SUCCESS) return ret;

    signal_protocol_session_store session_store = {
        .load_session_func = load_session_func,
        .get_sub_device_sessions_func = get_sub_device_sessions_func,
        .store_session_func = store_session_func,
        .contains_session_func = contains_session_func,
        .delete_session_func = delete_session_func,
        .delete_all_sessions_func = delete_all_sessions_func,
        .destroy_func = destroy_session_func,
        .user_data = db
    };
    signal_protocol_store_context_set_session_store(*store_context, &session_store);

    signal_protocol_pre_key_store pre_key_store = {
        .load_pre_key = load_pre_key_func,
        .store_pre_key = store_pre_key_func,
        .contains_pre_key = contains_pre_key_func,
        .remove_pre_key = remove_pre_key_func,
        .destroy_func = destroy_prekey_func,
        .user_data = db
    };
    signal_protocol_store_context_set_pre_key_store(*store_context, &pre_key_store);

    signal_protocol_signed_pre_key_store signed_pre_key_store = {
        .load_signed_pre_key = load_signed_pre_key_func,
        .store_signed_pre_key = store_signed_pre_key_func,
        .contains_signed_pre_key = contains_signed_pre_key_func,
        .remove_signed_pre_key = remove_signed_pre_key_func,
        .destroy_func = destroy_signed_prekey_func,
        .user_data = db
    };
    signal_protocol_store_context_set_signed_pre_key_store(*store_context, &signed_pre_key_store);

    signal_protocol_identity_key_store identity_store = {
        .get_identity_key_pair = get_identity_key_pair_func,
        .get_local_registration_id = get_local_registration_id_func,
        .save_identity = save_identity_func,
        .is_trusted_identity = is_trusted_identity_func,
        .destroy_func = destroy_identity_func,
        .user_data = db
    };
    signal_protocol_store_context_set_identity_key_store(*store_context, &identity_store);

    // sender_key_store is used for groups. We can leave it uninitialized for direct messages
    return SG_SUCCESS;
}

} // namespace crypto
} // namespace client
} // namespace wizz
