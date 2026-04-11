# Pillar 5 Mentorship: Security Dimension (Archived)

## 1. Password Storage Analysis
[Historical Context: This document traces the evolution from plaintext password storage to our current OpenSSL-backed EVP_sha256 hashing architecture.]

**Current State (Historical):**
In early versions, passwords were stored and queried in **plain text**.
```cpp
// From DatabaseManager.cpp
std::string sql = "INSERT INTO users (username, password) VALUES ('" + 
                  username + "', '" + password + "');";
```
**Vulnerability:**
Critical security flaw. Unified compromise of all accounts if the DB file was leaked. Vulnerable to SQL Injection.

**Implemented Solution:**
- **Hashing & Salting:** Integrated **OpenSSL `libcrypto`**.
- **SQL Injection Prevention:** Parameterized queries using `sqlite3_prepare_v2`.

[... See documentation/01_Architecture/Security.md for current implementation details ...]

---

## 4. Future Roadmap (Historical)
- [x] Rate Limiting & Brute-Force Protection (Implemented in Sentinel Phase)
- [x] Perfect Forward Secrecy (Implemented via ECDHE in TLS config)
- [ ] Memory Sanitizers (Planned for CI/CD)

---

## Mentor Q&A (Historical)
**Q: Do we really need TLS?**
**A:** Yes. Storing or transmitting plain-text data breeds dangerous habits. TLS is the bare minimum for professional grade software.
