# Pillar 3 Mentorship Q&A: Clean Code & Data Structures

## Topic 1: Separation of Concerns (SoC) & SRP

**Q: Why was `TcpServer::run()` refactored to extract lambda functions?**
**A:** Prior to our refactoring, the `TcpServer`'s main event loop was cluttered with massive lambda callbacks handling specific message protocols (`OnLogin`, `OnVoiceMessage`, etc.). This violated the Single Responsibility Principle (SRP) and Separation of Concerns (SoC). The event loop should *only* be responsible for multiplexing sockets and routing network events. By extracting the business logic into distinct handler methods (`handleLogin`, `handleVoiceMessage`), the code becomes much easier to read, test in isolation, and maintain.

**Q: How did `NetworkManager` violate the Open-Closed Principle (OCP)?**
**A:** `NetworkManager::onReadyRead()` consisted of a single massive `if-else if` block that tried to dispatch based on enum `PacketType`. Every time a new packet type was added, the core packet parsing logic had to be modified. OCP states a module should be open for extension but closed for modification. We resolved this by introducing a Dispatch Table (`QHash` of lambda handlers). Now, adding a new packet type requires only registering a new handler during initialization—not modifying the core reader.

## Topic 2: Deterministic Resource Management & RAII

**Q: What is RAII, and why did we apply it to `AudioManager`?**
**A:** Resource Acquisition Is Initialization (RAII) is a C++ idiom guaranteeing that resources are tied to object lifetime. If an object is destroyed or goes out of scope, the resource is automatically released. `AudioManager` manually managed `QAudioSource` and `QAudioSink` using naked `new` and `delete`. This is prone to memory leaks (e.g., if an exception occurs before `delete` is called). We upgraded the code to use modern C++ smart pointers (`std::unique_ptr`), which implicitly destroys the child when the parent dies, eliminating memory lifecycle bugs. We did similarly with `QMetaObject::Connection` in `ChatWindow` using `std::shared_ptr`.

## Topic 3: Big-O Complexity of Existing Data Structures

**Q: In `TcpServer`, we use `std::unordered_map` for `m_onlineUsers`. What is its complexity, and why wasn't `std::map` used?**
**A:** `std::unordered_map` is implemented as a Hash Table. 
- **Insertion:** `O(1)` average, `O(N)` worst case (if hash collisions rehash everything).
- **Lookup/Search:** `O(1)` average.
For a server managing thousands of connected users, fast `O(1)` lookups to check a socket by username are critical for throughput. `std::map`, on the other hand, is generally implemented as a Red-Black Tree. Its insertions and lookups are `O(log N)` guaranteed. We don't need the usernames sorted alphabetically during routing, so paying the `O(log N)` penalty for tree traversal just to route a direct message would be suboptimal.

**Q: We use `std::vector` to return offline messages from `DatabaseManager`. Why `std::vector` instead of `std::list`?**
**A:** `std::vector` has contiguous memory layout. This makes it incredibly cache-friendly for CPU caches during iteration (spatial locality). 
- **Iteration:** Ultra-fast, predictable memory access. 
- **Appending (`push_back`):** `O(1)` amortized.
Unless you are frequently inserting or removing elements from the *middle* of a collection, `std::vector` is almost always the right default choice in Modern C++ over linked structures like `std::list`.
