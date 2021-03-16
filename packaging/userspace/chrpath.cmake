#[======================================================================[.rst:
chrpath
-------


#]======================================================================]


include_guard(GLOBAL)

cmake_policy(PUSH)
cmake_policy(SET CMP0054 NEW)

macro(chrpath)
	cmake_parse_arguments(CHRPATH "" "RPATH" "TARGETS" ${ARGN})

	foreach(CHRPATH_OP_TARGET ${CHRPATH_OP_TARGETS})
		file(READ_ELF "${CHRPATH_OP_TARGET}" RUNPATH _old_runpath_in)
		file(READ_ELF "${CHRPATH_OP_TARGET}" RPATH _old_rpath_in)

		if(_old_runpath_in)
			set(old_rpath_in "${_old_runpath_in}")
		else()
			set(old_rpath_in "${_old_rpath_in}")
		endif()

		string(REPLACE ";" ":" old_rpath "${old_rpath_in}")

		file(RPATH_CHANGE
			FILE "${CHRPATH_OP_TARGET}"
			OLD_RPATH "${old_rpath}"
			NEW_RPATH "${CHRPATH_OP_RPATH}")

		unset(old_rpath)
		unset(old_rpath_in)
		unset(_old_rpath_in)
		unset(_old_runpath_in)
	endforeach()
endmacro()

if(CMAKE_CURRENT_LIST_FILE STREQUAL CMAKE_SCRIPT_MODE_FILE)
	chrpath(TARGETS "${CHRPATH_TARGETS}" RPATH "${NEW_CHRPATH}")
endif()

cmake_policy(POP)
