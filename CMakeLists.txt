cmake_minimum_required(VERSION 3.10)

project(SocialMedia)

file(GLOB SOURCES "*.cpp")

add_executable(sociald ${SOURCES})

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

target_link_libraries(sociald PRIVATE Threads::Threads)

find_library(MYSQLCLIENT_LIB 
  NAMES mysqlclient 
  PATHS /usr/lib /usr/local/lib
)

target_link_libraries(sociald PRIVATE mysqlclient)

target_link_libraries(sociald PRIVATE ${MYSQLCLIENT_LIB})