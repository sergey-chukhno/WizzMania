# WizzMania Group Chats & Channels — Feature Specification 💬📢

**Pillar Alignment**: Pillar 1 (The Shield) + Pillar 3 (The Ghost) + Pillar 4 (The Shell)
**Status**: Specified — Pending Implementation
**Last Updated**: 2026-05-09 (Decisions locked after architecture review)

---

## Overview

| Dimension | Group Chat | Channel |
|---|---|---|
| **Participants** | Up to 256 members | Unlimited subscribers |
| **Direction** | Multi-directional | Unidirectional (Admin broadcast only) |
| **Encryption** | E2EE via Sender Keys | Symmetric key at rest (server-held); full E2EE is a long-term MLS target |
| **Persistence** | Indefinite history | Indefinite history |
| **Discovery** | Invite-only | Public or invite-based |
| **Use Case** | Team collaboration, friend circles | Announcements, communities, broadcast |

---

## Part 1: Group Chats

### 1.1 Functional Requirements

**Creation & Membership**
- Any authenticated user can create a group with a name and optional avatar.
- The creator becomes the **Owner**. Only the Owner can promote members to **Admin**.
- Groups support a maximum of **256 members** (see §1.3 for rationale).
- Members are added by username invite and must accept before joining.
- Admins can remove members; the Owner can dissolve the group or transfer ownership.

**Messaging**
- All members can send text, voice messages, and multimedia.
- Messages display sender name and avatar inline.
- **Reply Threading**: Members can reply to a specific message, creating a visual thread reference.
- **Reactions**: Members react with emoji, stored server-side per message ID.
- **Mentions**: `@username` triggers a notification regardless of group mute setting.

**Roles & Permissions**

