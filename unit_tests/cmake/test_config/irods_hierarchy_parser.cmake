set(IRODS_TEST_TARGET irods_hierarchy_parser)

set(IRODS_TEST_SOURCE_FILES main.cpp
                            test_irods_hierarchy_parser.cpp)

set(IRODS_TEST_INCLUDE_PATH ${CMAKE_BINARY_DIR}/lib/core/include
                            ${CMAKE_SOURCE_DIR}/lib/core/include
                            ${IRODS_EXTERNALS_FULLPATH_CATCH2}/include
                            ${IRODS_EXTERNALS_FULLPATH_BOOST}/include)
 
set(IRODS_TEST_LINK_LIBRARIES irods_common
                              c++abi)
