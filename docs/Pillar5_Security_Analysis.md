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

**Production Grade Strategy:**
- **Hashing & Salting:** Passwords must never be stored in plain text. When a user registers, generate a cryptographically secure random salt. Hash the password combined with the salt using a modern algorithm like **Argon2**, **bcrypt**, or **PBKDF2**. Store only the salt and the resulting hash.
- **Parameterized Queries:** Use `sqlite3_prepare_v2` and `sqlite3_bind_text` instead of string concatenation to completely eliminate SQL injection vectors.

## 2. Traffic Encryption Analysis

**Current State:**
The client uses `QTcpSocket` and the server uses native POSIX `socket()` bound to a standard TCP port. All data, including login credentials (username and password), direct messages, and voice notes, is transmitted in **plain text**.

**Vulnerability:**
Any actor on the network (e.g., using Wireshark on a public Wi-Fi network) can intercept, read, and even modify the packets in transit. This exposes user privacy and allows for trivial Man-In-The-Middle (MITM) attacks.

**Production Grade Strategy:**
- **Transition to TLS/SSL:** The networking layer must be upgraded to encapsulate the TCP stream in TLS.
  - On the client side: Upgrade `QTcpSocket` to `QSslSocket`.
  - On the server side: Integrate a library like **OpenSSL** or **Botan** to wrap the accepted POSIX sockets in a TLS context. Certificates must be provisioned (e.g., via Let's Encrypt) and validated by the client to ensure server authenticity.

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

**Production Grade Strategy:**
- **Strict Size Bounds:** Apply a hard upper bound limitation on the `readBytes()` and `readString()` primitives at the `wizz::Packet` class level.
- **Fuzzing:** Implement a fuzzer (e.g., using `libFuzzer`) that feeds random, malformed bytes into the packet parser to uncover hidden segmentation faults or unhandled exceptions before deployment.

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
