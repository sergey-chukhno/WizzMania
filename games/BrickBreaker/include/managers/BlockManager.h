#ifndef BLOCKMANAGER_H
#define BLOCKMANAGER_H

#include "entities/Block.h"
#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>

/**
 * @brief Manages block spawning, descent, and wave system.
 * 
 * BlockManager handles:
 * - Wave-based block spawning at top of screen
 * - Level-based configuration (block count, descent speed, waves)
 * - Block descent mechanics
 * - Boundary detection and block removal
 * - Game over detection (blocks reach bottom or touch cannon)
 * - Level progression
 */
class BlockManager {
public:
    /**
     * @brief Constructs a BlockManager.
     * @param windowWidth Window width in pixels
     * @param windowHeight Window height in pixels
     */
    BlockManager(unsigned int windowWidth, unsigned int windowHeight);

    /**
     * @brief Destructor.
     */
    ~BlockManager() = default;

    /**
     * @brief Updates block manager state.
     * @param deltaTime Time elapsed since last frame (in seconds)
     * @param cannonBounds Cannon bounding rectangle (for collision detection)
     */
    void update(float deltaTime, const sf::FloatRect& cannonBounds);
    
    /**
     * @brief Updates block destroyed states (checks if blocks should be marked as destroyed).
     * This should be called after collision detection to ensure blocks are marked as destroyed
     * when all their bricks are destroyed.
     * @param deltaTime Time elapsed since last frame (in seconds)
     */
    void updateBlockDestroyedStates(float deltaTime);

    /**
     * @brief Renders all active blocks.
     * @param window Reference to the render window
     */
    void render(sf::RenderWindow& window) const;

    /**
     * @brief Starts a new level.
     * @param level Level number (1-based)
     */
    void startLevel(int level);

    /**
     * @brief Advances to the next level.
     */
    void advanceLevel();

    /**
     * @brief Gets the current level.
     * @return Current level number
     */
    int getCurrentLevel() const { return currentLevel_; }

    /**
     * @brief Checks if the current level is complete.
     * @return True if level is complete (all blocks destroyed or off-screen), false otherwise
     */
    bool isLevelComplete() const;

    /**
     * @brief Checks if the current wave is complete.
     * @return True if wave is complete (all blocks spawned), false otherwise
     */
    bool isWaveComplete() const;

    /**
     * @brief Gets all active blocks (for collision detection).
     * @return Vector of block pointers
     */
    std::vector<Block*> getActiveBlocks();

    /**
     * @brief Gets all active blocks (const version).
     * @return Vector of const block pointers
     */
    std::vector<const Block*> getActiveBlocks() const;

    /**
     * @brief Checks if any block has reached the bottom of the screen.
     * @return True if any block reached bottom, false otherwise
     */
    bool hasBlocksReachedBottom() const;

    /**
     * @brief Checks if any block has touched the cannon.
     * @param cannonBounds Cannon bounding rectangle
     * @return True if any block touched cannon, false otherwise
     */
    bool hasBlocksTouchedCannon(const sf::FloatRect& cannonBounds) const;

    /**
     * @brief Gets the number of active blocks.
     * @return Number of active blocks
     */
    int getActiveBlockCount() const;

    /**
     * @brief Updates window size (for resizing).
     * @param width New window width
     * @param height New window height
     */
    void setWindowSize(unsigned int width, unsigned int height);

private:
    /**
     * @brief Level configuration structure.
     */
    struct LevelConfig {
        int blockCount;           // Total blocks for this level
        int blocksPerWave;        // Blocks per wave
        int waveCount;            // Number of waves
        float waveDelay;          // Delay between waves (seconds)
        float spawnInterval;      // Delay between block spawns (seconds)
        float descentSpeed;       // Descent speed (pixels per second)
        float spawnYOffset;       // Y offset from top for spawning (negative, above screen)
    };

    // Window dimensions
    unsigned int windowWidth_;
    unsigned int windowHeight_;

    // Level and wave management
    int currentLevel_;
    int currentWave_;
    int blocksInCurrentWave_;
    int blocksSpawnedInWave_;
    int totalBlocksSpawned_;
    float timeSinceLastSpawn_;
    float waveSpawnTimer_;
    bool isSpawning_;
    bool waitingForWaveDelay_;

    // Block storage
    std::vector<std::unique_ptr<Block>> blocks_;

    // Level configuration
    LevelConfig currentLevelConfig_;

    // Game state
    bool levelComplete_;
    bool blockReachedBottom_;

    // Constants
    static constexpr float SPAWN_Y_OFFSET = -100.0f;  // Spawn above screen (off-screen top)
    static constexpr float BOTTOM_MARGIN = 10.0f;     // Safety margin for bottom detection

    /**
     * @brief Calculates level configuration based on level number.
     * @param level Level number (1-based)
     * @return Level configuration
     */
    LevelConfig calculateLevelConfig(int level) const;

    /**
     * @brief Starts a new wave.
     */
    void startWave();

    /**
     * @brief Spawns a single block at the top of the screen.
     */
    void spawnBlock();

    /**
     * @brief Removes blocks that are off-screen.
     */
    void removeOffScreenBlocks();

    /**
     * @brief Checks if a block is off-screen.
     * @param block Block to check
     * @return True if block is off-screen, false otherwise
     */
    bool isBlockOffScreen(const Block& block) const;

    /**
     * @brief Checks if a block has reached the bottom of the screen.
     * @param block Block to check
     * @return True if block reached bottom, false otherwise
     */
    bool hasBlockReachedBottom(const Block& block) const;

    /**
     * @brief Checks if a block touches the cannon.
     * @param block Block to check
     * @param cannonBounds Cannon bounding rectangle
     * @return True if block touches cannon, false otherwise
     */
    bool doesBlockTouchCannon(const Block& block, const sf::FloatRect& cannonBounds) const;

    /**
     * @brief Performs AABB collision detection.
     * @param rect1 First rectangle
     * @param rect2 Second rectangle
     * @return True if rectangles intersect, false otherwise
     */
    bool checkAABBCollision(const sf::FloatRect& rect1, const sf::FloatRect& rect2) const;
};

#endif // BLOCKMANAGER_H

