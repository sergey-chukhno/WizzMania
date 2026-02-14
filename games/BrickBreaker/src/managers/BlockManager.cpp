#include "managers/BlockManager.h"
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <iostream>

BlockManager::BlockManager(unsigned int windowWidth, unsigned int windowHeight)
    : windowWidth_(windowWidth)
    , windowHeight_(windowHeight)
    , currentLevel_(1)
    , currentWave_(0)
    , blocksInCurrentWave_(0)
    , blocksSpawnedInWave_(0)
    , totalBlocksSpawned_(0)
    , timeSinceLastSpawn_(0.0f)
    , waveSpawnTimer_(0.0f)
    , isSpawning_(false)
    , waitingForWaveDelay_(false)
    , levelComplete_(false)
    , blockReachedBottom_(false)
{
    // Initialize random seed
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
}

void BlockManager::update(float deltaTime, const sf::FloatRect& cannonBounds)
{
    // Update all blocks
    for (auto& block : blocks_) {
        if (block && !block->isDestroyed()) {
            block->update(deltaTime);
            
            // Check if block reached bottom (game over condition)
            if (!blockReachedBottom_ && hasBlockReachedBottom(*block)) {
                blockReachedBottom_ = true;
            }
            
            // Check if block touches cannon (game over condition)
            if (doesBlockTouchCannon(*block, cannonBounds)) {
                // Game over condition detected in PlayingState
            }
        } else if (block) {
            // Block is destroyed, but we still need to update it once to ensure
            // isDestroyed() returns true (blocks check their bricks in update)
            block->update(deltaTime);
        }
    }
    
    // Handle wave spawning
    if (isSpawning_) {
        // Check if we need to spawn next block
        timeSinceLastSpawn_ += deltaTime;
        
        // Spawn blocks at spawn interval
        while (timeSinceLastSpawn_ >= currentLevelConfig_.spawnInterval && 
               blocksSpawnedInWave_ < blocksInCurrentWave_ &&
               totalBlocksSpawned_ < currentLevelConfig_.blockCount) {
            spawnBlock();
            timeSinceLastSpawn_ -= currentLevelConfig_.spawnInterval;
            blocksSpawnedInWave_++;
            totalBlocksSpawned_++;
        }
        
        // Check if wave is complete (all blocks spawned in this wave)
        if (blocksSpawnedInWave_ >= blocksInCurrentWave_) {
            // Wave complete
            isSpawning_ = false;
            
            // Check if there are more blocks to spawn (even if we've reached wave count)
            // This ensures we spawn ALL required blocks, even if wave calculation was off
            if (totalBlocksSpawned_ < currentLevelConfig_.blockCount) {
                // Check if we should continue with more waves
                if (currentWave_ < currentLevelConfig_.waveCount) {
                    // Wait for wave delay before next wave
                    waitingForWaveDelay_ = true;
                    waveSpawnTimer_ = 0.0f;
                } else {
                    // We've reached the wave count limit, but still have blocks to spawn
                    // Continue spawning immediately to ensure all blocks are spawned
                    startWave();  // Start a new wave to spawn remaining blocks
                }
            } else {
                // All blocks spawned, wait for level completion
                waitingForWaveDelay_ = false;
            }
        }
    }
    
    // Handle wave delay
    if (waitingForWaveDelay_) {
        waveSpawnTimer_ += deltaTime;
        
        if (waveSpawnTimer_ >= currentLevelConfig_.waveDelay) {
            // Start next wave
            waitingForWaveDelay_ = false;
            startWave();
        }
    }
    
    // NOTE: Level completion check and block removal happen in updateBlockDestroyedStates()
    // which is called AFTER collision detection in PlayingState. This ensures blocks are
    // properly marked as destroyed before we check for level completion.
}

