# Wizz Mania — Protocol Implementation

## 1. Overview
The Wizz Mania protocol uses a custom **Binary Header** format over TCP.
This document explains the technical implementation of the `Packet` class found in `common/Packet.h`.

---

## 2. The Packet Structure
Every transmission consists of two parts:
1.  **Header (12 Bytes)**: Metadata about the message.
2.  **Body (Variable)**: The actual content.

```cpp
struct PacketHeader {
    uint32_t magic;  // 0xCAFEBABE (Sentinel)
    uint32_t type;   // Operation ID (Login, Message, etc.)
    uint32_t length; // Size of the Body
};
```

---

## 3. Network Byte Order (Endianness)

### The Problem: Little vs. Big Endian
Different CPU architectures store integers differently in memory.
*   **Host (Your Mac/Intel):** Little Endian. The number `1` is stored as `01 00 00 00`.
*   **Network (Standard):** Big Endian. The number `1` is expected as `00 00 00 01`.

If we send raw bytes from a Mac to a generic server without conversion, the server will interpret `01 00 00 00` as `16,777,216` instead of `1`.

### The Solution: `htonl` and `ntohl`
We use helper functions from `<arpa/inet.h>` (or `<winsock2.h>` on Windows):

*   **`htonl` (Host to Network Long):** Used when **Writing**. Converts your CPU's format to the Network Standard.
*   **`ntohl` (Network to Host Long):** Used when **Reading**. Converts the Network Standard back to your CPU's format.

**Example:**
```cpp
void Packet::writeInt(uint32_t val) {
    uint32_t networkVal = htonl(val); // Convert BEFORE sending
    writeData(&networkVal, sizeof(uint32_t));
}
```

---

## 4. Reading & Safety

### The "Cursor" Concept
Writing to a packet is easy (just append). Reading is dangerous because we must track "where we are".
We implemented a `size_t m_readOffset` (Cursor).

*   **Start:** Cursor = 0.
*   **Read Int:** Read 4 bytes at Cursor. Cursor += 4.
*   **Read String:** Read Length (Int). Read N bytes. Cursor += N.

### Buffer Overflow Prevention
We strictly enforce **Bounds Checking** before every read.

**Vulnerability Scenario:**
A hacker sends a packet with `Length: 5` but claims it contains a String of size `1,000,000`.
*   *Unsafe Code:* Reads 1,000,000 bytes -> Crashes or leaks memory.
*   *Our Code:*
    ```cpp
    if (m_readOffset + len > m_body.size()) {
        throw std::out_of_range("Not enough data to read string");
    }
    ```
    We throw an exception immediately, protecting the server process.

### Exception Handling
The Reader methods throw standard C++ exceptions:
*   `std::runtime_error`: Malformed header or invalid Magic Number.
*   `std::out_of_range`: Attempting to read past the end of the data.

---

## 5. Verification (Unit Tests)
The file `tests/common/test_packet.cpp` confirms correctness.

**Test Case 1: Serialization**
1.  We created a `Packet` with string "sergey" and int `42`.
2.  We verified the total size is exactly **26 bytes**:
    *   12 Bytes Header
    *   4 Bytes String Length
    *   6 Bytes "sergey"
    *   4 Bytes Integer
    *   Total = 26.

**Test Case 2: Deserialization**
1.  We fed those 26 bytes back into a new `Packet` object.
2.  We verified `readString()` returned "sergey" and `readInt()` returned 42.

**Test Case 3: Bounds Check**
1.  We created an empty packet.
2.  We tried to call `readInt()`.
3.  We asserted that the code **threw an exception** (Passed).
