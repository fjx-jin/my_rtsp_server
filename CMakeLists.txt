cmake_minimum_required(VERSION 3.10)
project(RTSP_SERVER)
add_definitions("-Wall -g")
# 设置使用C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
# libavdevice 
include_directories(./include)
file(GLOB_RECURSE SOURCES "src/*.cpp")
# 添加可执行文件


file(GLOB_RECURSE SRC_DIR_LIST "src/*.cpp")

add_library(
        ${PROJECT_NAME} STATIC
        ${SRC_DIR_LIST}	
)

add_executable(rtsp_server  src/main.cpp)

target_link_libraries(rtsp_server ${PROJECT_NAME} )
target_link_libraries(rtsp_server pthread)

