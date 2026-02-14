# Cyberpunk Cannon Shooter - Development Roadmap

## 🎯 Project Vision

Transform the Next.js Brick Breaker into a C++/SFML Cyberpunk Cannon Shooter with:
- Cannon at bottom shooting energy projectiles upward
- Energy projectiles with bouncing mechanics (bounce off walls, blocks, and cannon)
- Enemy blocks descending from top
- Multi-projectile system with counter display
- Varying block numbers, shapes, and durability based on level
- Block durability and health indicators
- Cyberpunk aesthetic (neon colors, glow effects)
- Level progression with increasing difficulty
- Zone destruction attacks (Phase 2)
- Essential unit and integration tests

## ⏱️ Timeline: One Week Project

## 📊 Development Phases

### Day 1: Foundation & Setup

#### Step 1.1: Project Setup (2-3 hours)
**Goal**: Create a working SFML window with basic structure

**Tasks**:
- [ ] Create project directory structure (`src/`, `include/`, `assets/`, `tests/`)
- [ ] Set up CMakeLists.txt with SFML dependencies
- [ ] Create main.cpp with basic window creation
- [ ] Implement basic game loop (update, render, handle events)
- [ ] Test compilation and window display
- [ ] Set up test framework (Catch2 or Google Test)

**Deliverable**: A window that opens, displays a dark background, and closes on ESC

---

#### Step 1.2: Game State System (3-4 hours)
**Goal**: Implement state machine for menu, playing, paused, gameover states

**Tasks**:
- [ ] Create `GameState` base class with virtual methods (update, render, handleEvent)
- [ ] Create `Game` class to manage state transitions
- [ ] Implement `MenuState` with placeholder UI
- [ ] Implement `PlayingState` (empty for now)
- [ ] Add state switching logic (Menu → Playing → GameOver → Menu)

**Deliverable**: State machine that can switch between menu and playing states

---

#### Step 1.3: Input System (2-3 hours)
**Goal**: Handle keyboard and mouse input

**Tasks**:
- [ ] Create `InputManager` class (optional, or handle in Game class)
- [ ] Implement keyboard input (arrows, space, ESC, A/D)
- [ ] Implement mouse input (position, clicks)
- [ ] Add input handling to each game state
- [ ] Test input responsiveness

**Deliverable**: Responsive input system for all states

---

### Day 2: Cannon & Projectiles

#### Step 2.1: Cannon Implementation (4-5 hours)
**Goal**: Create a cannon that moves and displays projectile counter

**Tasks**:
- [ ] Create `Cannon` class with position, sprite, and projectile count
- [ ] Implement horizontal movement (keyboard: A/D or Arrows, mouse follow optional)
- [ ] Add visual representation (rectangle/sprite with neon glow effect)
- [ ] Display projectile counter on cannon (text rendering)
- [ ] Add cannon bounds checking (stay within screen)
- [ ] **Unit Test**: Test cannon movement and bounds

**Deliverable**: Movable cannon with visible projectile counter (x=10)

---

#### Step 2.2: Projectile System with Bouncing (4-5 hours)
**Goal**: Create projectiles that shoot upward and bounce off walls, blocks, and cannon

**Tasks**:
- [ ] Create `Projectile` class with position, velocity, and visual
- [ ] Implement projectile physics (constant velocity with bouncing)
- [ ] Add wall collision and bounce (left, right, top walls)
- [ ] Add cannon collision and bounce (bottom collision)
- [ ] Add visual rendering (circle/sprite with glow effect)
- [ ] Implement projectile pool for performance (optional but recommended)
- [ ] Add projectile lifecycle (spawn, update, destroy when off-screen bottom)
- [ ] **Unit Test**: Test projectile physics and wall bouncing
- [ ] **Unit Test**: Test projectile-cannon collision

**Deliverable**: Projectiles that shoot upward, bounce off walls and cannon, and move correctly

---

#### Step 2.3: Cannon-Projectile Integration (3-4 hours)
**Goal**: Connect cannon shooting to projectile system

