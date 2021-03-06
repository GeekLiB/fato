cmake_minimum_required(VERSION 2.8.7)
project(utilities)

find_package(catkin REQUIRED COMPONENTS
  cmake_modules
)

find_package(OpenCV REQUIRED)
find_package(HDF5 COMPONENTS CXX)
find_package(Eigen REQUIRED)
add_definitions(${EIGEN_DEFINITIONS})

add_definitions(-DTRACKER_WITH_GPU)
find_package(CUDA REQUIRED)
#endif()

set(ROS_DISTRO $ENV{ROS_DISTRO})

catkin_package(
  INCLUDE_DIRS
    include
    ${CUDA_INCLUDE_DIRS}
  DEPENDS
  LIBRARIES
    ${PROJECT_NAME}
 CATKIN_DEPENDS
)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

# show additional files in qtcreator
execute_process(COMMAND find ${PROJECT_SOURCE_DIR} -type f
  OUTPUT_VARIABLE FILES_STRING)
string(REPLACE "\n" ";" FILES_LIST ${FILES_STRING})
add_custom_target(additional_files_${PROJECT_NAME} SOURCES
  ${FILES_LIST}
  ${PROJECT_SOURCE_DIR}/../README.md
)

include_directories(include
  ${catkin_INCLUDE_DIRS}
  ${OpenCV_INCLUDE_DIRS}
  ${EIGEN_INCLUDE_DIR}
  ${CUDA_INCLUDE_DIRS}
  ${HDF5_INCLUDE_DIRS}
)

add_library(${PROJECT_NAME}
  src/DebugFunctions.cpp
  src/ToString.cpp
  src/utilities.cpp
  src/GpuDebugFunctions.cpp
  src/draw_functions.cpp
  src/profiler.cpp
  src/visualization_ros.cpp
  src/logger.cpp
)

target_link_libraries(${PROJECT_NAME}
    ${catkin_LIBRARIES}
    ${OpenCV_LIBS}
    ${CUDA_LIBRARIES}
    ${HDF5_LIBRARIES}
 )
