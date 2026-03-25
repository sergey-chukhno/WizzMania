# Slide 11: Démo en Direct (The Live Experience)

## 1. Feature Overview
The call to action. This slide provides the launch point for the actual demonstration, showcasing the "finished product" running two instances of WizzMania.

## 2. How it Works
1. **Launcher Script**: A Python (`serve.py`) or Shell (`launch_multiclient.sh`) script is triggered.
2. **Environment Check**: Ensures certificates exist and the database is initialized.
3. **Execution**: Opens two windows side-by-side to simulate a real conversation between two users (e.g., Elodie and Louis).

## 3. Why This Code (Rationale)
- **`fetch('/launch')`**: The presentation itself (HTML) can control the OS! By running a small Python web server, we can click a "Launch" button on a slide and see real C++ windows pop up. This makes for a very impressive, professional demo.
- **Branded Assets**: The mockup on the right (`wizzmania_app_branded.png`) reinforces the visual identity.

## 4. How the Code Implements This
- **`launch_multiclient.sh`**:
  ```bash
  ./wizz_client --username=Elodie &
  ./wizz_client --username=Louis &
  ```
- **`script.js`**: Handles the button click in the presentation, sends an AJAX request to the local Python script, and updates the button label to "✓ Clients lancés !".

## 5. Strategic Analysis

### Advantages
- **Interaction**: Breaks the fourth wall of the presentation.
- **Reality Check**: Proves that the code snippets shown in previous slides actually work together.

### Drawbacks / Limits
- **Demo Effect**: Demos are notoriously fragile. If the server is already running on port 8080, the second one might fail.
- **Dependencies**: Requires the user to build the project successfully *before* starting the presentation.

### Edge Cases
- **Resolution**: On smaller laptops, two windows might overlap. We should use `QDesktopWidget` or a window manager script to position them side-by-side.
- **Cleanup**: If the demo is run multiple times, the temporary SQLite entries might need to be cleared (`rm wizzmania.db`).
