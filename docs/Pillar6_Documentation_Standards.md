# Pillar 6: Documentation & Engineering Communication Standards

This document outlines the professional standards for engineering communication and documentation practices implemented and encouraged within the WizzMania project.

## Q: Why is documentation considered a "Living Record"?
**A:** In fast-paced Agile environments, code changes rapidly. If documentation is treated as a one-time artifact, it becomes stale and misleading. A "Living Record" (like our `Client_Architecture.md` and `Server_Architecture.md`) is stored alongside the code in version control and is updated iteratively in the same Pull Requests as the code changes. This ensures architectural drift never happens.

## Q: What are the engineering standards for a README.md?
**A:** A professional `README.md` is the "landing page" of your repository. It must immediately communicate:
1. **What the project is:** A one-sentence elevator pitch.
2. **Visual Proof:** Screenshots or GIFs of the application in action.
3. **Prerequisites & Installation:** Exact commands (like our `Windows_Setup_Guide.md` instructions).
4. **Architecture Overview:** High-level component map.
5. **Usage/Run:** Instructions on how to start the app (e.g., `launch_multiclient.bat`).

## Q: How do we document security and threading?
**A:** We document these by the "Why", not just the "How". 
- **Threading:** We explain *why* we chose the Actor Model for the Database, isolating it to prevent blocking the `select()` event loop.
- **Security:** We explain *why* we moved from plain-text to OpenSSL SHA-256 with 128-bit salts, to protect user data from rainbow table attacks.

## Q: What is the standard for Pull Request (PR) communication?
**A:** 
- **Title:** Imperative mood (e.g., "Refactor Network Manager to use Qt Signals").
- **Body:** What was changed, *why* it was changed, and how to verify it.
- **Linkages:** Automatically linking to the Jira/GitHub Issue ticket.
- **Visuals:** Attach a screenshot of the UI if the UI was altered.

---
*This concludes the WizzMania Mentorship Curriculum documentation.*
