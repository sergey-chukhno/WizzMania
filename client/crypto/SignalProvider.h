#pragma once

#include <signal_protocol.h>

namespace wizz {
namespace client {
class LocalDatabase;

namespace crypto {

int setupSignalCryptoProvider(signal_context** global_context);

/**
 * @brief Generates E2EE Identity properties into LocalDatabase if not found.
 */
bool initializeLocalKeys(signal_context* global_context, LocalDatabase* db);

} // namespace crypto
} // namespace client
} // namespace wizz
