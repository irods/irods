set(IRODS_TEST_TARGET irods_key_value_proxy)

set(IRODS_TEST_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/src/test_irods_key_value_proxy.cpp)

set(IRODS_TEST_INCLUDE_PATH ${IRODS_EXTERNALS_FULLPATH_BOOST}/include)

set(IRODS_TEST_LINK_LIBRARIES
    irods_common
    "${IRODS_EXTERNALS_FULLPATH_FMT}/lib/libfmt.so"
    )
