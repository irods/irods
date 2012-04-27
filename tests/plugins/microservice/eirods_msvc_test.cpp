#include "msParam.h"
#include "reGlobalsExtern.h"
#include <iostream>

extern "C" {
	int NUMBER_OF_PARAMETERS = 3;
	int eirods_msvc_test( msParam_t* _a, msParam_t* _b, msParam_t* _c, ruleExecInfo_t* _rei ) {
		std::cout << "eirods_msvc_test :: " << parseMspForStr( _a ) << " " << parseMspForStr( _b ) << " " << parseMspForStr( _c ) << std::endl;
		return 0;
	}
}; // extern "C" 



