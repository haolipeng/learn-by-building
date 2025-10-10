project(cpp-timewheel)

cmake_minimum_required(VERSION 3.10)

# 设置C++标准为C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pthread")

# 查找线程库
find_package(Threads REQUIRED)

# 创建可执行文件
add_executable(cpp-timewheel main.cpp timeWheel.cpp)

# 链接线程库
target_link_libraries(cpp-timewheel Threads::Threads)

# 设置输出目录
set_target_properties(cpp-timewheel PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)