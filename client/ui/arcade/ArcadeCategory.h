#pragma once

#include <QString>
#include <QIcon>
#include <QList>

namespace wizz::ui::arcade {

/**
 * @brief ArcadeCategory — Represents a distinct category in the Social Arcade.
 */
struct ArcadeCategory {
    QString id;
    QString name;
    QString icon;      // e.g., an emoji or asset path
    QString colorHex;  // Brand color for this category
    QString tooltip;
};

inline QList<ArcadeCategory> getDefaultCategories() {
    return {
        {"music",  "Music",   ":/assets/arcade/music.png",  "#A855F7", "Share Spotify/Apple Music activity"},
        {"video",  "Video",   ":/assets/arcade/video.png",  "#F97316", "Share Netflix/YouTube activity"},
        {"books",  "Books",   ":/assets/arcade/books.png",  "#22C55E", "Share Kindle/Audible activity"},
        {"travel", "Travels", ":/assets/arcade/travel.png", "#3B82F6", "Share flight/travel status"},
        {"games",  "Games",   ":/assets/arcade/games.png",  "#EC4899", "Play Arcade Games"}
    };
}

} // namespace wizz::ui::arcade
