# Pillar 5 Mentorship: Security Dimension (Production Grade)

## 1. Password Storage Analysis

**Current State:**
In `DatabaseManager::registerUser` and `DatabaseManager::authenticateUser`, passwords are currently stored and queried in **plain text**.
```cpp
// From DatabaseManager.cpp
std::string sql = "INSERT INTO users (username, password) VALUES ('" + 
                  username + "', '" + password + "');";
```
**Vulnerability:**
Storing passwords in plain text is a critical security flaw. If the SQLite database file (`wizzmania.db`) is leaked or accessed by an unauthorized party, all user accounts are immediately compromised. Furthermore, this code is vulnerable to **SQL Injection** since the strings are simply concatenated into the query instead of parameterized.

**Implemented Solution (OpenSSL & Parameterized Queries):**
- **Hashing & Salting:** We completely removed plain-text storage and the deprecated `std::hash`. We integrated the **OpenSSL `libcrypto`** library directly into the server.
  1. During registration, we use `RAND_bytes()` to generate a cryptographically secure 128-bit random salt.
  2. We combine the password and salt, then hash them using the modern **OpenSSL EVP API (`EVP_sha256`)**. 
  3. We store only the resulting hexadecimal hash and the unique salt in the SQLite database, effectively neutralizing Rainbow Table and brute-force SQL leak attacks.
- **SQL Injection Prevention:** All SQL queries (`registerUser`, `authenticateUser`, `addFriend`, etc.) have been completely rewritten to use `sqlite3_prepare_v2` and `sqlite3_bind_text`. By parameterizing all dynamic inputs, SQL injection vectors are 100% neutralized; the database engine structurally prevents malicious strings from escaping the query boundaries.

## 2. Traffic Encryption Analysis

**Current State:**
The client uses `QTcpSocket` and the server uses native POSIX `socket()` bound to a standard TCP port. All data, including login credentials (username and password), direct messages, and voice notes, is transmitted in **plain text**.

**Vulnerability:**
Any actor on the network (e.g., using Wireshark on a public Wi-Fi network) can intercept, read, and even modify the packets in transit. This exposes user privacy and allows for trivial Man-In-The-Middle (MITM) attacks.

