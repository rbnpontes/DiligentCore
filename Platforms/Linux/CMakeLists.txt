cmake_minimum_required (VERSION 3.10)

project(Diligent-LinuxPlatform CXX)

set(INTERFACE
    interface/LinuxDebug.hpp
    interface/LinuxFileSystem.hpp
    interface/LinuxPlatformDefinitions.h
    interface/LinuxPlatformMisc.hpp
    interface/LinuxNativeWindow.h
)

set(SOURCE
    src/LinuxDebug.cpp
    src/LinuxFileSystem.cpp
    src/LinuxPlatformMisc.cpp
)

add_library(Diligent-LinuxPlatform ${SOURCE} ${INTERFACE} ${PLATFORM_INTERFACE_HEADERS})
set_common_target_properties(Diligent-LinuxPlatform)

target_include_directories(Diligent-LinuxPlatform
PUBLIC
    interface
)

target_link_libraries(Diligent-LinuxPlatform
PRIVATE
    Diligent-BuildSettings
PUBLIC
    Diligent-BasicPlatform
    Diligent-PlatformInterface
)

source_group("src" FILES ${SOURCE})
source_group("interface\\linux" FILES ${INTERFACE})
source_group("interface\\common" FILES ${PLATFORM_INTERFACE_HEADERS})

set_target_properties(Diligent-LinuxPlatform PROPERTIES
    FOLDER DiligentCore/Platforms
)

if(DILIGENT_INSTALL_CORE)
    install_core_lib(Diligent-LinuxPlatform)
endif()
