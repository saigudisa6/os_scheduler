cmake_minimum_required(VERSION 3.16)
project(HighPerformanceScheduler CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# gtest setup
add_subdirectory(third_party/google_test)

# Your existing configuration
add_subdirectory(src)


# Core library
add_library(scheduler_core STATIC
    src/FCFSScheduler.cpp
)
target_include_directories(scheduler_core PUBLIC src)

add_executable(scheduler_app
	src/main.cpp
)
target_link_libraries(scheduler_app
	PRIVATE
	scheduler_core
)

add_subdirectory(tests)

enable_testing()