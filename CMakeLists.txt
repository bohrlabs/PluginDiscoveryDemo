cmake_minimum_required(VERSION 3.10)
project(PluginWorkspace LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)

include_directories(${CMAKE_SOURCE_DIR}/include)

# Build plugin/shared lib
add_library(MyAddon SHARED MyAddon/MyAddon.cpp)
target_include_directories(MyAddon PRIVATE include)
set_target_properties(MyAddon PROPERTIES OUTPUT_NAME "MyAddon")

# Build host app
add_executable(HostApp HostApp/HostApp.cpp)
target_include_directories(HostApp PRIVATE include)

if(WIN32)
    target_link_libraries(HostApp PRIVATE MyAddon)
elseif(UNIX)
    target_link_libraries(HostApp PRIVATE dl)
endif()