#pragma once

#include <signal_protocol.h>
#include <optional>
#include <string>
#include <vector>

namespace wizz {
namespace client {
class LocalDatabase;

namespace crypto {

int setupSignalCryptoProvider(signal_context** global_context);

/**
 * @brief Generates E2EE Identity properties into LocalDatabase if not found.
 */
bool initializeLocalKeys(signal_context* global_context, LocalDatabase* db);

std::optional<std::string> decryptMessage(const std::string& sender, 
                                          const std::vector<uint8_t>& payload, 
                                          LocalDatabase* localDb,
                                          signal_context* globalContext,
                                          signal_protocol_store_context* storeContext);

} // namespace crypto
} // namespace client
} // namespace wizz
