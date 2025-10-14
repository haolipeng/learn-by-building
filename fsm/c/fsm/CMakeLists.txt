cmake_minimum_required(VERSION 3.10)

project(c-fsm)

# 设置C标准
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 设置编译选项
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")

# 创建可执行文件
add_executable(c-fsm fsm.c fsm.h main.c)

# 设置输出目录
set_target_properties(c-fsm PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)