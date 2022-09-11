
add_executable(cpposu_tests)

target_sources(cpposu_tests PRIVATE
    catch_main.cpp
    test_file_parser.cpp
    test_beatmap_parser.cpp
    )

target_link_libraries(cpposu_tests PRIVATE cpposu)
target_compile_definitions(cpposu_tests PRIVATE CPPOSU_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

enable_testing()
add_test(NAME cpposu_tests COMMAND cpposu_tests)