void BlockManager::updateBlockDestroyedStates(float /* deltaTime */)
{
    // Update all blocks to check if they should be marked as destroyed
    // This is called after collision detection to ensure blocks are marked as destroyed
    // when all their bricks are destroyed.
    // We pass 0.0f as deltaTime to Block::update() to check state without moving blocks.
    for (auto& block : blocks_) {
        if (block && !block->isDestroyed()) {
            // Call update with 0.0f deltaTime to check if all bricks are destroyed
            // without moving the block (blocks are already updated in main update())
            block->update(0.0f);
        }
    }
    
    // Check level completion BEFORE removing blocks
    // This ensures we can detect when all blocks are destroyed
    if (!levelComplete_) {
        // Check if all required blocks have been spawned
        bool allBlocksSpawned = (totalBlocksSpawned_ >= currentLevelConfig_.blockCount);
        
        if (allBlocksSpawned) {
            // All required blocks have been spawned, check if they're all cleared
            if (blocks_.empty()) {
                // All blocks have been removed (destroyed or off-screen)
                levelComplete_ = true;
            } else {
                // Check if all remaining blocks are destroyed or off-screen
                bool allBlocksCleared = true;
                
                for (const auto& block : blocks_) {
                    if (block) {
                        if (!block->isDestroyed() && !isBlockOffScreen(*block)) {
                            // Block is active (not destroyed, not off-screen)
                            allBlocksCleared = false;
                            break;
                        }
                    }
                }
                
                // Level is complete if all blocks are either destroyed or off-screen
                if (allBlocksCleared) {
                    levelComplete_ = true;
                }
            }
        }
    }
    
    // Remove off-screen and destroyed blocks AFTER level completion check
    // This cleanup helps performance and ensures destroyed blocks don't accumulate
    removeOffScreenBlocks();
}

void BlockManager::render(sf::RenderWindow& window) const
{
    // Render all active blocks
    for (const auto& block : blocks_) {
        if (block && !block->isDestroyed()) {
            block->render(window);
        }
    }
}

void BlockManager::startLevel(int level)
{
    // Reset state
    currentLevel_ = level;
    currentWave_ = 0;
    blocksInCurrentWave_ = 0;
    blocksSpawnedInWave_ = 0;
    totalBlocksSpawned_ = 0;
    timeSinceLastSpawn_ = 0.0f;
    waveSpawnTimer_ = 0.0f;
    isSpawning_ = false;
    waitingForWaveDelay_ = false;
    levelComplete_ = false;
    blockReachedBottom_ = false;
    
    // Clear all blocks
    blocks_.clear();
    
    // Calculate level configuration
    currentLevelConfig_ = calculateLevelConfig(level);
    
    // Start first wave immediately
    startWave();
}

void BlockManager::advanceLevel()
{
    // Advance to next level
    startLevel(currentLevel_ + 1);
}

bool BlockManager::isLevelComplete() const
{
    return levelComplete_;
}

bool BlockManager::isWaveComplete() const
{
    return !isSpawning_ && blocksSpawnedInWave_ >= blocksInCurrentWave_;
}

std::vector<Block*> BlockManager::getActiveBlocks()
{
    std::vector<Block*> activeBlocks;
    for (auto& block : blocks_) {
        if (block && !block->isDestroyed()) {
            activeBlocks.push_back(block.get());
        }
    }
    return activeBlocks;
}

std::vector<const Block*> BlockManager::getActiveBlocks() const
{
    std::vector<const Block*> activeBlocks;
    for (const auto& block : blocks_) {
        if (block && !block->isDestroyed()) {
            activeBlocks.push_back(block.get());
        }
    }
    return activeBlocks;
}

bool BlockManager::hasBlocksReachedBottom() const
{
    return blockReachedBottom_;
}

bool BlockManager::hasBlocksTouchedCannon(const sf::FloatRect& cannonBounds) const
{
    for (const auto& block : blocks_) {
        if (block && !block->isDestroyed()) {
            if (doesBlockTouchCannon(*block, cannonBounds)) {
                return true;
            }
        }
    }
    return false;
}

int BlockManager::getActiveBlockCount() const
{
    int count = 0;
    for (const auto& block : blocks_) {
        if (block && !block->isDestroyed()) {
            count++;
        }
    }
    return count;
}

