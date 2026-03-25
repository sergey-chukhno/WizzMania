# Pillar 5: Security Dimension Q&A

This document summarizes the cybersecurity principles and practices implemented within the WizzMania project to ensure production-grade security for networked C++ applications.

## Q: How do we securely store user passwords, and why was the previous approach flawed?
**A:** Originally, passwords were stored using a basic `std::hash` implementation, which is not cryptographically secure and is vulnerable to reverse engineering and collision attacks. 
We have replaced this with **OpenSSL's SHA-256** hashing algorithm. Furthermore, we implemented **Salted Hashes** by generating a 128-bit random salt (using OpenSSL's `RAND_bytes`) for each user upon creation. This ensures that even if two users have the same password, their stored hashes will look completely different, protecting the database against precomputed "Rainbow Table" attacks.

## Q: How does the server protect itself from malicious clients or Denial of Service (DoS) attacks?
**A:** In a networked environment, a malicious actor might send malformed packets or excessively large payloads attempting to crash the server through Buffer Overflows or memory exhaustion. 
We addressed this early in the packet processing pipeline (in `ClientSession.cpp`) by implementing strict **Buffer Bounds Checking**. Whenever a packet header is read, the server inspects the `Length` field. If a client claims a payload size larger than our defined acceptable limit (a 10MB ceiling), the server immediately drops the connection and discards the buffer. This guarantees the server cannot be forced to allocate massive amounts of memory requested by a bad actor.

## Q: What about network traffic encryption (Packet Sniffing)?
**A:** Previously, the TCP sockets transmitted data in plain text. In a production environment, this meant anyone on the same network (e.g., public Wi-Fi) could use a packet sniffer like Wireshark to read chat messages or intercept session tokens.
To secure our communications, we successfully transitioned our networking layer to **TLS/SSL encryption**. On the client, we upgraded to `QSslSocket` which natively supports encrypted connections. On the server, we integrated OpenSSL to wrap the Boost.Asio TCP sockets in an `SSL_CTX` stream (`asio::ssl::stream`). This established a secure handshake and now encrypts the TCP payload end-to-end, completely neutralizing packet sniffing threats.

## Q: Are there any best practices for handling sensitive data in memory, like when typing passwords in the UI?
**A:** Yes. In Qt applications, `QString` is often used for UI input fields. However, `QString` does not guarantee that its underlying memory is immediately zeroed out when the object is destroyed, meaning a password might linger in RAM, vulnerable to a memory dump. For maximum security, sensitive data like passwords should be scrubbed. When passing the password to the hashing function, the plain-text buffer should be explicitly zeroed out (e.g., using functions like `OPENSSL_cleanse` or `SecureZeroMemory`) as soon as the hashing process is complete.

## Q: Why did our server crash with `asio::ssl::error::stream_truncated` after we implemented TLS?
**A:** In standard TCP using Boost.Asio, it is sometimes technically forgiving (though bad practice) if concurrent `async_write` operations overlap. However, an `asio::ssl::stream` acts as a state machine managing encrypted records. If two concurrent async handlers attempt to write to the SSL stream simultaneously without sequencing, the encrypted blocks become interleaved and corrupted. Once the client receives a corrupted SSL frame, it drops the connection, causing a `stream_truncated` fatal error on the server. We resolved this by implementing an `m_outbox` (`std::deque`) in `ClientSession` that queues packets and flushes them efficiently one-by-one.

## Q: Why did the client's friend list stop displaying contact statuses after layering TLS?
**A:** We encountered a **Race Condition**. After sending the "Login Success" packet, the server immediately fired the "ContactList" packet containing friends and their statuses over the fast SSL stream. Back on the client, the `NetworkManager` received the "ContactList" packet *before* the UI `MainWindow` had finished instantiation or hooked up its Qt Signal/Slot listeners. Because nobody was listening to the generated signal, the payload was essentially dropped. We fixed this by introducing a state-caching mechanism in `NetworkManager` so `MainWindow` can safely query the initial contacts via a direct getter during constructor execution.

## Q: What actions do remote collaborators need to take to run the project after pulling these TLS changes?
**A:** Because we are using Transport Layer Security locally without an external Certificate Authority, remote collaborators must independently generate their own self-signed certificates for their local server instance to bind to.

**Required Steps for Collaborators:**
1. Open a terminal and navigate to the project root directory.
2. Create the expected certificates directory: `mkdir -p server/certs`
3. Generate the RSA key and X.509 certificate using OpenSSL:
   ```bash
   openssl req -newkey rsa:2048 -nodes -keyout server/certs/server.key -x509 -days 365 -out server/certs/server.crt -subj "/C=US/ST=State/L=City/O=Organization/CN=localhost"
   ```
4. Rebuild the server and client using CMake as usual. The server will now map `server.crt` and `server.key` upon launch and function identical to the pre-TLS era.
