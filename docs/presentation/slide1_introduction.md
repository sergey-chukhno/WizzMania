# Slide 1: Introduction to WizzMania

## 1. Feature Overview
The introduction slide presents **WizzMania** as a high-performance C++ social and gaming hub. It sets the tone for a professional, "Neo-Seoul" cyberpunk aesthetic while immediately grounding the project in technical excellence (Boost.Asio, POSIX IPC, Multi-threading).

## 2. How it Works
This is the "Hero" slide. It uses a 3D-extruded mockup of the application built with standard HTML/CSS to demonstrate the target UI:
- **Holographic Glass UI**: Represents the modern, premium feel.
- **Real-time Indicators**: Shows "Online" status, unread message badges, and "Louis is typing..." animations.
- **System Integrity**: Mentions "10,000 connexions ok" to signal scalability from the start.

## 3. Why This Code (Rationale)
Although this slide is mostly presentation-oriented, the *concept* of WizzMania's UI (mocked here) relies on **Signals and Slots** in the actual C++ code. We use C++ for the UI (Qt6) because it allows for hardware-accelerated rendering and direct integration with the high-speed network stack without the overhead of a web-based wrapper (like Electron).

## 4. How the Code Implements This
In the actual codebase (`client/MainWindow.cpp`):
- **Mockup Header**: Implemented via `QHBoxLayout` and custom `QWidget` styling.
- **Typing Indicator**: Powered by `QTimer` and `NetworkManager` signals. When a `PacketType::TypingIndicator` is received, the UI updates the label.
- **3D Scene**: In the presentation, this is CSS3D. In the app, we use custom Qt stylesheets (`QSS`) to achieve the glass-morphism effect.

## 5. Strategic Analysis

### Advantages
- **First Impression**: Immediately establishes WizzMania as more than a "simple chat app" through high-end visuals and technical keywords.
- **Clarity**: The "Wizz spirit" (fast, energetic, modern) is conveyed visually.

### Drawbacks / Limits
- **Aesthetic vs. Function**: High-end visuals must be backed by the performance promised (Boost.Asio). If the UI feels sluggish while the backend is fast, the "Wizz" brand fails.

### Edge Cases
- **Low-End Hardware**: While the presentation looks grand, the actual C++ app must remain lightweight. We use native components to ensure even users with limited resources get a fluid experience.
