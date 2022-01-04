set(IRODS_TEST_TARGET irods_fixed_buffer_resource)

set(IRODS_TEST_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/src/test_fixed_buffer_resource.cpp)

set(IRODS_TEST_INCLUDE_PATH ${CMAKE_BINARY_DIR}/lib/core/include
                            ${CMAKE_SOURCE_DIR}/lib/core/include
                            ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
                            ${IRODS_EXTERNALS_FULLPATH_FMT}/include)
 
set(IRODS_TEST_LINK_LIBRARIES ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_container.so
                              ${IRODS_EXTERNALS_FULLPATH_FMT}/lib/libfmt.so)
