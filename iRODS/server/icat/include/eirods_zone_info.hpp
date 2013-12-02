/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#ifndef _zone_info_H_
#define _zone_info_H_

#include "eirods_error.hpp"

#include "icatStructs.hpp"

#include <string>

namespace eirods {

    class zone_info {
    public:
	virtual ~zone_info();

	static zone_info* get_instance(void);

	error get_local_zone(icatSessionStruct _icss, int _logSQL, std::string& _rtn_local_zone);
	
    private:
	zone_info(void);

	static zone_info* the_instance_;
	
	std::string local_zone_;
    };
}; // namespace eirods

#endif // _zone_info_H_
