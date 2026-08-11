
message("[Tests]")

if (MSVC)
    add_compile_options("$<$<CONFIG:Release>:/Ox>")
else ()
    add_compile_options("$<$<CONFIG:Release>:-O3>")
    add_compile_options(-Wall)
endif ()

set(MANGO_TESTS
    cache
    threads
    pathtest
    mathtest
    hash
    checksum
    aes
)

foreach(test IN LISTS MANGO_TESTS)
    add_executable(${test} ${test}.cpp)
    target_link_libraries(${test} PRIVATE mango::mango)
    set_target_properties(${test} PROPERTIES FOLDER "tests")
    source_group("" FILES ${test}.cpp)

    add_test(NAME ${test} COMMAND ${test})
    set_tests_properties(${test} PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endforeach()

file(COPY data DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
