cmake_minimum_required(VERSION 3.0.0)
project(cw_game)

#C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#platform specific
#do this for function pointers
if(WIN32)
    set(LINK_LIBS vulkan-1.lib glfw3.lib)
    set(PP_DEF "VK_USE_PLATFORM_WIN32_KHR" "NOMINMAX")
    include_directories("C:\\glfw-3.2.1.bin.WIN64\\include" "C:\\VulkanSDK\\1.0.68.0\\Include")
    link_directories("C:\\glfw-3.2.1.bin.WIN64\\lib-vc2015" "C:\\VulkanSDK\\1.0.68.0\\Lib")           #using mingw
elseif(UNIX)
    set(LINK_LIBS libvulkan.so libglfw3.a -ldl -pthread -lm
    -ldl
    -lX11
    -lXxf86vm
    -lXrandr
    -lXinerama
    -lXinerama
    -lXcursor)
    #libs
    link_directories(/usr/lib /usr/local/lib ~/VulkanSDK/1.0.68.0/x86_64/lib)
    include_directories(/usr/include /usr/local/include ~/VulkanSDK/1.0.68.0/x86_64/include)
endif(WIN32)

#headers
include_directories(./src ./src./graphics/ ./src/graphics/buffers ./src/graphics/misc ./src/graphics/objects ./src//logic ./src/maths)

#source
file(GLOB_RECURSE SOURCES ./src/*.cpp)

#program
add_executable(cw ${SOURCES})

#libraries, todo: make compatible with windows
target_link_libraries(cw ${LINK_LIBS})
