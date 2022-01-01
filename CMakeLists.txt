cmake_minimum_required(VERSION 3.1)
project(kakuro VERSION 0.1 LANGUAGES CXX)

enable_testing()

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})

set(KAKURO_SRC
	kakuro.cpp
)

set(KAKURO2_SRC
	board.h
	board_generator.h
	combinations.h
	critical_path_finder.h
	kakuro2.cpp
	numbers.h
)

add_executable(kakuro ${KAKURO_SRC})
set_property(TARGET kakuro PROPERTY CXX_STANDARD 17)

add_executable(kakuro2 ${KAKURO2_SRC})
set_property(TARGET kakuro2 PROPERTY CXX_STANDARD 17)
