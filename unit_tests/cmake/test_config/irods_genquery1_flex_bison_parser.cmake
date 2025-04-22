set(IRODS_TEST_TARGET irods_genquery1_flex_bison_parser)

set(IRODS_TEST_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/test_genquery1_flex_bison_parser.cpp)

set(IRODS_TEST_LINK_LIBRARIES irods_common
                              ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so
                              fmt::fmt)
