#ifndef BLOCK_H
#define BLOCK_H

#include <SFML/Graphics.hpp>
#include "entities/Brick.h"
#include <memory>
#include <vector>

/**
 * @brief Enum for block shapes.
 */
enum class BlockShape {
    Square,     // Square block (120×120px)
    Rectangle,  // Rectangle block (180×90px)
    Hexagon     // Hexagon block (120px radius)
};

/**
 * @brief Represents a complex block composed of multiple bricks.
 * 
 * Blocks contain multiple bricks arranged randomly within block bounds.
 * Bricks have individual health based on distance from center (center stronger, edges weaker).
 * Block is destroyed when all bricks are destroyed.
 */
class Block {
public:
    /**
     * @brief Constructs a block at the specified position.
     * @param position Block position in world coordinates (center of block)
     * @param shapeType Block shape (Square, Rectangle, Hexagon)
     * @param baseColor Base color from cyberpunk palette
     * @param level Level this block belongs to (for scaling)
     */
    Block(const sf::Vector2f& position, BlockShape shapeType, const sf::Color& baseColor, int level);

    /**
     * @brief Destructor.
     */
    ~Block() = default;

    /**
     * @brief Updates the block state (all bricks).
     * @param deltaTime Time elapsed since last frame (in seconds)
     */
    void update(float deltaTime);

    /**
     * @brief Renders the block (all bricks).
     * @param window Reference to the render window
     */
    void render(sf::RenderWindow& window) const;

    /**
     * @brief Checks if the block is destroyed (all bricks destroyed).
     * @return True if destroyed, false otherwise
     */
    bool isDestroyed() const;

    /**
     * @brief Gets the bounding rectangle of the block.
     * @return Bounding rectangle
     */
    sf::FloatRect getBounds() const;

    /**
     * @brief Gets the position of the block (center).
     * @return Position vector
     */
    sf::Vector2f getPosition() const;

    /**
     * @brief Gets the level of the block.
     * @return Level
     */
    int getLevel() const;

    /**
     * @brief Gets the shape type of the block.
     * @return Block shape type
     */
    BlockShape getShapeType() const;

    /**
     * @brief Sets the position of the block.
     * @param position New position
     */
    void setPosition(const sf::Vector2f& position);

    /**
     * @brief Sets the velocity of the block (for descent).
     * @param velocity Velocity vector
     */
    void setVelocity(const sf::Vector2f& velocity);

    /**
     * @brief Gets the velocity of the block.
     * @return Velocity vector
     */
    sf::Vector2f getVelocity() const;

    /**
     * @brief Gets all bricks in the block (for collision detection).
     * @return Vector of brick pointers
     */
    std::vector<Brick*> getBricks();

    /**
     * @brief Gets all bricks in the block (const version).
     * @return Vector of const brick pointers
     */
    std::vector<const Brick*> getBricks() const;

    /**
     * @brief Gets a random color from the cyberpunk palette.
     * @return Random cyberpunk color
     */
    static sf::Color getRandomColor();

    /**
     * @brief Gets a random block shape based on level.
     * @param level Level number
     * @return Random block shape
     */
    static BlockShape getRandomShape(int level);

    /**
     * @brief Calculates maximum health based on level (legacy method, kept for compatibility).
     * @param level Level number
     * @return Maximum health
     * @deprecated This method is no longer used in the multi-brick system
     */
    static int calculateMaxHealth(int level);

private:
    // Position and movement
    sf::Vector2f position_;
    sf::Vector2f velocity_;

    // Shape and visual
    BlockShape shapeType_;
    sf::Color baseColor_;
    int level_;
    bool isDestroyed_;

    // Bricks
    std::vector<std::unique_ptr<Brick>> bricks_;
    std::vector<sf::Vector2f> brickRelativePositions_; // Relative positions of bricks (for movement)

    // Block size constants
    static constexpr float SQUARE_SIZE = 120.0f;        // Square block size (120×120px)
    static constexpr float RECTANGLE_WIDTH = 180.0f;    // Rectangle block width
    static constexpr float RECTANGLE_HEIGHT = 90.0f;    // Rectangle block height
    static constexpr float HEXAGON_RADIUS = 120.0f;     // Hexagon block radius

    // Brick constants
    static constexpr float BRICK_WIDTH = 60.0f;         // Brick width (60px)
    static constexpr float BRICK_HEIGHT = 30.0f;        // Brick height (30px, rectangular)
    static constexpr float BRICK_SPACING = 0.0f;        // Gap between bricks (0px, no gap)
    static constexpr int MAX_PLACEMENT_ATTEMPTS = 100;  // Maximum placement attempts per brick

    // Cyberpunk color palette
    static constexpr sf::Color COLOR_PINK = sf::Color(255, 0, 110);      // #ff006e
    static constexpr sf::Color COLOR_CYAN = sf::Color(0, 217, 255);      // #00d9ff
    static constexpr sf::Color COLOR_PURPLE = sf::Color(157, 78, 221);   // #9d4edd
    static constexpr sf::Color COLOR_GREEN = sf::Color(6, 255, 165);     // #06ffa5
    static constexpr sf::Color COLOR_YELLOW = sf::Color(255, 190, 11);   // #ffbe0b

    /**
     * @brief Initializes bricks with random arrangement and health pattern.
     */
    void initializeBricks();

    /**
     * @brief Calculates the number of bricks based on shape and level.
     * @param shapeType Block shape
     * @param level Level number
     * @return Number of bricks (random within range)
     */
    static int calculateBrickCount(BlockShape shapeType, int level);

    /**
     * @brief Calculates brick health based on distance from block center (pattern: center stronger, edges weaker).
     * @param brickPosition Brick position (relative to block center)
     * @param maxDistance Maximum distance from center
     * @param level Level number
     * @return Brick health
     */
    static int calculateBrickHealth(const sf::Vector2f& brickPosition, float maxDistance, int level);

    /**
     * @brief Gets block bounds based on shape type.
     * @return Block bounds (relative to block center)
     */
    sf::FloatRect getBlockBounds() const;

    /**
     * @brief Gets maximum distance from center for health calculation.
     * @return Maximum distance
     */
    float getMaxDistance() const;

    /**
     * @brief Checks if a position is within block bounds.
     * @param position Position to check (relative to block center)
     * @return True if within bounds, false otherwise
     */
    bool isWithinBounds(const sf::Vector2f& position) const;

    /**
     * @brief Checks if a brick position overlaps with existing bricks.
     * @param position Brick position (relative to block center)
     * @param existingBricks Existing brick positions
     * @return True if overlaps, false otherwise
     */
    bool overlapsWithExisting(const sf::Vector2f& position, const std::vector<sf::Vector2f>& existingBricks) const;

    /**
     * @brief Generates a random position within block bounds.
     * @return Random position (relative to block center)
     */
    sf::Vector2f generateRandomPosition() const;
};

#endif // BLOCK_H
