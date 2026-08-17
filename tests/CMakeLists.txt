
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
    ziptest
    core_buffer
    core_endian
    core_slice
    core_aligned
    core_compress
    core_half
    core_string
    core_checksum
    core_commandline
)

foreach(test IN LISTS MANGO_TESTS)
    add_executable(${test} ${test}.cpp)
    target_link_libraries(${test} PRIVATE mango::mango)
    set_target_properties(${test} PROPERTIES FOLDER "tests")
    source_group("" FILES ${test}.cpp)

    if (WIN32 AND BUILD_SHARED_LIBS)
        foreach (lib IN LISTS MANGO_LIBRARY_TARGETS)
            if (TARGET ${lib})
                add_custom_command(TARGET ${test} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "$<TARGET_FILE:${lib}>"
                        "$<TARGET_FILE_DIR:${test}>"
                )
            endif ()
        endforeach ()
    endif ()

    add_test(NAME ${test} COMMAND ${test})
    set_tests_properties(${test} PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endforeach()

if (WIN32 AND BUILD_SHARED_LIBS)
    set(_mango_test_path_dirs "${CMAKE_CURRENT_BINARY_DIR}")
    if (VCPKG_INSTALLED_DIR AND VCPKG_TARGET_TRIPLET)
        set(_vcpkg_bin "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin")
        if (IS_DIRECTORY "${_vcpkg_bin}")
            list(APPEND _mango_test_path_dirs "${_vcpkg_bin}")
        endif ()
    endif ()
    string(JOIN ";" _mango_test_path "${_mango_test_path_dirs}")
    set_tests_properties(${MANGO_TESTS} PROPERTIES
        ENVIRONMENT "PATH=${_mango_test_path};$ENV{PATH}"
    )
endif ()

file(COPY data DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
