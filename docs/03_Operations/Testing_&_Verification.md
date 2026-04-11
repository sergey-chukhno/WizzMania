# WizzMania Automated Test Suite (Verified 🛡️🧪)

## 1. Overview
The WizzMania Test Suite provides a robust framework for verifying the correctness, performance, and security of the entire ecosystem. It leverages **GoogleTest (GTest)** for unit testing and **GoogleMock (GMock)** for behavior verification and dependency isolation.

**Current Status**: 100% Pass Rate (77 / 77 Tests)

---

## 2. Test Inventory

### A. E2EE & Security Core
These tests verify the zero-knowledge infrastructure and cryptographic primitives.
- **`SignalCryptoTest`**: Validates the OpenSSL 3.0 bridge for AES-GCM, HMAC-SHA256, and RNG.
- **`LocalDatabaseTest`**: Verifies client-side persistence of Identity Keys, PreKeys, and Signal Sessions.
- **`ServerIntegrationTest.E2EKeyBundleExchange`**: Verifies the full X3DH bundle upload/fetch handshake between client and server.
- **`ServerIntegrationTest.E2EMessageRelaying`**: Confirms that the server correctly relays encrypted blobs without understanding their content.

### B. Server Logic & Routing
- **`PacketRouterTest`**: Verifies correct dispatching of all supported `PacketType` values to their specific handlers.
- **`SessionManagerTest`**: Validates user online status tracking and broadcast synchronization.
- **`RateLimiterTest`**: Ensures the **Sentinel** protection layer correctly throttles abusive clients.

### C. Game Logic (TileTwister)
- **`GameLogicTest`**: Comprehensive verification of 2048-style grid merging and win conditions.
- **`IntegrationTest`**: Verifies IPC shared-memory synchronization between the game and the messenger.

---

## 3. Execution & Verification

To run the full suite:
```bash
# 1. Build the tests
ninja -C build unit_tests_server unit_tests_client unit_tests_common IntegrationTests TileTwister_Tests

# 2. Run the full suite
ctest --test-dir build --output-on-failure
```

*Note: The suite targets 100% path coverage on all Authentication and Cryptography handlers.*
