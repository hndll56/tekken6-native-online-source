# GGPO (MIT, third_party/ggpo: the fork of the PS3 rollback, prediction 3 frames; inputs = XINPUT_GAMEPAD)
file(GLOB GGPO_SOURCES CONFIGURE_DEPENDS
    third_party/ggpo/src/lib/ggpo/*.cpp
    third_party/ggpo/src/lib/ggpo/backends/*.cpp
    third_party/ggpo/src/lib/ggpo/network/*.cpp)
if(WIN32)
    list(FILTER GGPO_SOURCES EXCLUDE REGEX "platform_linux\\.cpp$")
else()
    list(FILTER GGPO_SOURCES EXCLUDE REGEX "platform_windows\\.cpp$")
endif()

add_library(ggpo STATIC ${GGPO_SOURCES})
target_include_directories(ggpo PUBLIC third_party/ggpo/src/include PRIVATE third_party/ggpo/src/lib/ggpo)
if(WIN32)
    target_compile_definitions(ggpo PRIVATE _WINDOWS _WINSOCK_DEPRECATED_NO_WARNINGS _CRT_SECURE_NO_WARNINGS NOMINMAX)
    target_link_libraries(ggpo PUBLIC ws2_32 winmm)
else()
    find_package(Threads REQUIRED)
    target_link_libraries(ggpo PUBLIC Threads::Threads)
endif()