void BlockManager::setWindowSize(unsigned int width, unsigned int height)
{
    windowWidth_ = width;
    windowHeight_ = height;
}

BlockManager::LevelConfig BlockManager::calculateLevelConfig(int level) const
{
    LevelConfig config;
    
    // Level-based block count
    if (level == 1) {
        config.blockCount = 5 + (std::rand() % 4);  // 5-8 blocks
    } else if (level == 2) {
        config.blockCount = 10 + (std::rand() % 3);  // 10-12 blocks
    } else if (level == 3) {
        config.blockCount = 15 + (std::rand() % 4);  // 15-18 blocks
    } else if (level == 4) {
        config.blockCount = 20 + (std::rand() % 6);  // 20-25 blocks
    } else {
        // Level 5+
        config.blockCount = 25 + (std::rand() % 11);  // 25-35 blocks
    }
    
    // Level-based descent speed (constant per level)
    if (level == 1) {
        config.descentSpeed = 40.0f;  // 30-50 px/s average: 40
    } else if (level == 2) {
        config.descentSpeed = 65.0f;  // 50-80 px/s average: 65
    } else if (level == 3) {
        config.descentSpeed = 100.0f;  // 80-120 px/s average: 100
    } else if (level == 4) {
        config.descentSpeed = 140.0f;  // 120-160 px/s average: 140
    } else {
        // Level 5+
        config.descentSpeed = 190.0f;  // 160-220 px/s average: 190
    }
    
    // Level-based wave configuration
    if (level == 1) {
        config.waveCount = 2 + (std::rand() % 2);  // 2-3 waves
        config.blocksPerWave = 2 + (std::rand() % 2);  // 2-3 blocks per wave
        config.waveDelay = 2.0f;  // 2.0s delay
        config.spawnInterval = 0.3f;  // 0.3s spawn interval
    } else if (level == 2) {
        config.waveCount = 3 + (std::rand() % 2);  // 3-4 waves
        config.blocksPerWave = 3 + (std::rand() % 2);  // 3-4 blocks per wave
        config.waveDelay = 1.5f;  // 1.5s delay
        config.spawnInterval = 0.25f;  // 0.25s spawn interval
    } else if (level == 3) {
        config.waveCount = 4 + (std::rand() % 2);  // 4-5 waves
        config.blocksPerWave = 4 + (std::rand() % 2);  // 4-5 blocks per wave
        config.waveDelay = 1.2f;  // 1.2s delay
        config.spawnInterval = 0.2f;  // 0.2s spawn interval
    } else if (level == 4) {
        config.waveCount = 5 + (std::rand() % 2);  // 5-6 waves
        config.blocksPerWave = 5 + (std::rand() % 2);  // 5-6 blocks per wave
        config.waveDelay = 1.0f;  // 1.0s delay
        config.spawnInterval = 0.15f;  // 0.15s spawn interval
    } else {
        // Level 5+
        config.waveCount = 6 + (std::rand() % 3);  // 6-8 waves
        config.blocksPerWave = 6 + (std::rand() % 3);  // 6-8 blocks per wave
        config.waveDelay = 0.8f;  // 0.8s delay
        config.spawnInterval = 0.1f;  // 0.1s spawn interval
    }
    
    // Adjust wave count if blocksPerWave * waveCount exceeds blockCount
    // This ensures we don't spawn more blocks than configured
    int totalBlocksFromWaves = config.blocksPerWave * config.waveCount;
    if (totalBlocksFromWaves > config.blockCount) {
        // Recalculate wave count based on actual block count
        config.waveCount = (config.blockCount + config.blocksPerWave - 1) / config.blocksPerWave;  // Ceiling division
        if (config.waveCount < 1) {
            config.waveCount = 1;
        }
    }
    
    // Spawn Y offset (above screen)
    config.spawnYOffset = SPAWN_Y_OFFSET;
    
    return config;
}

