#[======================================================================[.rst:
IrodsCXXCompiler
----------------

Experimental stuff

#]======================================================================]

include_guard(GLOBAL)

include(IrodsExternals)

cmake_policy(PUSH)

set(IRODS_BUILD_WITH_CLANG ON CACHE BOOL "Try to build with clang instead of gcc")
set(CLANG_STATIC_ANALYZER OFF CACHE BOOL "Choose whether to run Clang Static Analyzer.")

if (IRODS_BUILD_WITH_CLANG)
	IRODS_MACRO_CHECK_DEPENDENCY_SET_FULLPATH(CLANG clang13.0.0-0)
else()
	set(CLANG_STATIC_ANALYZER OFF CACHE BOOL "Choose whether to run Clang Static Analyzer." FORCE)
	message(STATUS "Setting CLANG_STATIC_ANALYZER to 'OFF'.")
endif()

if (CLANG_STATIC_ANALYZER)
	if (NOT CMAKE_C_COMPILER)
		set(CMAKE_C_COMPILER ${IRODS_EXTERNALS_FULLPATH_CLANG}/bin/ccc-analyzer)
	endif()
	if (NOT CMAKE_CXX_COMPILER)
		set(CMAKE_CXX_COMPILER ${IRODS_EXTERNALS_FULLPATH_CLANG}/bin/c++-analyzer)
	endif()
elseif (IRODS_BUILD_WITH_CLANG)
	if (NOT CMAKE_C_COMPILER)
		set(CMAKE_C_COMPILER ${IRODS_EXTERNALS_FULLPATH_CLANG}/bin/clang)
	endif()
	if (NOT CMAKE_CXX_COMPILER)
		set(CMAKE_CXX_COMPILER ${IRODS_EXTERNALS_FULLPATH_CLANG}/bin/clang++)
	endif()
endif()

cmake_policy(POP)
