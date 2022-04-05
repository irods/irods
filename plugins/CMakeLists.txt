# create phony target to build everything in plugins folder
add_custom_target(all-plugins)
# and one to build everything in plugins folder *except* database plugins
add_custom_target(all-plugins_no_database)
add_dependencies(all-plugins all-plugins_no_database)

add_subdirectory(api)
add_subdirectory(auth)
add_subdirectory(auth_legacy)
add_subdirectory(database)
add_subdirectory(microservices)
add_subdirectory(network)
add_subdirectory(resources)
add_subdirectory(rule_engines)
add_subdirectory(experimental)
