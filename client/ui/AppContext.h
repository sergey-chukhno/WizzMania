#pragma once

namespace wizz::ui {

/**
 * @brief Defines the different functional modules of the WizzMania Pro platform.
 */
enum class AppContext {
    Messenger,  ///< Standard 1-on-1 chats and contact list
    Groups,     ///< Group chats and channels
    Calls,       ///< Voice and video communication
    Settings    ///< User preferences and profile
};

} // namespace wizz::ui
