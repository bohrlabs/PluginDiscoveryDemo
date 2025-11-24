cmake_minimum_required(VERSION 3.10)
project(PluginWorkspace LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)

include_directories(${CMAKE_SOURCE_DIR}/include)

# Build plugin/shared lib
add_library(MyAddon SHARED MyAddon/MyAddon.cpp)
target_include_directories(MyAddon PRIVATE include)
set_target_properties(MyAddon PROPERTIES OUTPUT_NAME "MyAddon")

# Build plugin/shared lib
add_library(MyAddon2 SHARED MyAddon2/MyAddon.cpp)
target_include_directories(MyAddon2 PRIVATE include)
set_target_properties(MyAddon2 PROPERTIES OUTPUT_NAME "MyAddon2")

# Build host app
add_executable(HostApp HostApp/HostApp.cpp
HostApp/SharedLibrary.hpp
HostApp/AddOnManager.hpp
HostApp/AddOnManager.cpp
HostApp/PortManager.hpp
HostApp/PortManager.cpp
include/PluginAPI.hpp
)
target_include_directories(HostApp PRIVATE include)

# Host should NOT link to plugin (it's loaded dynamically)
if(UNIX)
    target_link_libraries(HostApp PRIVATE dl)
endif()

# Put both targets into the same bin folder
set(OUTPUT_DIR ${CMAKE_BINARY_DIR}/bin)

set_target_properties(HostApp PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR}
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${OUTPUT_DIR}
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${OUTPUT_DIR}
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${OUTPUT_DIR}
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${OUTPUT_DIR}
)

set_target_properties(MyAddon PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR}          # Windows .dll
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${OUTPUT_DIR}
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${OUTPUT_DIR}
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${OUTPUT_DIR}
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${OUTPUT_DIR}
    LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIR}          # Linux .so
    LIBRARY_OUTPUT_DIRECTORY_DEBUG ${OUTPUT_DIR}
    LIBRARY_OUTPUT_DIRECTORY_RELEASE ${OUTPUT_DIR}
    LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO ${OUTPUT_DIR}
    LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL ${OUTPUT_DIR}
)
set_target_properties(MyAddon2 PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR}          # Windows .dll
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${OUTPUT_DIR}
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${OUTPUT_DIR}
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${OUTPUT_DIR}
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${OUTPUT_DIR}
    LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIR}          # Linux .so
    LIBRARY_OUTPUT_DIRECTORY_DEBUG ${OUTPUT_DIR}
    LIBRARY_OUTPUT_DIRECTORY_RELEASE ${OUTPUT_DIR}
    LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO ${OUTPUT_DIR}
    LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL ${OUTPUT_DIR}
)

# And/or just copy plugin next to HostApp after build
add_custom_command(TARGET HostApp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:MyAddon>
            $<TARGET_FILE:MyAddon2>
            $<TARGET_FILE_DIR:HostApp>
)
