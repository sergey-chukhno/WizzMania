# Slide 10: Tests Automatisés & Fiabilité

## 1. Feature Overview
This slide showcases WizzMania's "Defensive Programming" mindset. It presents the **GoogleTest (GTest)** and **GoogleMock (GMock)** integration.

## 2. How it Works
- **Unit Tests**: Test individual classes (e.g., "Does the Packet protocol correctly read an integer?").
- **Integration Tests**: Test the full chain (Client -> SSL -> Server -> SQLite).
- **Mocks**: We create "Fake" versions of the Server or Database to test components in isolation.

## 3. Why This Code (Rationale)
- **`FetchContent`**: We don't ask users to install GTest. CMake downloads and builds it automatically. This ensures consistency across WSL, Mac, and Linux.
- **GMock**: Mocking allows us to test "Error Paths" (like a Database failure) that are hard to trigger in real life.

## 4. How the Code Implements This
### `TEST(Router, Dispatches)`
This test creates a `MockHandler` and *expects* one specific call. If the `PacketRouter` doesn't call that handler when it should, the test fails. This is **TDD (Test Driven Development)** at its best.
### `IntegrationTest.RegistrationFlow`
This is a "Black Box" test. It starts a real server on a random port, connects a real TLS client, and checks if a new user is actually created in the `wizzmania.db` file.

## 5. Strategic Analysis

### Advantages
- **Regession Safety**: We can refactor the `TcpServer` logic and instantly know if we broke the Login flow.
- **Confidence**: 0 failures in the test suite means the binary protocol is stable.

### Drawbacks / Limits
- **Maintenance**: Every time a packet field changes, the tests must be updated. This is a "Tax" on development speed for the benefit of quality.
- **Complexity of Network Tests**: Testing SSL handshakes in a CI/C environment requires managing certificates correctly.

### Edge Cases
- **Port Collisions**: If the integration test tries to bind to port 8080 while the real server is running, it will fail. (Fix: Use port 0 for auto-assignment).
- **Timeouts**: Async tests need a "Watchdog" timer. If the server never responds, the test shouldn't hang forever.
