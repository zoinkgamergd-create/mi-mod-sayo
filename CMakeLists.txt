cmake_minimum_required(VERSION 3.21)
cmake_policy(VERSION 3.21)

project(click_animations)

find_package(Geode REQUIRED)

add_geode_mod(click_animations
    SOURCES src/main.cpp
)

target_link_libraries(click_animations PRIVATE Geode::geode)
