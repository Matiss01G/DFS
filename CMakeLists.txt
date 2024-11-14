cmake_minimum_required(VERSION 3.10)
project(DFS VERSION 1.0)

# specify c++ standard 
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# find required packages
find_package(Boost REQUIRED COMPONENTS system)
find_package(GTest REQUIRED)

# add include directories
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/include/third_party)

# add subdirectories
add_subdirectory(src)
add_subdirectory(tests)

# create the main executable
add_executable(dfs_server src/main.cpp)

# link libraries to the main executable
target_link_libraries(dfs_server
  PRIVATE
  Boost::system
  cryptopp
  pthread
)