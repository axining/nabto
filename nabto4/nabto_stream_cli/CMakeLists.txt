cmake_minimum_required(VERSION 3.1)
 
project(nabto_stream_cli)
 
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_COMPILER g++)

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib)

include_directories(
  ${SRC_DIR}/src
  ${SRC_DIR}/3rdparty
)

link_directories(${LIB_DIR})

link_libraries(libnabto_client_api.so)

set(SOURCE_FILES 
  ${SRC_DIR}/src/main_stream.cpp
  ${SRC_DIR}/src/nabto_stream_log.c
  )
add_executable(${PROJECT_NAME} ${SOURCE_FILES})
 
target_link_libraries(${PROJECT_NAME} libnabto_client_api.so)
