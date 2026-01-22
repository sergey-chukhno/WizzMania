
# Wizz Mania Client
# Requires Qt 6
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Client Sources
set(CLIENT_SOURCES
    main.cpp
    NetworkManager.cpp
    LoginDialog.cpp
    resources.qrc
)

# Enable AutoRcc for resource processing
set(CMAKE_AUTORCC ON)

# Add Executable
add_executable(wizz_client ${CLIENT_SOURCES})

# Link Libraries
target_link_libraries(wizz_client
    PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    wizz_common # Link our Packet library
)
