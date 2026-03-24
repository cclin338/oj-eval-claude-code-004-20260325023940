cmake_minimum_required(VERSION 3.10)
project(BookstoreManagementSystem)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")

# Create executable
add_executable(code main.cpp)

# Set output name to 'code' as required
set_target_properties(code PROPERTIES OUTPUT_NAME "code")