
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
    filesystem_path
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
    image_inspect_test
)

if (TARGET mango-window)
    list(APPEND MANGO_TESTS eventloop)
endif ()

# Windows shared builds: copy mango DLLs once into tests/ (not per-test POST_BUILD).
# Parallel POST_BUILD copies raced with vcpkg z-applocal during link (permission denied).
if (WIN32 AND BUILD_SHARED_LIBS)
    foreach (lib IN LISTS MANGO_LIBRARY_TARGETS)
        if (TARGET ${lib})
            add_custom_target(copy_${lib}_to_tests ALL
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "$<TARGET_FILE:${lib}>"
                    "${CMAKE_CURRENT_BINARY_DIR}"
                DEPENDS ${lib}
            )
        endif ()
    endforeach ()
endif ()

foreach(test IN LISTS MANGO_TESTS)
    add_executable(${test} ${test}.cpp)
    target_link_libraries(${test} PRIVATE mango::mango)
    set_target_properties(${test} PROPERTIES FOLDER "tests")
    source_group("" FILES ${test}.cpp)

    if (WIN32 AND BUILD_SHARED_LIBS)
        foreach (lib IN LISTS MANGO_LIBRARY_TARGETS)
            if (TARGET ${lib})
                add_dependencies(${test} copy_${lib}_to_tests)
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

list(LENGTH MANGO_TESTS _mango_tests_count)
message("    Added ${_mango_tests_count} tests.")
