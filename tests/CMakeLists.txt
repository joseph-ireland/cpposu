
add_executable(cpposu_tests)

target_sources(cpposu_tests PRIVATE
    test_file_parser.cpp
    )

target_link_libraries(cpposu_tests PRIVATE cpposu)


enable_testing()
add_test(NAME cpposu_tests COMMAND cpposu_tests)