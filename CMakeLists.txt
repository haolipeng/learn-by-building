cmake_minimum_required(VERSION 3.10)

project(learn-by-building)

# 设置全局输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 添加子项目
# Design Pattern 项目
add_subdirectory(design-pattern/lsp)

# FSM 项目
add_subdirectory(fsm/c/fsm)
add_subdirectory(fsm/c++/fsm)

# TimeWheel 项目
add_subdirectory(timewheel/c++/timewheel-c++11)
add_subdirectory(timewheel/c++/timewheel-c++98)
