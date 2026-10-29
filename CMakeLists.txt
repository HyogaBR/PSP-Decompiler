cmake_minimum_required(VERSION 3.14)
project(ConsoleDecompiler LANGUAGES C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(${PROJECT_NAME}
  src/main.c
  src/headers.c
)

target_include_directories(${PROJECT_NAME} PRIVATE
  include/
)
