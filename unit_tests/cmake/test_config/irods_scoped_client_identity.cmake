set(IRODS_TEST_TARGET irods_scoped_client_identity)

set(IRODS_TEST_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/src/test_scoped_client_identity.cpp)

set(IRODS_TEST_INCLUDE_PATH ${CMAKE_SOURCE_DIR}/lib/core/include
                            ${CMAKE_SOURCE_DIR}/lib/api/include
                            ${CMAKE_SOURCE_DIR}/server/core/include
                            ${IRODS_EXTERNALS_FULLPATH_CATCH2}/include)
 
set(IRODS_TEST_LINK_LIBRARIES irods_server)
