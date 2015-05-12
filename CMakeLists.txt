cmake_minimum_required (VERSION 2.6)
project (LMW-tree)

# TODO(cdevries): use find_library() instead
include_directories("${CMAKE_SOURCE_DIR}/external/install/include")
link_directories("${CMAKE_SOURCE_DIR}/external/install/lib")
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=c++11 -march=native -mtune=native -O2")
add_executable(emtree src/EMTree.cpp)
target_link_libraries(emtree "-ltbb -lboost_timer -lboost_system -lboost_chrono")
