set(IRODS_TEST_TARGET irods_resource_administration)

set(IRODS_TEST_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/src/test_resource_administration.cpp)

set(IRODS_TEST_INCLUDE_PATH ${CMAKE_BINARY_DIR}/lib/core/include
                            ${CMAKE_SOURCE_DIR}/lib/core/include
                            ${CMAKE_SOURCE_DIR}/lib/api/include
                            ${CMAKE_SOURCE_DIR}/server/core/include
                            ${CMAKE_SOURCE_DIR}/server/icat/include
                            ${IRODS_EXTERNALS_FULLPATH_BOOST}/include)
 
set(IRODS_TEST_LINK_LIBRARIES irods_common
                              irods_client)
