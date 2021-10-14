set(IRODS_TEST_TARGET irods_client_server_negotiation)

set(IRODS_TEST_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/src/test_client_server_negotiation.cpp)

set(IRODS_TEST_INCLUDE_PATH ${CMAKE_SOURCE_DIR}/lib/core/include
                            ${CMAKE_SOURCE_DIR}/lib/api/include
                            ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
                            ${IRODS_EXTERNALS_FULLPATH_CATCH2}/include)

set(IRODS_TEST_LINK_LIBRARIES irods_client
                              ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_system.so)