| Permission | Owner | Admin | Member |
|---|---|---|---|
| Send messages | ✅ | ✅ | ✅ |
| Add members | ✅ | ✅ | ❌ |
| Remove members | ✅ | ✅ | ❌ |
| Edit group info | ✅ | ✅ | ❌ |
| Delete messages | ✅ | ✅ (others') | ✅ (own only) |
| Promote to Admin | ✅ | ❌ | ❌ |
| Dissolve group | ✅ | ❌ | ❌ |

**Notifications**: Per-group preferences: All Messages / Mentions Only / Muted.

### 1.2 History Retention
**Decision**: Indefinite retention. All group messages are stored permanently on the server.
Pagination is implemented from day one (cursor-based) so that join-time history fetch does not load the full history at once.

### 1.3 Group Size Limit: Why 256?

The 256-member cap is driven by the cost of **Sender Key rotation** upon member removal.

When a member is removed, Forward Secrecy requires all remaining members to exchange new Sender Keys — each distributed via pairwise Double Ratchet. For a 256-member group, worst-case rotation requires ~32,000 pairwise E2EE messages across the network. This is feasible. For a 10,000-member group, the same operation requires ~50 million messages — computationally impossible on mobile hardware and catastrophic for server bandwidth.

We start at 256 and raise the limit when we migrate to MLS (which achieves O(log N) key updates via a ratchet tree structure, making large-group rotation practical).

**Server Fan-Out** is how a single group message reaches all members despite the 256-member cap:

```
Client → Server: 1 ciphertext packet
Server → RoomManager::fanOut() → Client[0..255]: same ciphertext × N sessions
```

```cpp
// Conceptual RoomManager fan-out (Asio-based, non-blocking)
void RoomManager::fanOut(GroupId id, const Packet& packet) {
    for (auto* session : m_groups[id].onlineMembers) {
        asio::post(m_ioContext, [session, packet]() {
            session->sendPacket(packet);
        });
    }
}
```

Fan-out is a pure network copy — no per-recipient cryptographic operation — because Sender Keys produce one shared ciphertext that every member independently decrypts.

### 1.4 End-to-End Encryption: Sender Keys

**Why Sender Keys instead of N-times encryption?**

The naive approach (encrypting once per member) costs O(N) CPU and bandwidth from the sender per message. In a 256-member group, every message would require 256 separate encryption operations and 256 ciphertexts sent. On a mobile device sending 50 messages/day across 10 groups, this becomes untenable.

Sender Keys split the cost into two phases:

**Phase A — Key Distribution (one-time)**: Each member generates a Sender Key (an HKDF-derived symmetric chain key + Ed25519 signing keypair) and distributes it to every other member via the existing pairwise Double Ratchet channel. Cost: O(N) once, not per message.

**Phase B — Messaging (O(1) forever)**: To send a message, a member encrypts once using their own Sender Key chain → one ciphertext → server fans it out → all members decrypt independently using the pre-distributed Sender Key.

**Key rotation on member removal**: When a member is removed, all remaining members distribute new Sender Keys to each other (O(N) operation, acceptable at 256 members). This restores Forward Secrecy by cryptographically evicting the removed member.

**Why not MLS instead of Sender Keys?**

MLS (RFC 9420) achieves O(log N) key updates via a binary ratchet tree, making it superior for large groups and post-compromise security. However:
1. No production-grade C++ library exists today. OpenMLS is Rust-only.
2. Implementing MLS from scratch is a multi-year effort with extremely high risk of cryptographic bugs.
3. Sender Keys are proven at scale (Signal, WhatsApp — billions of messages/day).
4. For ≤256 members, Sender Keys' O(N) rotation is operationally acceptable.

**MLS remains our long-term migration target** once a stable C++ library matures.

### 1.5 Protocol Extensions

| Opcode | Direction | Description |
|---|---|---|
| `GROUP_CREATE` | Client → Server | Create group, supply name, avatar, initial member list |
| `GROUP_INVITE` | Server → Client | Deliver invitation to invited user |
| `GROUP_ACCEPT` | Client → Server | Accept membership |
| `GROUP_LEAVE` | Client → Server | Leave the group |
| `GROUP_MESSAGE` | Client → Server | Send a message to a group room |
| `GROUP_MESSAGE_RELAY` | Server → Client | Fan-out message to all online members |
| `GROUP_MEMBER_CHANGE` | Server → Client | Broadcast membership/role changes |
| `GROUP_HISTORY_REQUEST` | Client → Server | Fetch paginated history (cursor-based) |
| `GROUP_HISTORY_RESPONSE` | Server → Client | Deliver history chunk |
| `SENDER_KEY_DISTRIBUTION` | Client → Client (via relay) | Distribute Sender Keys to group members |

### 1.6 Server-Side: Room Manager

New `RoomManager` service required:
- Maps `group_id → set<ClientSession*>` for online member routing.
- New `GroupDAO` persists metadata (name, avatar, members, roles) and message history.
- Handles fan-out for online members and store-and-forward for offline members.
- Offline message delivery: all missed messages delivered on next login (indefinite retention, cursor-based pagination).

---

## Part 2: Channels

### 2.1 Functional Requirements

**Structure**: Channels have Owners and Admins who broadcast; regular users are Subscribers. No upper subscriber limit.

**Broadcasting**
- Only Admins/Owners can post to a channel.
- Posts support text, images, voice, video clips, and embedded links with rich previews.
- Posts can be **pinned** for persistent visibility.
- Posts support Reactions.
- **Comments are a launch feature**: Admins can enable threaded comments on any post.

**Discovery**: Public channels are indexed in the Channel Directory (searchable by name/category/keyword).

**Notifications**: All Posts / Mentioned Only / Muted.

### 2.2 Encryption Model

- **Public Channels**: Plaintext on server (required for search indexing and discoverability).
- **Private Channels**: Encrypted at rest using a server-held AES channel key. This protects against external network attackers but not a compromised server. Full E2EE private channels require MLS and are a **long-term target**.

**Why not full E2EE private channels now?** Channels have unbounded, high-churn membership. Sender Key distribution becomes O(N × M) per membership change for N members and M Sender Keys. MLS solves this with O(log N) tree updates, but no production C++ MLS library exists. The symmetric-at-rest model is an explicit, documented trade-off — not an oversight.

### 2.3 History Retention
**Decision**: Indefinite retention. Offline subscribers receive all missed posts upon reconnection. As subscriber counts grow, this will require a cursor-based pagination solution to remain scalable (planned as a Phase 2 optimization).

### 2.4 Protocol Extensions

| Opcode | Direction | Description |
|---|---|---|
| `CHANNEL_CREATE` | Client → Server | Create channel |
| `CHANNEL_SUBSCRIBE` | Client → Server | Subscribe to a channel |
| `CHANNEL_POST` | Client → Server | Admin/Owner posts to channel |
| `CHANNEL_POST_RELAY` | Server → Client | Deliver new post to online subscribers |
| `CHANNEL_HISTORY_REQUEST` | Client → Server | Fetch paginated post history |
| `CHANNEL_SEARCH` | Client → Server | Search public channel directory |
| `CHANNEL_DIRECTORY` | Server → Client | Return search results |
| `CHANNEL_COMMENT` | Client → Server | Post a comment on a channel post |
| `CHANNEL_COMMENT_RELAY` | Server → Client | Deliver comment to subscribers |

---

## 3. Shared Infrastructure

- **`GroupDAO` / `ChannelDAO`**: New DAOs in the server's modular database layer.
- **`RoomManager`**: Server-side service mapping rooms to active sessions with fan-out.
- **Paginated History**: Cursor-based pagination for both entities from day one.
- **Notification Service**: Server-side dispatch aware of per-user per-room preferences.

---

## 4. Architectural Decisions Log

| Decision | Choice | Rationale |
|---|---|---|
| Group size limit | 256 members | Sender Key rotation cost; raise with MLS migration |
| History retention | Indefinite | Simplicity; pagination prevents loading cost |
| Admin promotion | Owner only | Prevents privilege escalation chains |
| Channel comments | Launch feature | Increases engagement; manageable scope |
| Offline channel delivery | All missed posts | Simple and correct; pagination added in Phase 2 |
| Private channel E2EE | Symmetric at rest | No MLS library; full E2EE is long-term target |
