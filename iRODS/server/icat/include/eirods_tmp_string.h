/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#ifndef _tmp_string_H_
#define _tmp_string_H_

#include <string>

namespace eirods {

    class tmp_string {
    public:
	tmp_string(const std::string& orig);
	virtual ~tmp_string(void);

	char* str(void) { return string_; }

    private:
	char* string_;
    };
}; // namespace eirods

#endif // _tmp_string_H_
