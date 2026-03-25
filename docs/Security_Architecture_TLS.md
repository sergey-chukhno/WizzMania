# WizzMania Architecture Deep Dive: Transport Layer Security (TLS)

Prior to Phase 3, WizzMania utilized raw TCP sockets. The implication of this was severe: any user sitting in a Starbucks café, utilizing shared Public Wi-Fi, was transmitting direct messages, passwords, and private voice memos in completely unencrypted plaintext. Anyone running Wireshark could passively read their deepest secrets.

To establish enterprise-grade data privacy, we wrapped our active Boost.Asio and Qt sockets inside **Transport Layer Security (TLS v1.2)**.

---

## 1. The Cryptographic Handshake

TLS serves exactly three cryptographic purposes:
1. **Confidentiality:** Scrambling the bytes into static so routers and ISPs cannot read the contents (Symmetric Encryption, i.e., AES-256).
2. **Integrity:** Ensuring packets are not maliciously modified in transit (MACs / SHA-256 hashing).
3. **Authentication:** Proving the Server is actually WizzMania, and not a hacker performing a "Man in the Middle" attack (Asymmetric Key Pairs / RSA Certificates).

---

## 2. Server Implementation (`boost::asio::ssl`)

With Boost.Asio, we do not need to rewrite our entire server architecture to support TLS. We simply "wrap" the raw socket in an SSL stream wrapper!

### Loading the Digital Identity
During Server initialization, the application loads the RSA private key (`server.key`) and the public X.509 certificate (`server.crt`). 

```cpp
m_sslContext.set_options(
    asio::ssl::context::default_workarounds |
    asio::ssl::context::no_sslv2 | // Block ancient, insecure protocols
    asio::ssl::context::no_sslv3 |
    asio::ssl::context::no_tlsv1 |
    asio::ssl::context::no_tlsv1_1);

m_sslContext.use_certificate_chain_file("certs/server.crt");
m_sslContext.use_private_key_file("certs/server.key", asio::ssl::context::pem);
```

### The Async Handshake Wrapper
Inside `ClientSession`, the raw `boost::asio::ip::tcp::socket` was upgraded to a `boost::asio::ssl::stream<boost::asio::ip::tcp::socket>`.  

When a new client connects, before *any* WizzMania usernames or JSON packets are exchanged, the Session enforces the TLS handshake:

```cpp
void ClientSession::start() {
    auto self(shared_from_this());
    
    // Don't read! Handshake first!
    m_sslSocket.async_handshake(boost::asio::ssl::stream_base::server,
        [this, self](const boost::system::error_code& error) {
            if (!error) {
                // Handshake success! Secure tunnel is established!
                doReadHeader(); // Begin reading raw encrypted packets
            } else {
                std::cerr << "TLS Handshake Failed: " << error.message();
            }
        });
}
```
Boost intercepts the `async_read` and `async_write` calls and seamlessly decrypts/encrypts the raw arrays on the fly using OpenSSL, meaning our internal WizzMania packet logic never needed to change!

---

## 3. Client Implementation (`QSslSocket`)

Over on the Qt Client side, upgrading from a raw TCP socket to a secure TLS stream was remarkably simple thanks to the Qt Web Framework.

Inside `NetworkManager.cpp`, we swapped the standard `QTcpSocket` for a `QSslSocket`. 

### Trusting Self-Signed Certificates
In a production environment, WizzMania would purchase a verified SSL Certificate from an authority like Let's Encrypt. Because we generated our own *Self-Signed* certificate using OpenSSL for local desktop development, Qt's security protocols immediately rejected the connection, raising a glaring `QSslError::SelfSignedCertificate` flag!

To force the client to temporarily trust our local test server, we selectively intercepted the SSL Error signal:

```cpp
m_socket = new QSslSocket(this);

// Ignore local verification errors for development
connect(m_socket, QOverload<const QList<QSslError>&>::of(&QSslSocket::sslErrors),
        this, [this](const QList<QSslError> &errors) {
    for (const auto& error : errors) {
        if (error.error() == QSslError::SelfSignedCertificate || 
            error.error() == QSslError::HostNameMismatch) {
            m_socket->ignoreSslErrors({error}); // "I know, let me in anyway"
        }
    }
});

m_socket->connectToHostEncrypted(host, port);
```

By calling `connectToHostEncrypted()` instead of a standard `connectToHost()`, Qt initiates the client-side of the negotiation, negotiating cipher suites with the Asio server and establishing the Secure Session Key.

---

### Architectural Summary
By seamlessly migrating Both Client and Server socket handlers up a level in abstraction to OpenSSL/QSsl sockets, we achieved total Transport Layer Security. 

Every single Direct Message, Password login attempt, Contact Status Change, and Game Score broadcast is now wrapped in a cryptographically unbreakable AES envelope before touching the Wi-Fi card preventing any form of network espionage on the WizzMania platform.
