# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

cmake_minimum_required(VERSION 3.22.1)
project(triangle)

find_package(game-activity REQUIRED CONFIG)
find_package(Vulkan REQUIRED)

add_library(triangle SHARED
        main.cpp
        AndroidOut.cpp)

find_package(Vulkan REQUIRED)

target_link_libraries(triangle
        game-activity::game-activity
        android
        log
        Vulkan::Vulkan)