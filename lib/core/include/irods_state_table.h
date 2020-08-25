#ifndef IRODS_STATE_TABLE_H
#define IRODS_STATE_TABLE_H

#ifdef MAKE_IRODS_STATE_MAP
#include <map>
#include <string>
namespace {
    namespace irods_state_map_construction {
        std::map<const int, const std::string> irods_state_map;
        std::map<const std::string, const int> irods_state_name_map;

        // We pass the variable as a const reference here to silence
        // unused variable warnings in a controlled manner.
        int create_state(const std::string& state_name, const int state_code, const int&) {
            irods_state_map.insert(std::pair<const int, const std::string>(state_code, state_name));
            irods_state_name_map.insert(std::pair<const std::string, const int>(state_name, state_code));
            return state_code;
        }
    }
}

namespace irods {
	inline void fill_state_constants ( std::map<std::string,int> & dest )
	{
		for (const auto &[k,v]: irods_state_map_construction::irods_state_name_map) {
			dest[k] = v;
		}
	}
}
#define NEW_STATE(state_name, state_code) const int state_name = irods_state_map_construction::create_state(#state_name, state_code, state_name);
#else
#define NEW_STATE(state_name, state_code) state_name = state_code,
enum IRODS_STATE_ENUM
{
#endif

// 5,000,000 - 5,999,999 - Rule Engine
// clang-format off
NEW_STATE(RULE_ENGINE_CONTINUE,              5000000)
NEW_STATE(RULE_ENGINE_SKIP_OPERATION,        5001000)
// clang-format on
#ifndef MAKE_IRODS_STATE_MAP
};
#endif

#endif // IRODS_STATE_TABLE_H
