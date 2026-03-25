# Slide 4: Architecture du Serveur (Deep Dive)

## 1. Feature Overview
This slide focuses on the backend's core: `TcpServer` (the acceptor), `ClientSession` (the pipe), and `DatabaseManager` (the state).

## 2. How it Works
The server is a non-blocking, event-driven machine:
1. **Acceptor**: `TcpServer` listens for new connections.
2. **Session**: Each connection is a `ClientSession` object that lives as long as the socket is open.
3. **Actor**: Database requests are "posted" to a background worker thread so the network thread is never blocked by slow disk I/O.

## 3. Why This Code (Rationale)
- **`std::enable_shared_from_this`**: Used in `ClientSession` to ensure the session object doesn't get deleted while an asynchronous operation (like `async_read`) is still pending. This is the gold standard for Asio safety.
- **Actor Pattern**: We use a `std::condition_variable` and a task queue. This prevents "Head-of-Line Blocking" where a slow SQL query would pause all chat messages for all users.

## 4. How the Code Implements This
### `TcpServer::doAccept()`
It uses **Recursive Asynchronous Calls**. After accepting a socket, it immediately calls itself again. This ensures the server is always ready for the next client.
### `ClientSession::doRead()`
It reads data into a buffer and then passes it to `onDataReceived`. This method handles **Packet Fragmentation** (if half a packet arrives, it waits for the rest).
### `DatabaseManager::workerLoop()`
A classic producer-consumer loop. It sleeps on the `condition_variable` until `postTask()` is called.

## 5. Strategic Analysis

### Advantages
- **High Concurrency**: Can handle 10,000+ connections with very few threads.
- **Reliability**: Member buffers in `ClientSession` prevent thread-safety bugs common in static buffer designs.

### Drawbacks / Limits
- **Memory Overhead**: Each session has its own buffer. With 100,000 users, this could consume significant RAM (solved via smaller initial buffers or dynamic resizing).

### Edge Cases
- **Slow Clients**: If a client receives slowly, the `m_outbox` queue in `ClientSession` could grow indefinitely. We need "Backpressure" (dropping connections if they lag too much).
- **Graceful Shutdown**: The `workerLoop` must be stopped correctly by setting `m_stopWorker` and notifying the CV, otherwise the program will hang on exit.
