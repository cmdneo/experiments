cmake_minimum_required(VERSION 3.20)
project("Random Scripts")

add_compile_options(-Wall -Wextra -Wshadow -Wnull-dereference -Wformat=2 -pedantic)

add_executable(fgampl harmonics.c)
target_link_libraries(fgampl m)

add_executable(aud-mod-af "aud-mod-af.c")
target_link_libraries(aud-mod-af m)

add_executable(test "test.c")


