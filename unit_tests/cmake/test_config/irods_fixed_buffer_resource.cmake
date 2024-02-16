set(IRODS_TEST_TARGET irods_fixed_buffer_resource)

set(IRODS_TEST_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/src/test_fixed_buffer_resource.cpp)

set(IRODS_TEST_INCLUDE_PATH ${IRODS_EXTERNALS_FULLPATH_BOOST}/include)
 
set(IRODS_TEST_LINK_LIBRARIES irods_common # only need headers
                              ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_container.so)
