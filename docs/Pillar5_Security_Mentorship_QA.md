# Pillar 5: Security Dimension Q&A

This document summarizes the cybersecurity principles and practices implemented within the WizzMania project to ensure production-grade security for networked C++ applications.

## Q: How do we securely store user passwords, and why was the previous approach flawed?
**A:** Originally, passwords were stored using a basic `std::hash` implementation, which is not cryptographically secure and is vulnerable to reverse engineering and collision attacks. 
We have replaced this with **OpenSSL's SHA-256** hashing algorithm. Furthermore, we implemented **Salted Hashes** by generating a 128-bit random salt (using OpenSSL's `RAND_bytes`) for each user upon creation. This ensures that even if two users have the same password, their stored hashes will look completely different, protecting the database against precomputed "Rainbow Table" attacks.

## Q: How does the server protect itself from malicious clients or Denial of Service (DoS) attacks?
**A:** In a networked environment, a malicious actor might send malformed packets or excessively large payloads attempting to crash the server through Buffer Overflows or memory exhaustion. 
We addressed this early in the packet processing pipeline (in `ClientSession.cpp`) by implementing strict **Buffer Bounds Checking**. Whenever a packet header is read, the server inspects the `Length` field. If a client claims a payload size larger than our defined acceptable limit (a 10MB ceiling), the server immediately drops the connection and discards the buffer. This guarantees the server cannot be forced to allocate massive amounts of memory requested by a bad actor.

## Q: What about network traffic encryption (Packet Sniffing)?
**A:** Currently, the TCP sockets transmit data in plain text. In a production environment, this means anyone on the same network (e.g., public Wi-Fi) could use a packet sniffer like Wireshark to read chat messages or intercept session tokens.
The next architectural step for WizzMania's security roadmap is transitioning from standard `QTcpSocket` and raw OS sockets to **TLS/SSL encryption**. In Qt, this is achieved by upgrading to `QSslSocket` on the client and wrapping the server sockets with OpenSSL `SSL_CTX`, which establishes a secure handshake and encrypts the TCP payload end-to-end.

## Q: Are there any best practices for handling sensitive data in memory, like when typing passwords in the UI?
**A:** Yes. In Qt applications, `QString` is often used for UI input fields. However, `QString` does not guarantee that its underlying memory is immediately zeroed out when the object is destroyed, meaning a password might linger in RAM, vulnerable to a memory dump. For maximum security, sensitive data like passwords should be scrubbed. When passing the password to the hashing function, the plain-text buffer should be explicitly zeroed out (e.g., using functions like `OPENSSL_cleanse` or `SecureZeroMemory`) as soon as the hashing process is complete.
