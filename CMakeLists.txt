cmake_minimum_required(VERSION 3.20)
project("Random Scripts")

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED true)

add_compile_options(-Wall -Wextra -Wshadow -Wnull-dereference -Wpedantic -Wformat=2)
add_compile_options("$<$<CONFIG:Debug>:-fsanitize=address,undefined>")
add_link_options("$<$<CONFIG:Debug>:-fsanitize=address,undefined>")

add_executable(caps-blink "caps-blink.c")

add_executable(shell "shell.c")

add_executable(fgampl harmonics.c)
target_link_libraries(fgampl m)

add_executable(calculator "calculator.c")
target_compile_definitions(calculator PRIVATE READLINE_ENABLED=1)
target_link_libraries(calculator m readline)

add_library(stackfulcoro "stackful-coro/coroutine.c")