**Tasks**:
- [ ] Implement `Cannon::shoot()` method that creates projectiles
- [ ] Decrement projectile counter on shoot
- [ ] Handle multiple projectiles (shoot 10 in succession or simultaneously)
- [ ] Add reload/cooldown mechanism
- [ ] Update counter display in real-time
- [ ] Add visual feedback (muzzle flash, sound placeholder)
- [ ] **Integration Test**: Test cannon shooting and projectile creation

**Deliverable**: Cannon shoots projectiles and counter decreases

---

### Day 3: Blocks & Level System

#### Step 3.1: Block Base System with Varying Properties (5-6 hours)
**Goal**: Create blocks with varying shapes, colors, and durability

**Tasks**:
- [ ] Create `Block` class with position, shape, color, durability, health
- [ ] Implement different shapes (square, rectangle, hexagon)
- [ ] Add cyberpunk color palette (pink, cyan, purple, green, yellow)
- [ ] Implement health system (take damage, destroy when health <= 0)
- [ ] Add visual health indicator (numeric or bar)
- [ ] Create block rendering with glow effects
- [ ] Implement level-based durability scaling (higher level = more hits required)
- [ ] **Unit Test**: Test block health system and destruction

**Deliverable**: Blocks with different shapes, colors, and scalable durability

---

#### Step 3.2: Block Descent System with Level Variation (3-4 hours)
**Goal**: Make blocks descend from top with varying numbers per level

**Tasks**:
- [ ] Implement block spawning at top of screen
- [ ] Add level-based block count (level 1: few blocks, level 5+: many blocks)
- [ ] Add descent velocity to blocks (faster with higher levels)
- [ ] Implement wave-based spawning system
- [ ] Add block bounds checking (remove when off-screen)
- [ ] Vary block shapes based on level
- [ ] **Unit Test**: Test block spawning and level-based configuration

**Deliverable**: Blocks that spawn at top, descend downward, with varying numbers and speeds per level

---

#### Step 3.3: Collision Detection (4-5 hours)
**Goal**: Detect collisions between projectiles and blocks

**Tasks**:
- [ ] Implement AABB (Axis-Aligned Bounding Box) collision detection
- [ ] Add collision check between projectiles and blocks
- [ ] Handle collision response (damage block, bounce projectile)
- [ ] Add visual feedback (particle effects, sound placeholder)
- [ ] Test collision accuracy and performance
- [ ] **Unit Test**: Test AABB collision detection with known positions
- [ ] **Integration Test**: Test projectile-block collision in game scenario

**Deliverable**: Working collision system between projectiles and blocks with bouncing

---

### Day 4: Game Logic & Level Progression

#### Step 4.1: Game Rules & Level System (4-5 hours)
**Goal**: Implement game win/lose conditions, scoring, and level progression

**Tasks**:
- [ ] Implement score system (points per block destroyed, scaled by level)
- [ ] Add game over condition (block touches bottom or cannon)
- [ ] Implement level progression (clear wave, advance level)
- [ ] Add difficulty scaling system:
  - More blocks per level
  - Higher block durability
  - Faster block descent
  - More varied block shapes
- [ ] Create game over screen with score display
- [ ] Add restart functionality
- [ ] **Integration Test**: Test level progression and difficulty scaling
- [ ] **Integration Test**: Test game over conditions

**Deliverable**: Complete game loop with win/lose conditions and level progression

---

#### Step 4.2: HUD and UI (5-6 hours)
**Goal**: Create heads-up display and game UI

**Tasks**:
- [ ] Create `HUD` class for score, level, lives display
- [ ] Implement menu UI (Start, Settings, Quit buttons)
- [ ] Add settings menu (volume, controls)
- [ ] Implement pause menu (P key)
- [ ] Add game over screen UI
- [ ] Style UI with cyberpunk aesthetic (neon colors, glow effects)
- [ ] Display level information and difficulty indicators

**Deliverable**: Complete UI system for all game states

---

### Day 5: Visual Effects & Polish

#### Step 5.1: Visual Effects (6-8 hours)
**Goal**: Add particle effects and visual polish

**Tasks**:
- [ ] Create particle system for block destruction
- [ ] Add glow effects using SFML shaders or sprite techniques
- [ ] Implement muzzle flash effects
- [ ] Add screen shake on impact (optional)
- [ ] Create background effects (stars, grid, etc.)
- [ ] Polish visual feedback for all actions
- [ ] Add visual indicators for level difficulty

