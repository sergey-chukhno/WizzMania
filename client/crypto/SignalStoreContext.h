#pragma once

#include <signal_protocol.h>
#include "../data/LocalDatabase.h"

namespace wizz {
namespace client {
namespace crypto {

/**
 * @brief Initialize the global Signal Store Context and inject our internal SQLite wrapper
 *        to act as the E2EE persistent state mapping.
 * 
 * @param store_context Out-pointer to the instantiated signal_protocol_store_context
 * @param global_context Active global signal_context instance
 * @param db Our SQLite implementation class instance
 * @return 0 on success, negative on error.
 */
int setupSignalStoreContext(signal_protocol_store_context** store_context, signal_context* global_context, LocalDatabase* db);

} // namespace crypto
} // namespace client
} // namespace wizz
