# Slide 7: Sécurité — Défenses en Profondeur

## 1. Feature Overview
This slide highlights the three layers of defense in WizzMania: **Database Security** (SQLi prevention), **Authentication Security** (Hashing/Salting), and **Transport Security** (TLS).

## 2. How it Works
1. **At Rest**: User credentials and chats are stored in SQLite using parameterized queries.
2. **In Memory**: Passwords never touch the RAM as plain text for longer than a few milliseconds. They are immediately combined with a unique 128-bit salt and hashed.
3. **In Transit**: A 2048-bit SSL certificate secures the connection via TLS 1.2. SSLv2 and SSLv3 are explicitly disabled.

## 3. Why This Code (Rationale)
- **OpenSSL EVP API**: We use the high-level EVP interface rather than low-level SHA functions. EVP is safer and can be easily switched to SHA-3 or other algorithms without rewriting the logic.
- **`sqlite3_bind_text`**: This is the only way to effectively prevent SQL injection in C++. It separates the *command* (the SQL) from the *data* (the username).
- **`RAND_bytes`**: We use a Cryptographically Secure Pseudo-Random Number Generator (CSPRNG) from OpenSSL, not `std::rand()`. This is non-negotiable for security.

## 4. How the Code Implements This
### `DatabaseManager::createUser()`
Uses placeholders (`?`) in the SQL string. The `sqlite3_bind_*` series ensures that if a user names themselves `' OR 1=1; --`, it's treated as a literal string, not a command.
### `hashPassword()`
Combines the password and salt, then runs a SHA-256 digest. The use of a "Salt" prevents **Rainbow Table attacks** (where precomputed hashes are used to crack leaked databases).
### `TcpServer::m_sslContext`
Initialized with `tlsv12`. We explicitly set `no_sslv2` to kill legacy protocols that are vulnerable to attacks like POODLE or BEAST.

## 5. Strategic Analysis

### Advantages
- **Industry Standards**: Using OpenSSL and Prepared Statements brings the project to a professional production-grade level.
- **Privacy**: Even if the `wizzmania.db` file is leaked, attackers cannot immediately decrypt user passwords.

### Drawbacks / Limits
- **CPU Overhead**: TLS handshake and hashing cost CPU cycles. This is the trade-off for security.
- **Certificate Management**: Self-signed certificates (currently used) trigger warnings in browser-like environments. Production would require Let's Encrypt or a Private CA.

### Edge Cases
- **Buffer Overflows**: While using `std::string` mitigates many C-style overflow bugs, we must be careful when passing pointers to OpenSSL (`(unsigned char*)combined.c_str()`).
- **Timing Attacks**: Comparing hashes should ideally use a constant-time comparison to prevent side-channel timing attacks (though SHAnding is usually fast enough to hide this).
   