**Deliverable**: Visually polished game with particle effects

---

### Day 6: Audio System

#### Step 6.1: Audio Manager (4-5 hours)
**Goal**: Create centralized audio system

**Tasks**:
- [ ] Create `AudioManager` class (singleton or dependency injection)
- [ ] Load sound effects (shoot, hit, destroy, game over)
- [ ] Load background music
- [ ] Implement volume control
- [ ] Add sound effect playback methods
- [ ] Integrate audio into game events

**Deliverable**: Working audio system with sound effects and music

---

#### Step 6.2: Audio Integration (2-3 hours)
**Goal**: Add sounds to all game events

**Tasks**:
- [ ] Add shooting sound effect
- [ ] Add block hit sound effect
- [ ] Add block destruction sound effect
- [ ] Add game over sound effect
- [ ] Add background music with looping
- [ ] Implement volume settings integration

**Deliverable**: Fully integrated audio system

---

### Day 7: Testing & Advanced Features

#### Step 7.1: Testing Suite (4-5 hours)
**Goal**: Implement essential unit and integration tests

**Tasks**:
- [ ] Set up test infrastructure in `tests/` directory
- [ ] Write unit tests for:
  - Collision detection functions
  - Projectile physics and bouncing
  - Block health and durability system
  - Cannon movement and bounds
  - Level progression logic
- [ ] Write integration tests for:
  - Cannon shooting and projectile creation
  - Projectile-block collision system
  - Block spawning and descent
  - Game over conditions
  - Level progression flow
- [ ] Run test suite and fix any failures
- [ ] Ensure all critical paths are tested

**Deliverable**: Comprehensive test suite covering essential functionality

---

#### Step 7.2: Asset Manager (3-4 hours) - If Time Permits
**Goal**: Centralize asset loading and caching

**Tasks**:
- [ ] Create `AssetManager` class
- [ ] Implement texture loading and caching
- [ ] Implement sound loading and caching
- [ ] Implement font loading and caching
- [ ] Add error handling for missing assets
- [ ] Refactor existing code to use AssetManager

**Deliverable**: Centralized asset management system

---

#### Step 7.3: Zone Destruction Attack (4-5 hours) - If Time Permits
**Goal**: Implement right-click area attack

**Tasks**:
- [ ] Add right-click input handling
- [ ] Create zone destruction effect (circular area)
- [ ] Implement block destruction within zone
- [ ] Add cooldown system for zone attack
- [ ] Add visual feedback (explosion effect, circle indicator)
- [ ] Balance zone attack (damage, radius, cooldown)

**Deliverable**: Working zone destruction attack with cooldown

---

## 🎨 Aesthetic Implementation Guide

### Color Palette
```cpp
namespace Colors {
    const sf::Color Background(10, 10, 26);      // #0a0a1a
    const sf::Color Pink(255, 0, 110);           // #ff006e
    const sf::Color Cyan(0, 217, 255);           // #00d9ff
    const sf::Color Purple(157, 78, 221);        // #9d4edd
    const sf::Color Green(6, 255, 165);          // #06ffa5
    const sf::Color Yellow(255, 190, 11);        // #ffbe0b
}
```

### Visual Effects
1. **Neon Glow**: Use SFML shaders or multiple sprite layers with different opacities
2. **Particle Effects**: Custom particle system or use SFML's vertex arrays
3. **Glass Effects**: Transparent sprites with outline rendering
4. **Background**: Starfield using simple shapes or sprites

### Typography
- Use a futuristic/monospace font (e.g., "Orbitron", "Rajdhani", or system monospace)
- Add text shadow/outline for readability
- Use neon colors for text

## 📝 Level Progression System

### Level Configuration
- **Level 1**: 5-8 blocks, 1-2 hits to destroy, slow descent
- **Level 2**: 10-12 blocks, 2-3 hits to destroy, medium descent
- **Level 3**: 15-18 blocks, 3-4 hits to destroy, medium-fast descent
- **Level 4**: 20-25 blocks, 4-5 hits to destroy, fast descent
- **Level 5+**: 25+ blocks, 5+ hits to destroy, very fast descent, multiple shapes

