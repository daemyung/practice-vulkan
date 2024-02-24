# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

cmake_minimum_required(VERSION 3.22.1)

project("practicevulkan")

find_package(game-activity REQUIRED CONFIG)
find_package(Vulkan REQUIRED)

add_library(practicevulkan SHARED
        VkRenderer.h
        VkRenderer.cpp
        VkUtil.h
        main.cpp
        AndroidOut.cpp)

target_link_libraries(practicevulkan
        game-activity::game-activity
        android
        log
        Vulkan::Vulkan)