cmake_minimum_required(VERSION 3.1)
project(kakuro VERSION 0.1 LANGUAGES CXX)

enable_testing()

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})

set(KAKURO_SRC
	kakuro.cpp
)

add_executable(kakuro ${KAKURO_SRC})
set_property(TARGET kakuro PROPERTY CXX_STANDARD 17)