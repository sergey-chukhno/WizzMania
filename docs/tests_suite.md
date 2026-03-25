# WizzMania Automated Test Suite

## Overview
The WizzMania Test Suite provides a robust framework for verifying the correctness, performance, and security of the WizzMania ecosystem. It leverages **GoogleTest (GTest)** for unit testing and **GoogleMock (GMock)** for behavior verification and dependency isolation.

## Current Test Coverage

### 1. Unit Tests (`unit_tests_server`)
Located in `tests/server/`, these tests focus on individual components in isolation:
- **`SessionManager`**: 
    - Verifies session addition, removal, and lookup.
    - Validates user online status transitions and broadacst logic.
- **`PacketRouter`**:
    - Uses GMock to verify that the router correctly dispatches different `PacketType` values to their registered handlers.
    - Ensures that unregistered packet types are handled gracefully without crashing.
- **`Packet` (Common)**:
    - Located in `tests/common/test_packet_gtest.cpp`.
    - Verifies serialization and deserialization of various data types (ints, strings, bodies).
    - Ensures consistency across different playload sizes.

### 2. Integration Tests (`integration_test_server`)
Located in `tests/server/integration_test.cpp`, these tests verify the interaction between multiple server components and external dependencies:
- **SSL Registration Flow**:
    - Perfroms a complete `asio::ssl` handshake between a test client and the server.
    - Validates the `Register` packet flow, including database entry creation and success response validation.
    - Ensures isolated database state using `wizz_test.db`.

## Future Phases & Roadmap

To achieve 100% coverage of WizzMania functionalities, the following phases are planned:

### Phase 1: Authentication & Security Expansion
- **Login Flow**: Verify credential validation and session token generation.
- **Rate Limiting**: Integration tests to ensure the server rejects rapid-fire authentication attempts.
- **Certificate Validation**: Negative tests with expired or invalid SSL certificates.

### Phase 2: Social & Messaging Features
- **Direct Messaging**: Verify end-to-end delivery of message packets between two simulated clients.
- **Friend Requests**: Test the state machine for adding, removing, and blocking contacts.
- **Avatar Persistence**: Integration tests for large binary payload handling during avatar updates.

### Phase 3: Gaming & Real-time Interaction
- **TicTacToe Logic**: Unit tests for game state transitions, win conditions, and move validation.
- **Game Matchmaking**: Verify `GameRoomManager` correctly pairs players and handles disconnects mid-game.

### Phase 4: Stress & Performance Testing
- **Concurrency**: Simulate 100+ simultaneous SSL connections to verify `SessionManager` and `TcpServer` stability under load.
- **Database Contention**: Stress test asynchronous DB tasks to ensure no deadlocks occur during peak activity.

## How to Contribute
When adding new features:
1. Create a corresponding `.cpp` test file in `tests/server/`.
2. Register the test file in `tests/server/CMakeLists.txt`.
3. Verify changes using `ninja -C build && ctest`.
