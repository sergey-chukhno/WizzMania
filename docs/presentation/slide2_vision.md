# Slide 2: L'Équipe d'Ingénierie (The Engineering Team)

## 1. Feature Overview
This slide introduces the human element behind WizzMania. It presents the team roles (Architect, Tech Lead, Lead Dev) and their specific domains of expertise, humanizing the technical complexity.

## 2. How it Works
The slide uses a **Glass Panel Team Grid**:
- **Sergey (Architect)**: Focused on the C++ engine and IPC.
- **Louis (Tech Lead)**: Focused on Asio, networking, and TLS security.
- **Elodie (Lead Dev)**: Focused on the Qt GUI and SQLite database integration.

## 3. Why This Code (Rationale)
While there is no "WizzMania code" on this slide, the division of labor reflects the **Separation of Concerns** principle in the project's architecture:
- Backend vs. Frontend.
- Networking vs. Persistence.
- Systems Programming (IPC) vs. Application Programming (Qt).

## 4. How the Code Implements This
The project structure directly mirrors this team division:
- `server/` (Sergey/Louis): Handles the Asio loops and IPC keys.
- `client/` (Elodie): Handles the Qt Widgets and Signal/Slot plumbing.
- `common/` (Shared): Contains the `Packet` and `GameIPC` structures used by both teams.

## 5. Strategic Analysis

### Advantages
- **Role Clarity**: In a technical defense, this shows that the project wasn't just "hit with a hammer" but carefully partitioned among specialists.
- **Scalability**: A team that understands their boundaries can build modular code (SOLID).

### Drawbacks / Limits
- **Siloing**: If the team doesn't communicate, the "Contract" (the `Packet` protocol) can break. This is why the `common/` folder is the most critical part of the repo.

### Edge Cases
- **Merge Conflicts**: In a real team, Sergey and Louis would often touch the same files in `server/`. We use a **Hander-based architecture** (Slide 6) to minimize these conflicts—each dev works on their own Handler class.
