set(IRODS_TEST_TARGET irods_generate_random_alphanumeric_string)

set(IRODS_TEST_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/src/test_generate_random_alphanumeric_string.cpp)

set(IRODS_TEST_INCLUDE_PATH ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
                            ${IRODS_EXTERNALS_FULLPATH_FMT}/include)

set(IRODS_TEST_LINK_LIBRARIES irods_common
                              ${IRODS_EXTERNALS_FULLPATH_FMT}/lib/libfmt.so)
