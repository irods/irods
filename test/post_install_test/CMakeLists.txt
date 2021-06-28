cmake_minimum_required(VERSION ${CMAKE_VERSION})
project(irods_post_install_test LANGUAGES C CXX)

set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(IRODS_POST_INSTALL_TARGET irodsTestPutGet)

add_executable(${IRODS_POST_INSTALL_TARGET} main.cpp)

target_include_directories(${IRODS_POST_INSTALL_TARGET} PRIVATE ${CMAKE_BINARY_DIR}/lib/core/include
                                                                ${CMAKE_SOURCE_DIR}/lib/core/include
                                                                ${CMAKE_SOURCE_DIR}/lib/api/include
                                                                ${CMAKE_SOURCE_DIR}/lib/filesystem/include
                                                                ${CMAKE_SOURCE_DIR}/plugins/api/include
                                                                ${CMAKE_SOURCE_DIR}/server/core/include
                                                                ${CMAKE_SOURCE_DIR}/server/icat/include
                                                                ${CMAKE_SOURCE_DIR}/server/re/include
                                                                ${IRODS_EXTERNALS_FULLPATH_BOOST}/include)

target_compile_options(${IRODS_POST_INSTALL_TARGET} PRIVATE -Wall -Wextra)

target_link_libraries(${IRODS_POST_INSTALL_TARGET} PRIVATE irods_common
                                                           irods_client
                                                           irods_plugin_dependencies)
