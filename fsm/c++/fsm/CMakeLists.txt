cmake_minimum_required(VERSION 3.10)

project(cpp-fsm)

# 设置C++标准为C++98
set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# 创建可执行文件
add_executable(cpp-fsm main.cpp fsm.cpp fsm.h)

# 设置输出目录
set_target_properties(cpp-fsm PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)