### Block Shape Distribution
- Early levels: Mostly squares and rectangles
- Mid levels: Mix of squares, rectangles, and hexagons
- Late levels: All shapes, with more hexagons (harder to hit)

## 🧪 Testing Strategy

### Test Folder Structure
```
tests/
├── unit/
│   ├── test_collision.cpp
│   ├── test_projectile.cpp
│   ├── test_block.cpp
│   ├── test_cannon.cpp
│   └── test_level.cpp
├── integration/
│   ├── test_cannon_projectile.cpp
│   ├── test_projectile_block.cpp
│   ├── test_game_flow.cpp
│   └── test_level_progression.cpp
└── CMakeLists.txt
```

### Unit Tests (Essential)
1. **Collision Detection**: Test AABB collision with known positions
2. **Projectile Physics**: Test velocity, bouncing, boundary conditions
3. **Block System**: Test health, durability, destruction
4. **Cannon Movement**: Test bounds checking and movement
5. **Level Logic**: Test difficulty scaling and progression

### Integration Tests (Essential)
1. **Cannon-Projectile**: Test shooting and projectile creation
2. **Projectile-Block**: Test collision and damage system
3. **Block Spawning**: Test level-based block generation
4. **Game Flow**: Test state transitions and game over
5. **Level Progression**: Test difficulty increases and wave completion

### Testing Framework
- Use **Catch2** (lightweight, header-only) or **Google Test**
- Keep tests simple and focused
- Test critical paths only
- Aim for 70-80% code coverage of core game logic

## 🐛 Common Pitfalls to Avoid

1. **Memory Leaks**: Always use smart pointers, avoid raw `new/delete`
2. **Performance**: Don't create/destroy objects every frame (use object pools)
3. **Collision Detection**: Keep it simple (AABB) unless performance requires optimization
4. **State Management**: Clear state transitions, avoid state conflicts
5. **Resource Loading**: Load assets once, cache them, don't reload every frame
6. **Magic Numbers**: Use named constants for all game values
7. **Hardcoded Values**: Make values configurable (window size, speeds, etc.)
8. **Testing**: Don't over-test - focus on critical functionality

## 🎓 Learning Objectives

By completing this project, you will learn:
- C++ game development with SFML
- Game state management
- Collision detection algorithms
- Physics simulation (bouncing, velocity)
- Particle systems
- Audio integration
- Resource management
- Game loop architecture
- Object-oriented design patterns
- Modern C++ features (smart pointers, STL, etc.)
- Unit and integration testing

## 📚 Resources

### SFML
- Official Tutorials: https://www.sfml-dev.org/tutorials/
- API Documentation: https://www.sfml-dev.org/documentation/

### C++
- C++ Core Guidelines: https://isocpp.github.io/CppCoreGuidelines/
- Learn C++: https://www.learncpp.com/

### Testing
- Catch2: https://github.com/catchorg/Catch2
- Google Test: https://github.com/google/googletest

### Game Development
- Game Programming Patterns: https://gameprogrammingpatterns.com/
- Game Loop Architecture: Research game loop patterns

---

## ✅ One-Week Checklist

### Day 1: Foundation
- [ ] Project setup and CMake configuration
- [ ] Game state system
- [ ] Input system

### Day 2: Core Mechanics
- [ ] Cannon implementation
- [ ] Projectile system with bouncing
- [ ] Cannon-projectile integration

### Day 3: Blocks & Levels
- [ ] Block system with varying properties
- [ ] Block descent with level variation
- [ ] Collision detection

### Day 4: Game Logic
- [ ] Game rules and level progression
- [ ] HUD and UI

### Day 5: Polish
- [ ] Visual effects
- [ ] Particle systems

### Day 6: Audio
- [ ] Audio manager
- [ ] Audio integration

### Day 7: Testing & Extras
- [ ] Unit tests
- [ ] Integration tests
- [ ] Asset manager (if time)
- [ ] Zone attack (if time)

---

**Next Steps**: Begin with Step 1.1 (Project Setup) on Day 1.
