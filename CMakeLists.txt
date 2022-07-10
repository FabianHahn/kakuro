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
	constrained_board.h
	critical_path_finder.h
	kakuro2.cpp
	numbers.h
	solver.h
	sum_generator.h
)

set(KAKURO_TEST_SRC
	test.cpp
	constrained_board_test.cpp
	solver_test.cpp
	sum_generator_test.cpp
)


add_executable(kakuro ${KAKURO_SRC})
set_property(TARGET kakuro PROPERTY CXX_STANDARD 17)

add_executable(kakuro2 ${KAKURO2_SRC})
set_property(TARGET kakuro2 PROPERTY CXX_STANDARD 17)

add_executable(kakuro_test ${KAKURO_TEST_SRC})
target_include_directories(kakuro_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(kakuro_test GTest::gtest GTest::gmock)
set_property(TARGET kakuro_test PROPERTY CXX_STANDARD 17)
add_test(kakuro_test kakuro_test)

add_subdirectory(thirdparty)
