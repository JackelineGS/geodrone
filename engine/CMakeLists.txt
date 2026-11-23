cmake_minimum_required(VERSION 3.16)
project(geodrone CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../bin)

include_directories(include)

add_library(core src/geometry.cpp src/io.cpp src/visibility.cpp src/shortest_path.cpp
                 src/solver_exact.cpp src/solver_heur.cpp src/judge.cpp
                 src/reachability.cpp src/param_search.cpp src/station_planner.cpp
                 src/graph_analysis.cpp)

add_executable(main_solver src/main_solver.cpp)
target_link_libraries(main_solver core)

add_executable(main_planner src/main_planner.cpp)
target_link_libraries(main_planner core)

add_executable(main_bench src/main_bench.cpp)
target_link_libraries(main_bench core)

add_executable(main_generator src/main_generator.cpp)
target_link_libraries(main_generator core)

add_executable(main_network src/main_network.cpp)
target_link_libraries(main_network core)

foreach(t geometry io visibility shortest_path solver solver_heur reachability param_search station_planner graph_analysis)
  add_executable(test_${t} tests/test_${t}.cpp)
  target_link_libraries(test_${t} core)
endforeach()
