cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)
# target_link_libraries for object targets

add_library(
  irods_rule_engine_plugin-irods_rule_language
  MODULE
  "${CMAKE_CURRENT_SOURCE_DIR}/src/arithmetics.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/cache.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/configuration.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/conversion.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/datetime.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/filesystem.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/msiHelper.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/index.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/nre.reHelpers1.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/nre.reHelpers2.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/nre.reLib1.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/parser.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/reVariableMap.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/reVariableMap.gen.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/restructs.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/rules.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/typing.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/utils.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/rsRe.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/functions.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/irods_rule_language.cpp"
)
target_link_libraries(
  irods_rule_engine_plugin-irods_rule_language
  PRIVATE
  irods_server
  irods_common
  "${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so"
  "${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_system.so"
  "${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_regex.so"
  "${IRODS_EXTERNALS_FULLPATH_FMT}/lib/libfmt.so"
  ${CMAKE_DL_LIBS}
)
target_include_directories(
  irods_rule_engine_plugin-irods_rule_language
  PRIVATE
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  #"$<BUILD_INTERFACE:${CMAKE_IRODS_SOURCE_DIR}/plugins/rule_engines/include>"
  "$<BUILD_INTERFACE:${CMAKE_IRODS_SOURCE_DIR}/server/main_server/include>"
  "${IRODS_EXTERNALS_FULLPATH_BOOST}/include"
  "${IRODS_EXTERNALS_FULLPATH_FMT}/include"
  "${IRODS_EXTERNALS_FULLPATH_SPDLOG}/include"
)
target_compile_definitions(
  irods_rule_engine_plugin-irods_rule_language
  PRIVATE
  ${IRODS_COMPILE_DEFINITIONS_PRIVATE}
  ENABLE_RE
  IRODS_ENABLE_SYSLOG
)

add_dependencies(all-plugins-rule_engines irods_rule_engine_plugin-irods_rule_language)
install(
  TARGETS
  irods_rule_engine_plugin-irods_rule_language
  LIBRARY
  DESTINATION "${IRODS_PLUGINS_DIRECTORY}/rule_engines"
  COMPONENT ${IRODS_PACKAGE_COMPONENT_SERVER_NAME}
)
