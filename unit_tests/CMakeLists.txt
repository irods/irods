cmake_minimum_required(VERSION ${CMAKE_VERSION})
project(unit_tests LANGUAGES C CXX)

set(BUILD_UNIT_TESTS YES CACHE BOOL "Build unit tests")
set(UNIT_TESTS_RUN_AFTER_BUILD NO CACHE BOOL "Run after building unit tests")
set(UNIT_TESTS_REPORTING_STYLE "junit" CACHE STRING "The style of output used for unit test reporting [console, compact, junit, xml]")
set(UNIT_TESTS_REPORT_FILENAME "report.xml" CACHE STRING "The filename of the unit test report")

if (NOT BUILD_UNIT_TESTS)
    return()
endif()

# Enable CTest support.
enable_testing()

IRODS_MACRO_CHECK_DEPENDENCY_SET_FULLPATH_ADD_TO_IRODS_PACKAGE_DEPENDENCIES_LIST(CATCH2 catch22.3.0-0)

set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)

# Update the CMake module path so that the test compilation variables
# can be found.  Prepends the new path to the beginning of the list.
list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Include helper functions and other utilities.
include(utils)

if (UNIT_TESTS_RUN_AFTER_BUILD)
    set(TEST_RUNNER_ARGS -r ${UNIT_TESTS_REPORTING_STYLE}
                         -o ${UNIT_TESTS_REPORT_FILENAME})
endif()

# List of cmake files defined under ./cmake/test_config.
# Each file in the ./cmake/test_config directory defines variables for a specific test.
# New tests should be added to this list.
set(TEST_INCLUDE_LIST test_config/irods_connection_pool
                      test_config/irods_dstream
                      test_config/irods_filesystem
                      test_config/irods_hierarchy_parser
                      test_config/irods_key_value_proxy
                      test_config/irods_lifetime_manager
                      test_config/irods_linked_list_iterator
                      test_config/irods_logical_paths_and_special_characters
                      test_config/irods_query_builder
                      test_config/irods_shared_memory_object)

foreach(IRODS_TEST_CONFIG ${TEST_INCLUDE_LIST})
    unset_irods_test_variables()

    include(${IRODS_TEST_CONFIG})
    add_executable(${IRODS_TEST_TARGET} ${IRODS_TEST_SOURCE_FILES})
    set_property(TARGET ${IRODS_TEST_TARGET} PROPERTY CXX_STANDARD ${IRODS_CXX_STANDARD})
    target_include_directories(${IRODS_TEST_TARGET} PRIVATE ${IRODS_TEST_INCLUDE_PATH})
    target_link_libraries(${IRODS_TEST_TARGET} PRIVATE ${IRODS_TEST_LINK_LIBRARIES})

    # =======================
    # Post Build Instructions
    # =======================

    if (UNIT_TESTS_RUN_AFTER_BUILD)
        # Run the test immediately following successful compilation.
        add_custom_command(TARGET ${IRODS_TEST_TARGET}
                           POST_BUILD
                           COMMAND ${IRODS_TEST_TARGET} ${TEST_RUNNER_ARGS})
    endif()

    # Make the new test available to CTest.
    add_test(NAME ${IRODS_TEST_TARGET} COMMAND ${IRODS_TEST_TARGET} ${TEST_RUNNER_ARGS})
endforeach()

