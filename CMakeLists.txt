cmake_minimum_required(VERSION 3.10)

project(HelloWorld)

find_package(fmt CONFIG REQUIRED)
find_package(PThreads4W REQUIRED)

add_executable(HelloWorld  "main.c")

target_link_libraries(HelloWorld PRIVATE PThreads4W::PThreads4W)
target_link_libraries(HelloWorld PRIVATE fmt::fmt)