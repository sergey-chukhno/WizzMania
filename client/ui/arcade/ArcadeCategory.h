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
        {"music", "Music", "🎵", "#1DB954", "Share Spotify/Apple Music activity"},
        {"video", "Video", "🎬", "#E50914", "Share Netflix/YouTube activity"},
        {"books", "Books", "📚", "#FF9900", "Share Kindle/Audible activity"},
        {"travel", "Travel", "✈️", "#00AEEF", "Share flight/travel status"},
        {"events", "Events", "🎟️", "#FF4500", "Share concert/event tickets"}
    };
}

} // namespace wizz::ui::arcade