**Implemented Solution (Transport Layer Security - TLS/SSL):**
- **Transition to TLS/SSL:** The networking layer has been fully upgraded to encapsulate the TCP streams in TLS.
  1. **Phase 1 (Server):** Integrated Boost.Asio's SSL capabilities (`asio::ssl::stream`) to wrap the accepted TCP sockets in an `SSL_CTX` context (via `asio::ssl::context::tlsv12`). The server now requires a self-signed local certificate (`server.crt` and `server.key`) for development, which is actively loaded during initialization.
  2. **Phase 2 (Client):** Refactored `QTcpSocket` to use Qt's native `QSslSocket`. To facilitate early development without a recognized CA, the client explicitly ignores self-signed certificate errors, while correctly extracting the binary streams.
  3. **Phase 3 (Production Readiness):** When deploying, developers simply replace the local certificates with proper CA-issued certificates (e.g., Let's Encrypt) and remove the `QSslSocket` error overrides.

## 3. Packet Validation & Memory Safety

**Current State:**
The packet parsing logic reads an integer length from the network stream and dynamically allocates memory for the payload.
```cpp
// Example in NetworkManager::handleAvatarDataPacket
uint32_t len = pkt.readInt();
if (len < 10 * 1024 * 1024) { // 10MB limit
    std::vector<uint8_t> imgData = pkt.readBytes(len);
    // ...
}
```
**Vulnerability:**
While there is a rudimentary 10MB check on avatar payloads, other custom packet receivers might not have strict bounds checking. If an attacker sends a malformed packet with `length = 0xFFFFFFFF`, the `std::vector` constructor will attempt to allocate 4 GB of heap memory. This results in an immediate **Denial of Service (DoS) via Out-Of-Memory (OOM) crash**. Additionally, if integer overflows are unaccounted for during byte reading, it can lead to **Buffer Overflows**.

**Implemented Solution (DoS Mitigation):**
- **Strict Size Bounds:** We have applied hard upper bound limitations on the network receivers. For example, `ClientSession::onDataReceived` now enforces a strict 10MB payload ceiling. If a malignant packet declares a body larger than 10MB, the server aggressively drops the socket, successfully preventing OOM memory exhaustion attacks.
- **Fuzzing:** (Future roadmap) Implement a fuzzer (e.g., using `libFuzzer`) that feeds random, malformed bytes into the packet parser to uncover hidden segmentation faults before deployment.

---

## 4. Future Roadmap: Achieving Industry Standards

To further elevate WizzMania's security posture to match modern production-grade C++ applications, the following enhancements should be considered in future iterations:

### 4.1. Certificate Pinning & Mutual TLS (mTLS)
Currently, the client ignores SSL errors for self-signed certificates. In production, this allows attackers to use spoofed certificates to decrypt traffic.
* **Certificate Pinning:** The client should hardcode the public key hash of the server's authoritative certificate, refusing to connect if the server presents anything else.
* **Mutual TLS (mTLS):** For ultra-secure environments, the server should also verify the *client's* certificate, ensuring only WizzMania official clients can communicate with the backend, effectively locking out unauthorized third-party scrapers or bots.

### 4.2. Rate Limiting & Brute-Force Protection
While we reject oversized payloads to stop OOM crashes, we are currently vulnerable to brute-force credential stuffing and connection flooding.
* **Connection Throttling:** The server should track connection attempts per IP address using a token bucket algorithm to drop aggressive spam traffic from botnets.
* **Account Lockouts:** Implement a database column to track consecutive failed login attempts, temporarily locking the account after 5 failed tries to neutralize brute-force attacks.

### 4.3. Perfect Forward Secrecy (PFS)
If the server's private key (`server.key`) is compromised in the future, an attacker who recorded past TLS traffic could decrypt it simultaneously.
* **Implementation:** We should configure OpenSSL to strictly enforce Ephemeral Diffie-Hellman cipher suites (e.g., `ECDHE-RSA-AES256-GCM-SHA384`). By rotating the symmetric session keys dynamically, past conversations remain permanently encrypted even if the long-term server key is eventually leaked.

### 4.4. Memory Sanitizers & Hardened Builds
C++ is inherently unsafe regarding memory management.
* **Address Sanitizer (ASan):** The CI/CD pipeline should execute the WizzServer automated test suite with Clang's `-fsanitize=address` to mathematically guarantee zero Buffer Overflows, Use-After-Free, or Memory Leaks occur during packet parsing.
* **Executable Hardening:** Compile the production server with Position Independent Executables (`-fPIE`), Stack Canaries (`-fstack-protector-strong`), and strict `RELRO` to mitigate the exploitability of any zero-day vulnerabilities.

---

## Mentor Q&A: Cybersecurity Principles for C++ Network Apps

**Q: Since this is just a messenger project, do we really need TLS?**
**A:** Yes. In modern software engineering, security is not a bolted-on feature; it is fundamental. Teaching or practicing plain-text networking breeds habits that are dangerous in the industry. Think of TLS as the bare minimum entry ticket to deployed applications.

**Q: How do we fix the SQL injection easily?**
**A:** Stop using `std::string::operator+`. SQLite provides a robust C API for prepared statements.
```cpp
// Incorrect / Vulnerable
std::string sql = "SELECT * FROM users WHERE user='" + username + "'";
sqlite3_exec(db, sql.c_str(), ...);

// Correct / Secure
const char* sql = "SELECT * FROM users WHERE user=?";
sqlite3_stmt* stmt;
sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
sqlite3_step(stmt);
```

**Q: What is the most dangerous feature in C++ for networked applications?**
**A:** Manual memory management and unchecked array bounds. The vast majority of CVEs (Common Vulnerabilities and Exposures) are due to Buffer Overflows or Use-After-Free bugs. This is precisely why we enforced RAII (Pillar 3) and must rigorously check packet bounds (Pillar 5).