void BlockManager::startWave()
{
    currentWave_++;
    blocksSpawnedInWave_ = 0;
    
    // Calculate blocks in this wave
    int remainingBlocks = currentLevelConfig_.blockCount - totalBlocksSpawned_;
    blocksInCurrentWave_ = std::min(currentLevelConfig_.blocksPerWave, remainingBlocks);
    
    // Ensure we don't spawn more than the total block count
    if (blocksInCurrentWave_ <= 0) {
        // No more blocks to spawn
        isSpawning_ = false;
        waitingForWaveDelay_ = false;
        return;
    }
    
    // Start spawning immediately (spawn first block right away, then use intervals)
    isSpawning_ = true;
    waitingForWaveDelay_ = false;
    timeSinceLastSpawn_ = currentLevelConfig_.spawnInterval;  // Start at spawn interval so first block spawns immediately
}

void BlockManager::spawnBlock()
{
    // Calculate spawn position (random X, above screen)
    float spawnY = currentLevelConfig_.spawnYOffset;
    
    // Get block bounds to calculate min/max X (account for block size)
    // Use maximum block size for safety (hexagon radius = 120px, so half = 60px)
    float blockHalfWidth = 90.0f;  // Maximum half-width (rectangle width/2 = 90px)
    float minX = blockHalfWidth;
    float maxX = static_cast<float>(windowWidth_) - blockHalfWidth;
    
    // Ensure valid range
    if (maxX < minX) {
        minX = static_cast<float>(windowWidth_) / 2.0f;
        maxX = minX;
    }
    
    // Random X position
    float spawnX = minX + static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) * (maxX - minX);
    
    // Get random shape and color based on level
    BlockShape shape = Block::getRandomShape(currentLevel_);
    sf::Color color = Block::getRandomColor();
    
    // Create block
    auto block = std::make_unique<Block>(
        sf::Vector2f(spawnX, spawnY),
        shape,
        color,
        currentLevel_
    );
    
    // Set descent velocity (downward, constant speed)
    float descentSpeed = currentLevelConfig_.descentSpeed;
    block->setVelocity(sf::Vector2f(0.0f, descentSpeed));
    
    // Add to blocks vector
    blocks_.push_back(std::move(block));
}

void BlockManager::removeOffScreenBlocks()
{
    // Remove blocks that are off-screen (entire block below screen) OR destroyed
    // This helps keep the blocks_ vector clean and speeds up level completion checks
    blocks_.erase(
        std::remove_if(
            blocks_.begin(),
            blocks_.end(),
            [this](const std::unique_ptr<Block>& block) {
                // Remove if block is off-screen OR destroyed
                return block && (isBlockOffScreen(*block) || block->isDestroyed());
            }
        ),
        blocks_.end()
    );
}

bool BlockManager::isBlockOffScreen(const Block& block) const
{
    // Check if entire block is below screen bottom
    sf::FloatRect blockBounds = block.getBounds();
    
    // Block is off-screen if its top edge is below screen bottom
    return blockBounds.position.y > static_cast<float>(windowHeight_);
}

bool BlockManager::hasBlockReachedBottom(const Block& block) const
{
    // Check if block has reached bottom of screen (with safety margin)
    sf::FloatRect blockBounds = block.getBounds();
    
    // Block reached bottom if its bottom edge is at or below screen bottom (minus margin)
    return (blockBounds.position.y + blockBounds.size.y) >= (static_cast<float>(windowHeight_) - BOTTOM_MARGIN);
}

bool BlockManager::doesBlockTouchCannon(const Block& block, const sf::FloatRect& cannonBounds) const
{
    // AABB collision detection between block and cannon
    sf::FloatRect blockBounds = block.getBounds();
    return checkAABBCollision(blockBounds, cannonBounds);
}

bool BlockManager::checkAABBCollision(const sf::FloatRect& rect1, const sf::FloatRect& rect2) const
{
    // AABB collision detection
    // SFML 3.0: Rect uses position and size (Vector2f)
    return (rect1.position.x < rect2.position.x + rect2.size.x &&
            rect1.position.x + rect1.size.x > rect2.position.x &&
            rect1.position.y < rect2.position.y + rect2.size.y &&
            rect1.position.y + rect1.size.y > rect2.position.y);
}

